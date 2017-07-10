#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "renderthread.h"

#include <QtWidgets>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLContext>
#include <QtGui/QSurfaceFormat>


MainWindow::MainWindow(ovrSession session, QWidget *parent) :
    QMainWindow(parent)
    , m_session(session)
    , m_textureSwapChain(nullptr)
{
    setWindowTitle("Qt for Oculus Rift");

    setCentralWidget(new QWidget(nullptr));

    auto mainLayout = new QHBoxLayout(nullptr);
    centralWidget()->setLayout(mainLayout);

    m_timer.setInterval(5);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(readTrackingState()));
    m_timer.start();

    // Setup offscreen surface and OpenGL Context
    m_offscreenSurface = new QOffscreenSurface();
    m_offscreenSurface->create();
    m_glContext = new QOpenGLContext(this);

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

    if (!m_glContext->makeCurrent(m_offscreenSurface))
        qWarning("opengl context could not be made current");

    if (ovr_CreateTextureSwapChainGL(session, &desc, &m_textureSwapChain) == ovrSuccess) {
        qDebug() << "success";
    }

    m_renderThread = new RenderThread(this);
    connect(m_renderThread, &RenderThread::finished, m_renderThread, &QObject::deleteLater);
    m_renderThread->start();
}

MainWindow::~MainWindow()
{
    m_glContext->makeCurrent(m_offscreenSurface);
    ovr_DestroyTextureSwapChain(m_session, m_textureSwapChain);
}

void MainWindow::setResolution(const QSize &size)
{
    //ui->resolutionLabel->setText("(" + QString::number(size.width()) + ", " + QString::number(size.height()) + ")");
}

void MainWindow::setRefreshRate(float refresh)
{
    //ui->refreshRateLabel->setText(QString::number(refresh));
}

void MainWindow::readTrackingState()
{
    ovrTrackingState ts = ovr_GetTrackingState(m_session, ovr_GetTimeInSeconds(), ovrTrue);
    if (ts.StatusFlags & (ovrStatus_OrientationTracked | ovrStatus_PositionTracked))
    {
        ovrPosef pose = ts.HeadPose.ThePose;
        //ui->positionLabel->setText(QString::number(pose.Position.x) + ", " + QString::number(pose.Position.y) + ", " + QString::number(pose.Position.z));
        //ui->orientationLabel->setText(QString::number(pose.Orientation.x) + ", " + QString::number(pose.Orientation.y) + ", " + QString::number(pose.Orientation.z) + ", " + QString::number(pose.Orientation.w));
    }
}
