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

//    m_sceneManager = m_root->createSceneManager("OctreeSceneManager", "OSM_SMGR");
//    m_sceneManager->setShadowTechnique(Ogre::ShadowTechnique::SHADOWTYPE_STENCIL_ADDITIVE);
}
