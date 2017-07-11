#include "renderthread.h"
#include "renderer.h"

#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QSurfaceFormat>
#include <QtCore/QMutex>
#include <QtGui/QOpenGLFunctions_4_5_Core>

#include <QtCore/QDebug>



RenderThread::RenderThread(ovrSession session, QSurface *surface, QObject *parent)
    : QThread(parent)
    , m_surface(surface)
    , m_isActive(false)
    , m_session(session)
    , m_mutex(new QMutex)
    , m_textureSwapChain(nullptr)
{

}

RenderThread::~RenderThread()
{
    m_mutex->lock();
    if (m_isActive)
        stop();
    m_mutex->unlock();
    delete m_mutex;
}

void RenderThread::run()
{
    init();
    while(true) {
        // Check if active
        m_mutex->lock();
        bool running = m_isActive;
        m_mutex->unlock();
        if (!running)
            break;

        m_renderer->render();

    }
    cleanup();
}

void RenderThread::stop()
{
    m_mutex->lock();
    m_isActive = false;
    m_mutex->unlock();
}

void RenderThread::init()
{
    m_mutex->lock();
    m_isActive = true;
    m_mutex->unlock();
    // Setup OpenGL Context
    m_glContext = new QOpenGLContext(nullptr);

    // Setup format
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setMajorVersion(4);
    format.setMinorVersion(5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setStereo(true);

    m_glContext->setFormat(format);
    if (!m_glContext->create()) {
        qWarning("Could not create opengl context");
    }

    // Get Buffer sizes
    ovrHmdDesc sessionDesc = ovr_GetHmdDesc(m_session);
    ovrSizei recommenedTex0Size = ovr_GetFovTextureSize(m_session, ovrEye_Left,
                                                        sessionDesc.DefaultEyeFov[0], 1.0f);
    ovrSizei recommenedTex1Size = ovr_GetFovTextureSize(m_session, ovrEye_Right,
                                                        sessionDesc.DefaultEyeFov[1], 1.0f);
    QSize bufferSize;
    bufferSize.setWidth(recommenedTex0Size.w + recommenedTex1Size.w);
    bufferSize.setHeight(std::max( recommenedTex0Size.h, recommenedTex1Size.h));

    qDebug() << bufferSize;

    // Create texture swapchain
    ovrTextureSwapChainDesc desc = {};
    desc.Type = ovrTexture_2D;
    desc.ArraySize = 1;
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.Width = bufferSize.width();
    desc.Height = bufferSize.height();
    desc.MipLevels = 1;
    desc.SampleCount = 1;
    desc.StaticImage = ovrFalse;

    if (!m_glContext->makeCurrent(m_surface))
        qWarning("opengl context could not be made current");

    QOpenGLFunctions_4_5_Core* gl = nullptr;
    gl = m_glContext->versionFunctions<QOpenGLFunctions_4_5_Core>();
    if (!gl) {
        qWarning("opengl functions could not be resolved");
    }

    if (ovr_CreateTextureSwapChainGL(m_session, &desc, &m_textureSwapChain) == ovrSuccess) {
        // Get textures to be used as render buffers
        int count = 0;
        ovr_GetTextureSwapChainLength(m_session, m_textureSwapChain, &count);
        for (int i = 0; i < count; ++i) {
            FramebufferObject framebuffer;
            ovr_GetTextureSwapChainBufferGL(m_session, m_textureSwapChain, i, &framebuffer.texture);
            // Create framebuffer
            gl->glGenFramebuffers(1, &framebuffer.fbo);
            gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
            gl->glBindTexture(GL_TEXTURE_2D, framebuffer.texture);
            gl->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebuffer.texture, 0);
            // Create depth/stencil buffer
            gl->glGenRenderbuffers(1, &framebuffer.depthStencilBuffer);
            gl->glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.depthStencilBuffer);
            gl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, bufferSize.width(), bufferSize.height());
            gl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer.depthStencilBuffer);
            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            gl->glDrawBuffers(1, drawBuffers);
            // Check validity of framebuffer
            GLenum status = gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
            if (status != GL_FRAMEBUFFER_COMPLETE) {
                switch(status) {
                case GL_FRAMEBUFFER_UNSUPPORTED:
                    qDebug("QOpenGLFramebufferObject: Unsupported framebuffer format.");
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
                    qDebug("QOpenGLFramebufferObject: Framebuffer incomplete attachment.");
                    break;
                case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
                    qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, missing attachment.");
                    break;
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT
                case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT:
                    qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, duplicate attachment.");
                    break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS
                case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
                    qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, attached images must have same dimensions.");
                    break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_FORMATS
                case GL_FRAMEBUFFER_INCOMPLETE_FORMATS:
                    qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, attached images must have same format.");
                    break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER
                case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
                    qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, missing draw buffer.");
                    break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER
                case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
                    qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, missing read buffer.");
                    break;
#endif
#ifdef GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE
                case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
                    qDebug("QOpenGLFramebufferObject: Framebuffer incomplete, attachments must have same number of samples per pixel.");
                    break;
#endif
                default:
                    qDebug() <<"QOpenGLFramebufferObject: An undefined error has occurred: "<< status;
                    break;
                }
                qWarning() << "framebuffer incomplete!";
            } else {
                m_framebufferObjects.append(framebuffer);
            }
        }
    }

    m_renderer = new Renderer(nullptr);
}

void RenderThread::cleanup()
{
    m_glContext->makeCurrent(m_surface);
    ovr_DestroyTextureSwapChain(m_session, m_textureSwapChain);
    auto gl = m_glContext->versionFunctions<QOpenGLFunctions_4_5_Core>();
    // cleanup framebuffers
    gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    for (int i = 0; i < m_framebufferObjects.count(); ++i) {
        auto framebuffer = m_framebufferObjects[i];
        gl->glDeleteRenderbuffers(1, &framebuffer.depthStencilBuffer);
        gl->glDeleteFramebuffers(1, &framebuffer.fbo);
        gl->glDeleteTextures(1, &framebuffer.texture);
    }
    // cleanup renderer
    delete m_renderer;

    m_glContext->doneCurrent();
    delete m_glContext;
    qDebug() << "render thread cleaned up";
}
