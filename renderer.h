#ifndef RENDERER_H
#define RENDERER_H

#include <OVR_CAPI.h>

#include <QObject>
#include <Ogre.h>

#include <QtCore/QSize>


class QOpenGLContext;
class QWindow;
class Scene;

class Renderer : public QObject
{
    Q_OBJECT
public:
    explicit Renderer(QWindow *surface, QOpenGLContext *glContext, const QSize &size, QObject *parent = nullptr);

    unsigned int render(const ovrTrackingState &trackingState, ovrEyeRenderDesc eyeRenderDesc[]);

    void setEyeMatrices(ovrMatrix4f left, ovrMatrix4f right);

private:
    void init();
    void loadResources();
    void setupCameras();
    void setupRenderTarget();
    void setupCompositor();
    enum {
        left = 0,
        right = 1
    };

    QWindow *m_offscreenSurface;
    QOpenGLContext *m_glContext;
    QSize m_size;
    Ogre::Root *m_root;
    Ogre::RenderWindow *m_renderWindow;
    Ogre::SceneManager *m_sceneManager;
    Ogre::SceneNode *m_camerasNode;
    Ogre::Camera *m_cameras[2];
    Ogre::CompositorWorkspace *m_workspaces[2];
    Ogre::String m_pluginsPath;
    Ogre::String m_writeAccessFolder;
    Ogre::String m_resourcePath;
    Ogre::ViewPoint *m_viewports[2];
    Ogre::Real m_nearClippingDistance;
    Ogre::Real m_farClippingDistance;
    Ogre::TexturePtr m_renderTexturePtr;
    Ogre::RenderTexture* m_renderTexture;
    Scene *m_scene;

    double m_currentFrameDisplayTime;
    double m_lastFrameDisplayTime;
};

#endif // RENDERER_H
