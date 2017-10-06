/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "renderer.h"
#include "scene.h"
#include "player.h"

#include <Compositor/OgreCompositorManager2.h>
#include <Compositor/OgreCompositorWorkspace.h>
#include <RenderSystems/GL3Plus/OgreGL3PlusTextureManager.h>
#include <RenderSystems/GL3Plus/OgreGL3PlusTexture.h>
#include <Hlms/Pbs/OgreHlmsPbs.h>
#include <Hlms/Unlit/OgreHlmsUnlit.h>

#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>
#include <QtPlatformHeaders/QWGLNativeContext>

Renderer::Renderer(QWindow *surface, QOpenGLContext *glContext, const QSize &size, QObject *parent)
    : QObject(parent)
    , m_offscreenSurface(surface)
    , m_glContext(glContext)
    , m_size(size)
{
    // init loads subsystems, creates cameras, and sets up the compositor
    init();
    m_scene = new Scene();
    m_scene->init(m_sceneManager, m_root, m_camerasNode, m_cameras);
}

unsigned int Renderer::render(const ovrTrackingState &trackingState, ovrEyeRenderDesc eyeRenderDesc[], qint64 timeDelta)
{
    // Update position of Head (cameras) for current tracking position
    m_scene->player()->updateFromTracking(trackingState, eyeRenderDesc);

    m_scene->update(timeDelta);

    // Render
    m_root->renderOneFrame();
//    m_root->getRenderSystem()->_beginFrameOnce();
//    m_root->_fireFrameRenderingQueued();
//    m_workspaces[left]->_beginUpdate(true);
//    m_workspaces[left]->_update();
//    m_workspaces[left]->_endUpdate(true);
//    m_workspaces[right]->_beginUpdate(true);
//    m_workspaces[right]->_update();
//    m_workspaces[right]->_endUpdate(true);

    Ogre::GL3PlusTexture* gltex = static_cast<Ogre::GL3PlusTexture*>(Ogre::GL3PlusTextureManager::getSingleton().getByName("RenderTarget").getPointer());
    bool isFsaa = false;
    unsigned int texId = gltex->getGLID(isFsaa);
    Q_ASSERT(isFsaa == false);
    return texId;
}

namespace {
Ogre::Matrix4 convertMatrix(ovrMatrix4f matrix) {
    Ogre::Matrix4 ogreProj;
    for(size_t x(0); x < 4; x++)
        for(size_t y(0); y < 4; y++)
            ogreProj[x][y] = matrix.M[x][y];
    return ogreProj;
}
}

void Renderer::setEyeMatrices(ovrMatrix4f left, ovrMatrix4f right)
{
    m_cameras[0]->setCustomProjectionMatrix(true, convertMatrix(left));
    m_cameras[1]->setCustomProjectionMatrix(true, convertMatrix(right));
}

void Renderer::init()
{
    Ogre::String pluginsPath;
    pluginsPath = m_resourcePath + "plugins.cfg";
    m_root = new Ogre::Root(pluginsPath,
                            m_resourcePath + "ogre.cfg",
                            m_resourcePath + "Ogre.log" );
#ifdef WALLERBE_DEBUG
    m_root->loadPlugin("RenderSystem_GL3Plus_d");
#else
    m_root->loadPlugin("RenderSystem_GL3Plus");
#endif
    m_root->setRenderSystem(m_root->getRenderSystemByName("OpenGL 3+ Rendering Subsystem"));

    QVariant nativeHandle = m_glContext->nativeHandle();
    if (nativeHandle.isNull() || !nativeHandle.canConvert<QWGLNativeContext>()) {
        // Abort
        qWarning("Invalid OpenGL Context");
        delete m_root;
        return;
    }

    size_t windowHandle = (size_t)m_offscreenSurface->winId();

    // Setup offscreen window connection
    Ogre::NameValuePairList params;
    params.insert(std::make_pair("externalWindowHandle",  Ogre::StringConverter::toString(windowHandle))); //null
    params.insert(std::make_pair("externalGLControl", "true"));
    params.insert(std::make_pair("currentGLContext", "true"));
    params.insert(std::make_pair("hidden", "true"));

    m_root->initialise(false);
    m_renderWindow = m_root->createRenderWindow("hidden window", 0, 0, false, &params);

    loadResources();

    Ogre::InstancingThreadedCullingMethod threadedCullingMethod = Ogre::INSTANCING_CULLING_SINGLETHREAD;
    const size_t numThreads = std::max<size_t>( 1, Ogre::PlatformInformation::getNumLogicalCores() );
    m_sceneManager = m_root->createSceneManager( Ogre::ST_GENERIC,
                                                 numThreads,
                                                 threadedCullingMethod,
                                                 "SMInstance" );
    //Set sane defaults for proper shadow mapping
    m_sceneManager->setShadowDirectionalLightExtrusionDistance( 500.0f );
    m_sceneManager->setShadowFarDistance( 500.0f );

    setupCameras();
    setupRenderTarget();
    setupCompositor();

}

