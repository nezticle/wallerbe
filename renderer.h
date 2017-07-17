#ifndef RENDERER_H
#define RENDERER_H

#include <QObject>
#include <Ogre.h>

class QOpenGLContext;
class QWindow;

class Renderer : public QObject
{
    Q_OBJECT
public:
    explicit Renderer(QWindow *surface, QOpenGLContext *glContext, QObject *parent = nullptr);

    void render();

signals:

public slots:

private:
    void init();
    void loadResources();
    enum {
        left = 0,
        right = 1
    };

    QWindow *m_offscreenSurface;
    QOpenGLContext *m_glContext;
    Ogre::Root *m_root;
    Ogre::RenderWindow *m_renderWindow;
    Ogre::SceneManager *m_sceneManager;
    Ogre::Camera *m_cameras[2];
    Ogre::CompositorWorkspace *m_workspace;
    Ogre::String m_pluginsPath;
    Ogre::String m_writeAccessFolder;
    Ogre::String m_resourcePath;
    Ogre::ViewPoint *m_viewports[2];
    Ogre::Real m_nearClippingDistance;
    Ogre::Real m_farClippingDistance;

    double m_currentFrameDisplayTime;
    double m_lastFrameDisplayTime;
};

#endif // RENDERER_H
