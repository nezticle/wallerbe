#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QMutex>
#include <QtCore/QVector>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <QtGui/QOpenGLContext>

class QWindow;
class Renderer;
class QOpenGLFunctions_4_5_Core;

class RenderThread : public QThread
{
    Q_OBJECT
    Q_PROPERTY(bool isActive READ isActive WRITE setIsActive NOTIFY isActiveChanged)
public:
    RenderThread(QWindow *surface, QOpenGLContext *mirrorContext, QObject *parent = nullptr);
    ~RenderThread();
    void stop();

    QOpenGLContext *openGLContext() const;
    ovrSession session() const;

    bool isActive() const;

    QSize mirrorTextureSize() const;
    unsigned int mirrorTextureId() const;

public slots:
    void setIsActive(bool isActive);

signals:
    void isActiveChanged(bool isActive);

protected:
    void run() override;

private:
    bool init();
    void cleanup();

    bool m_isActive;
    QMutex *m_mutex;
    ovrSession m_session;
    ovrTextureSwapChain m_textureSwapChain;
    QOpenGLContext *m_mirrorContext;
    QOpenGLContext *m_glContext;
    QWindow *m_surface;
    struct FramebufferObject {
        GLuint fbo;
        GLuint depthStencilBuffer;
        GLuint texture;
    };

    QVector<FramebufferObject> m_framebufferObjects;
    Renderer *m_renderer;
    QOpenGLFunctions_4_5_Core* m_gl;

    ovrEyeRenderDesc m_eyeRenderDesc[2];
    ovrVector3f m_hmdToEyeViewOffset[2];
    ovrHmdDesc m_hmdDesc;
    ovrLayerEyeFov m_layer;

    struct MirrorTexture {
        ovrMirrorTextureDesc description;
        ovrMirrorTexture texture;
        unsigned int id;
    } m_mirrorTexture;
};

#endif // RENDERTHREAD_H
