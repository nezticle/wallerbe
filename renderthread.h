#ifndef RENDERTHREAD_H
#define RENDERTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QVector>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <QtGui/QOpenGLContext>

class QOffscreenSurface;
class QMutex;
class Renderer;
class QOpenGLFunctions_4_5_Core;

class RenderThread : public QThread
{
public:
    RenderThread(QSurface *surface, QObject *parent = nullptr);
    ~RenderThread();
    void stop();

protected:
    void run() override;

private:
    void init();
    void cleanup();

    bool m_isActive;
    QMutex *m_mutex;
    ovrSession m_session;
    ovrTextureSwapChain m_textureSwapChain;
    QOpenGLContext *m_glContext;
    QSurface *m_surface;
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
};

#endif // RENDERTHREAD_H
