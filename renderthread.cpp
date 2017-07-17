#include "renderer.h"
#include "renderthread.h"

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/QSurfaceFormat>
#include <QtCore/QMutex>
#include <QtGui/QOpenGLFunctions_4_5_Core>
#include <QtGui/QColor>

#include <QtCore/QDebug>



RenderThread::RenderThread(QWindow *surface, QObject *parent)
    : QThread(parent)
    , m_surface(surface)
    , m_isActive(false)
    , m_mutex(new QMutex)
    , m_textureSwapChain(nullptr)
    , m_gl(nullptr)
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
    bool isVisible = true;
    while(true) {
        // Check if active
        m_mutex->lock();
        bool running = m_isActive;
        m_mutex->unlock();
        if (!running)
            break;

        // Update VR states
        double displayMidpointSeconds = ovr_GetPredictedDisplayTime(m_session, 0);
        ovrTrackingState hmdState = ovr_GetTrackingState(m_session, displayMidpointSeconds, ovrTrue);
        ovr_CalcEyePoses(hmdState.HeadPose.ThePose, m_hmdToEyeViewOffset, m_layer.RenderPose);

        // Visual Block
        if (isVisible) {   
            m_glContext->makeCurrent(m_surface);
            // Get next available index of the texture swap chain
            int currentIndex = 0;
            ovr_GetTextureSwapChainCurrentIndex(m_session, m_textureSwapChain, &currentIndex);
            auto currentFramebuffer = m_framebufferObjects[currentIndex];
            // Set currentFBO as active render target
            m_gl->glBindFramebuffer(GL_FRAMEBUFFER, currentFramebuffer.fbo);


            // Test render stuff
            const QColor color1 = QColor(Qt::red);
            const QColor color2 = QColor(Qt::blue);
            static bool flipper = true;
            flipper = !flipper;

            // Eye 1
            m_gl->glViewport(m_layer.Viewport[0].Pos.x,
                             m_layer.Viewport[0].Pos.y,
                             m_layer.Viewport[0].Size.w,
                             m_layer.Viewport[0].Size.h);
            if (flipper)
                m_gl->glClearColor(color1.redF(), color1.greenF(), color1.blueF(), 1.0f);
            else
                m_gl->glClearColor(color2.redF(), color2.greenF(), color2.blueF(), 1.0f);
            m_gl->glClear(GL_COLOR_BUFFER_BIT);

            // Eye 2
            m_gl->glViewport(m_layer.Viewport[1].Pos.x,
                             m_layer.Viewport[1].Pos.y,
                             m_layer.Viewport[1].Size.w,
                             m_layer.Viewport[1].Size.h);
            if (!flipper)
                m_gl->glClearColor(color1.redF(), color1.greenF(), color1.blueF(), 1.0f);
            else
                m_gl->glClearColor(color2.redF(), color2.greenF(), color2.blueF(), 1.0f);
            m_gl->glClear(GL_COLOR_BUFFER_BIT);

            m_renderer->render();

            // Commit the changes to the texture swapchain
            ovr_CommitTextureSwapChain(m_session, m_textureSwapChain);
        }

        // Submit frame with layer we use
        ovrLayerHeader* layers = &m_layer.Header;
        ovrResult result = ovr_SubmitFrame(m_session, 0, nullptr, &layers, 1);
        isVisible = (result == ovrSuccess);
        if (result == ovrError_DisplayLost || result == ovrError_TextureSwapChainInvalid)
            break;

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
    ovrResult result = ovr_Initialize(nullptr);
    if (OVR_FAILURE(result))
        return;

    ovrGraphicsLuid luid;
    result = ovr_Create(&m_session, &luid);
    if (OVR_FAILURE(result)) {
        ovr_Shutdown();
        return;
    }

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

    m_gl = m_glContext->versionFunctions<QOpenGLFunctions_4_5_Core>();
    if (!m_gl) {
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
            m_gl->glGenFramebuffers(1, &framebuffer.fbo);
            m_gl->glBindFramebuffer(GL_FRAMEBUFFER, framebuffer.fbo);
            m_gl->glBindTexture(GL_TEXTURE_2D, framebuffer.texture);
            m_gl->glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, framebuffer.texture, 0);
            // Create depth/stencil buffer
            m_gl->glGenRenderbuffers(1, &framebuffer.depthStencilBuffer);
            m_gl->glBindRenderbuffer(GL_RENDERBUFFER, framebuffer.depthStencilBuffer);
            m_gl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, bufferSize.width(), bufferSize.height());
            m_gl->glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, framebuffer.depthStencilBuffer);
            GLenum drawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
            m_gl->glDrawBuffers(1, drawBuffers);
            // Check validity of framebuffer
            GLenum status = m_gl->glCheckFramebufferStatus(GL_FRAMEBUFFER);
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

    m_renderer = new Renderer(m_surface, m_glContext);

    // Setup VR Structures
    m_hmdDesc = ovr_GetHmdDesc(m_session);
    m_eyeRenderDesc[0] = ovr_GetRenderDesc(m_session, ovrEye_Left, m_hmdDesc.DefaultEyeFov[0]);
    m_eyeRenderDesc[1] = ovr_GetRenderDesc(m_session, ovrEye_Right, m_hmdDesc.DefaultEyeFov[1]);
    m_hmdToEyeViewOffset[0] = m_eyeRenderDesc[0].HmdToEyeOffset;
    m_hmdToEyeViewOffset[1] = m_eyeRenderDesc[1].HmdToEyeOffset;

    // Initialize our single full screen Fov layer.
    m_layer.Header.Type      = ovrLayerType_EyeFov;
    m_layer.Header.Flags     = 0;
    m_layer.ColorTexture[0]  = m_textureSwapChain;
    m_layer.ColorTexture[1]  = m_textureSwapChain;
    m_layer.Fov[0]           = m_eyeRenderDesc[0].Fov;
    m_layer.Fov[1]           = m_eyeRenderDesc[1].Fov;
    m_layer.Viewport[0].Pos = { 0, 0};
    m_layer.Viewport[0].Size = { bufferSize.width() / 2, bufferSize.height() };
    m_layer.Viewport[1].Pos = {  bufferSize.width() / 2, 0 };
    m_layer.Viewport[1].Size = { bufferSize.width() / 2, bufferSize.height() };
}

void RenderThread::cleanup()
{
    m_glContext->makeCurrent(m_surface);
    ovr_DestroyTextureSwapChain(m_session, m_textureSwapChain);
    // cleanup framebuffers
    m_gl->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    for (int i = 0; i < m_framebufferObjects.count(); ++i) {
        auto framebuffer = m_framebufferObjects[i];
        m_gl->glDeleteRenderbuffers(1, &framebuffer.depthStencilBuffer);
        m_gl->glDeleteFramebuffers(1, &framebuffer.fbo);
        m_gl->glDeleteTextures(1, &framebuffer.texture);
    }
    // cleanup renderer
    delete m_renderer;

    m_glContext->doneCurrent();
    delete m_glContext;

    // Cleanup
    ovr_Destroy(m_session);
    ovr_Shutdown();

    qDebug() << "render thread cleaned up";
}
