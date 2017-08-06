#include "renderer.h"

#include <Compositor/OgreCompositorManager2.h>
#include <RenderSystems/GL3Plus/OgreGL3PlusTextureManager.h>
#include <RenderSystems/GL3Plus/OgreGL3PlusTexture.h>

#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>
#include <QtPlatformHeaders/QWGLNativeContext>

Renderer::Renderer(QWindow *surface, QOpenGLContext *glContext, const QSize &size, QObject *parent)
    : QObject(parent)
    , m_offscreenSurface(surface)
    , m_glContext(glContext)
    , m_size(size)
{
    init();
}

unsigned int Renderer::render(const ovrTrackingState &trackingState, ovrEyeRenderDesc eyeRenderDesc[])
{
    auto cameraPosition = m_camerasNode->getPosition();
    auto cameraOrientation = m_camerasNode->getOrientation();
    ovrQuatf oculusOrient = trackingState.HeadPose.ThePose.Orientation;
    ovrVector3f oculusPosition = trackingState.HandPoses->ThePose.Position;
    for (int i = 0; i < 2; ++i) {
        m_cameras[i]->setOrientation(cameraOrientation * Ogre::Quaternion(oculusOrient.w, oculusOrient.x, oculusOrient.y, oculusOrient.z));
        m_cameras[i]->setPosition(cameraPosition // "gameplay" position
                                  + (m_cameras[i]->getOrientation()
                                     * Ogre::Vector3(eyeRenderDesc[i].HmdToEyeOffset.x,
                                                     eyeRenderDesc[i].HmdToEyeOffset.y,
                                                     eyeRenderDesc[i].HmdToEyeOffset.z)
                                     + cameraOrientation
                                     * Ogre::Vector3(oculusPosition.x,
                                                     oculusPosition.y,
                                                     oculusPosition.z)));
    }
    m_root->_fireFrameRenderingQueued();

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

    Ogre::String secName, typeName, archName;
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
                                                   0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);
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
