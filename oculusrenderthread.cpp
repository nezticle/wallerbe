#include "renderer.h"
#include "oculusrenderthread.h"

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/QSurfaceFormat>
#include <QtGui/QOpenGLFunctions_4_5_Core>
#include <QtGui/QColor>

#include <QtCore/QDebug>
#include <QtCore/QMutexLocker>

#include <QtCore/QElapsedTimer>


OculusRenderThread::OculusRenderThread(QWindow *surface, QOpenGLContext *mirrorContext, QObject *parent)
    : QThread(parent)
    , m_isActive(false)
    , m_mutex(new QMutex)
    , m_textureSwapChain(nullptr)
    , m_mirrorContext(mirrorContext)
    , m_surface(surface)
    , m_gl(nullptr)
    , m_blitter(nullptr)
{

}

OculusRenderThread::~OculusRenderThread()
{
    if (m_isActive)
        stop();
}

void OculusRenderThread::run()
{
    if (!init())
        return;
    else
        setIsActive(true);

    QElapsedTimer frameTimer;
    frameTimer.start();

    bool isVisible = true;
    while(true) {
        // Check if active
        bool running = isActive();
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

            unsigned int glTexture = m_renderer->render(hmdState, m_eyeRenderDesc, frameTimer.elapsed());

            //Copy the rendered image to the Oculus Swap Texture
            m_gl->glCopyImageSubData(glTexture, GL_TEXTURE_2D, 0, 0, 0, 0,
                                     currentFramebuffer.texture, GL_TEXTURE_2D, 0, 0, 0, 0,
                                     m_outputSize.width(), m_outputSize.height(), 1);

            // Set currentFBO as active render target
//            m_gl->glBindFramebuffer(GL_FRAMEBUFFER, currentFramebuffer.fbo);
//            m_gl->glViewport(0, 0, m_outputSize.width(), m_outputSize.height());
//            m_gl->glClearColor(1.f, 0.f, 0.f, 1.f);
//            m_gl->glClear(GL_COLOR_BUFFER_BIT);
//            if (m_blitter->isCreated()) {
//                m_blitter->bind();
//                auto target = QOpenGLTextureBlitter::targetTransform(QRect(0, 0, m_outputSize.width(), m_outputSize.height()),
//                                                                     QRect(0, 0, m_outputSize.width(), m_outputSize.height()));
//                m_blitter->blit(glTexture, target, QOpenGLTextureBlitter::OriginTopLeft);
//                m_blitter->release();
//            }

            // Commit the changes to the texture swapchain
            ovr_CommitTextureSwapChain(m_session, m_textureSwapChain);
            frameTimer.restart();
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

void OculusRenderThread::stop()
{
    setIsActive(false);
}

QOpenGLContext *OculusRenderThread::openGLContext() const
{
    return m_glContext;
}

ovrSession OculusRenderThread::session() const
{
    return m_session;
}

bool OculusRenderThread::isActive() const
{
    QMutexLocker locker(m_mutex);
    return m_isActive;
}

QSize OculusRenderThread::mirrorTextureSize() const
{
    QMutexLocker locker(m_mutex);
    return QSize(m_mirrorTexture.description.Width,
                 m_mirrorTexture.description.Height);
}

unsigned int OculusRenderThread::mirrorTextureId() const
{
    QMutexLocker locker(m_mutex);
    return m_mirrorTexture.id;
}

void OculusRenderThread::setIsActive(bool isActive)
{
    QMutexLocker locker(m_mutex);
    if (m_isActive == isActive)
        return;

    m_isActive = isActive;
    emit isActiveChanged(m_isActive);
}

bool OculusRenderThread::init()
{
    ovrResult result = ovr_Initialize(nullptr);
    if (OVR_FAILURE(result))
        return false;

    ovrGraphicsLuid luid;
    result = ovr_Create(&m_session, &luid);
    if (OVR_FAILURE(result)) {
        ovr_Shutdown();
        return false;
    }

    // Setup OpenGL Context
    m_glContext = new QOpenGLContext(nullptr);

    // Setup format
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setMajorVersion(4);
    format.setMinorVersion(5);
    format.setProfile(QSurfaceFormat::CoreProfile);
    //format.setStereo(true);

    m_glContext->setFormat(format);
    m_glContext->setShareContext(m_mirrorContext);
    if (!m_glContext->create()) {
        qWarning("Could not create opengl context");
        return false;
    }

    // Get Buffer sizes
    ovrHmdDesc sessionDesc = ovr_GetHmdDesc(m_session);
    ovrSizei recommenedTex0Size = ovr_GetFovTextureSize(m_session, ovrEye_Left,
                                                        sessionDesc.DefaultEyeFov[0], 1.0f);
    ovrSizei recommenedTex1Size = ovr_GetFovTextureSize(m_session, ovrEye_Right,
                                                        sessionDesc.DefaultEyeFov[1], 1.0f);
    m_outputSize.setWidth(recommenedTex0Size.w + recommenedTex1Size.w);
    m_outputSize.setHeight(std::max( recommenedTex0Size.h, recommenedTex1Size.h));

    // Create texture swapchain
    ovrTextureSwapChainDesc desc = {};
    desc.Type = ovrTexture_2D;
    desc.ArraySize = 1;
    desc.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    desc.Width = m_outputSize.width();
    desc.Height = m_outputSize.height();
    desc.MipLevels = 1;
    desc.SampleCount = 1;
    desc.StaticImage = ovrFalse;

    if (!m_glContext->makeCurrent(m_surface)) {
        qWarning("opengl context could not be made current");
        return false;
    }

    m_gl = m_glContext->versionFunctions<QOpenGLFunctions_4_5_Core>();
    if (!m_gl) {
        qWarning("opengl functions could not be resolved");
        return false;
    }

    // Create texture blitter
    m_blitter = new QOpenGLTextureBlitter;
    if (!m_blitter->create()) {
        qWarning("Could not create headset blitter");
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
            m_gl->glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_outputSize.width(), m_outputSize.height());
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
                return false;
            } else {
                m_framebufferObjects.append(framebuffer);
            }
        }
    }

    m_renderer = new Renderer(m_surface, m_glContext, m_outputSize);

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
    m_layer.Viewport[0].Size = { m_outputSize.width() / 2, m_outputSize.height() };
    m_layer.Viewport[1].Pos = {  m_outputSize.width() / 2, 0 };
    m_layer.Viewport[1].Size = { m_outputSize.width() / 2, m_outputSize.height() };

    // Setup projection matrices for each eye
    ovrMatrix4f leftProjection = ovrMatrix4f_Projection(m_eyeRenderDesc[0].Fov,
                                                        0.2f,
                                                        1000.0f,
                                                        true);
    ovrMatrix4f rightProjection = ovrMatrix4f_Projection(m_eyeRenderDesc[1].Fov,
                                                         0.2f,
                                                         1000.0f,
                                                         true);
    m_renderer->setEyeMatrices(leftProjection, rightProjection);

    // Setup mirror Texture
    m_mutex->lock();
    m_mirrorTexture.description.Format = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    m_mirrorTexture.description.Width = m_outputSize.width();
    m_mirrorTexture.description.Height = m_outputSize.height();
    m_mirrorTexture.description.MiscFlags = ovrTextureMisc_None;
    result = ovr_CreateMirrorTextureGL(m_session, &m_mirrorTexture.description, &m_mirrorTexture.texture);
    if (result != ovrSuccess) {
        qWarning("failed to create mirror Texture");
        m_mirrorTexture.id = 0;
    } else {
        result = ovr_GetMirrorTextureBufferGL(m_session, m_mirrorTexture.texture, &m_mirrorTexture.id);
        if (result != ovrSuccess) {
            qWarning("failed to get mirror Texture ID");
            m_mirrorTexture.id = 0;
        }
    }
    m_mutex->unlock();
    return true;
}

void OculusRenderThread::cleanup()
{
    m_glContext->makeCurrent(m_surface);
    ovr_DestroyMirrorTexture(m_session, m_mirrorTexture.texture);
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

    delete m_blitter;

    m_glContext->doneCurrent();
    delete m_glContext;

    // Cleanup
    ovr_Destroy(m_session);
    ovr_Shutdown();

    qDebug() << "render thread cleaned up";
}