void Renderer::loadResources()
{
    // Load resource paths from config file
    Ogre::ConfigFile cf;
    cf.load("resources.cfg");

    // Go through all sections & settings in the file
    Ogre::ConfigFile::SectionIterator seci = cf.getSectionIterator();

    Ogre::String secName;
    Ogre::String typeName;
    Ogre::String archName;
    while( seci.hasMoreElements() )
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();

        if( secName != "Hlms" )
        {
            Ogre::ConfigFile::SettingsMultiMap::iterator i;
            for (i = settings->begin(); i != settings->end(); ++i)
            {
                typeName = i->first;
                archName = i->second;
                Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                            archName, typeName, secName);
            }
        }
    }

    // High Level Material System
    Ogre::String dataFolder = cf.getSetting( "DoNotUseAsResource", "Hlms", "" );
    if( dataFolder.empty() )
        dataFolder = "./";
    else if( *(dataFolder.end() - 1) != '/' )
        dataFolder += "/";

    Ogre::RenderSystem *renderSystem = m_root->getRenderSystem();

    Ogre::String shaderSyntax = "GLSL";
    if( renderSystem->getName() == "Direct3D11 Rendering Subsystem" )
        shaderSyntax = "HLSL";
    else if( renderSystem->getName() == "Metal Rendering Subsystem" )
        shaderSyntax = "Metal";

    Ogre::Archive *archiveLibrary = Ogre::ArchiveManager::getSingletonPtr()->load(
                dataFolder + "Hlms/Common/" + shaderSyntax,
                "FileSystem", true );
    Ogre::Archive *archiveLibraryAny = Ogre::ArchiveManager::getSingletonPtr()->load(
                dataFolder + "Hlms/Common/Any",
                "FileSystem", true );
    Ogre::Archive *archivePbsLibraryAny = Ogre::ArchiveManager::getSingletonPtr()->load(
                dataFolder + "Hlms/Pbs/Any",
                "FileSystem", true );
    Ogre::Archive *archiveUnlitLibraryAny = Ogre::ArchiveManager::getSingletonPtr()->load(
                dataFolder + "Hlms/Unlit/Any",
                "FileSystem", true );

    Ogre::ArchiveVec library;
    library.push_back( archiveLibrary );
    library.push_back( archiveLibraryAny );

    Ogre::Archive *archiveUnlit = Ogre::ArchiveManager::getSingletonPtr()->load(
                dataFolder + "Hlms/Unlit/" + shaderSyntax,
                "FileSystem", true );

    library.push_back( archiveUnlitLibraryAny );
    Ogre::HlmsUnlit *hlmsUnlit = nullptr;
    hlmsUnlit = OGRE_NEW Ogre::HlmsUnlit( archiveUnlit, &library );
    Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsUnlit );
    library.pop_back();

    Ogre::Archive *archivePbs = Ogre::ArchiveManager::getSingletonPtr()->load(
                dataFolder + "Hlms/Pbs/" + shaderSyntax,
                "FileSystem", true );
    library.push_back( archivePbsLibraryAny );
    Ogre::HlmsPbs *hlmsPbs = nullptr;
    hlmsPbs = OGRE_NEW Ogre::HlmsPbs( archivePbs, &library );
    Ogre::Root::getSingleton().getHlmsManager()->registerHlms( hlmsPbs );
    library.pop_back();

    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups( true );
}

