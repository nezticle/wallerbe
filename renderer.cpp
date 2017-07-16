#include "renderer.h"

Renderer::Renderer(QObject *parent)
    : QObject(parent)
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

    m_root->loadPlugin("RenderSystem_GL3Plus_d");
    m_root->setRenderSystem(m_root->getRenderSystemByName("OpenGL 3+ Rendering Subsystem"));

    m_root->initialise(false);

//    m_sceneManager = m_root->createSceneManager("OctreeSceneManager", "OSM_SMGR");
//    m_sceneManager->setShadowTechnique(Ogre::ShadowTechnique::SHADOWTYPE_STENCIL_ADDITIVE);
}
