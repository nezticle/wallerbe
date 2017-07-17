#include "renderer.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QWindow>
#include <QtPlatformHeaders/QWGLNativeContext>

Renderer::Renderer(QWindow *surface, QOpenGLContext *glContext, QObject *parent)
    : QObject(parent)
    , m_offscreenSurface(surface)
    , m_glContext(glContext)
{
    init();
}

void Renderer::render()
{

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