void Renderer::setupCameras()
{
    m_camerasNode = m_sceneManager->getRootSceneNode(Ogre::SCENE_DYNAMIC)->createChildSceneNode(Ogre::SCENE_DYNAMIC);
    m_camerasNode->setName("camerasNode");
    m_camerasNode->setPosition(0.f, 5.f, 15.f);

    m_cameras[left] = m_sceneManager->createCamera("leftEyeCamera");
    m_cameras[right] = m_sceneManager->createCamera("rightEyeCamera");

    // TODO: replace with info from Rift

    const Ogre::Real eyeDistance = 0.5f;
    const Ogre::Real eyeFocusDistance = 0.45f;

    for (int i = 0; i < 2; ++i) {
        const Ogre::Vector3 camPos(eyeDistance * (i * 2 - 1), 0, 0);
        m_cameras[i]->setPosition(camPos);

        Ogre::Vector3 lookAt(eyeFocusDistance * (i * 2 - 1), -5, -15);

        // look back along -Z
        m_cameras[i]->lookAt(lookAt);
        m_cameras[i]->setNearClipDistance(0.2f);
        m_cameras[i]->setFarClipDistance(1000.f);
        m_cameras[i]->setAutoAspectRatio(true);

        // by default cameras are attached to the root scene node
        m_cameras[i]->detachFromParent();
        m_camerasNode->attachObject(m_cameras[i]);
    }
}

void Renderer::setupRenderTarget()
{
    auto textureManager = Ogre::TextureManager::getSingletonPtr();
    m_renderTexturePtr = textureManager->createManual("RenderTarget",
                                                   Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
                                                   Ogre::TEX_TYPE_2D, m_size.width(), m_size.height(),
                                                   0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET,
                                                   0, true);
    m_renderTexture = m_renderTexturePtr->getBuffer(0, 0)->getRenderTarget();
}

void Renderer::setupCompositor()
{
    Ogre::CompositorManager2* compositionManager = m_root->getCompositorManager2();
    Ogre::uint8 viewportModifierMask = 0x01;
    Ogre::uint8 executionMask = 0x01;
    Ogre::Vector4 viewportOffsetScale = Ogre::Vector4(0.f, 0.f, .5f, 1.f);
    Ogre::CompositorChannelVec renderTextureChannel(1);
    renderTextureChannel[0].target = m_renderTexture;
    renderTextureChannel[0].textures.push_back(m_renderTexturePtr);
    Ogre::ResourceLayoutMap initialTextureLayouts;
    Ogre::ResourceAccessMap initialTextureUavAccess;
    const Ogre::IdString workspaceName( "MainWorkspace" );
    const Ogre::ColourValue backgroundColor(0.f, 0.f, 0.f, 1.f);
    if( !compositionManager->hasWorkspaceDefinition( workspaceName ) )
    {
        compositionManager->createBasicWorkspaceDef( "MainWorkspace", backgroundColor, Ogre::IdString());
    }

    m_workspaces[left] = compositionManager->addWorkspace(m_sceneManager, renderTextureChannel,
                                                          m_cameras[left], workspaceName,
                                                          true, -1, (Ogre::UavBufferPackedVec*)0,
                                                          &initialTextureLayouts,
                                                          &initialTextureUavAccess,
                                                          viewportOffsetScale,
                                                          viewportModifierMask,
                                                          executionMask);
    viewportModifierMask = 0x02;
    executionMask = 0x02;
    viewportOffsetScale = Ogre::Vector4(.5f, 0.f, .5f, 1.f);
    m_workspaces[right] = compositionManager->addWorkspace(m_sceneManager, renderTextureChannel,
                                                           m_cameras[right], workspaceName,
                                                           true, -1, (Ogre::UavBufferPackedVec*)0,
                                                           &initialTextureLayouts,
                                                           &initialTextureUavAccess,
                                                           viewportOffsetScale,
                                                           viewportModifierMask,
                                                           executionMask);
}
