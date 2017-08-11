#include "mainwindow.h"
#include "oculusrenderthread.h"

#include <QtWidgets>
#include <QOpenGLWidget>
#include <QOpenGLContext>

#include "mirrorrenderer.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
    , m_offscreenSurface(nullptr)
    , m_mirrorContext(nullptr)
    , m_mirrorRenderer(nullptr)
{
    setWindowTitle("Qt for Oculus Rift");

    setCentralWidget(new QWidget(nullptr));

    auto mainLayout = new QHBoxLayout(nullptr);
    centralWidget()->setLayout(mainLayout);

    resize(1280, 720);


    // Setup Mirroring
    m_mirrorView = new QWindow();
    m_mirrorView->setSurfaceType(QSurface::OpenGLSurface);
    auto format = QSurfaceFormat::defaultFormat();
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setMajorVersion(4);
    format.setMinorVersion(5);
    m_mirrorView->setFormat(format);
    m_mirrorView->create();
    m_mirrorViewWidget = QWidget::createWindowContainer(m_mirrorView, this);
    m_mirrorContext = new QOpenGLContext(this);
    m_mirrorContext->setFormat(format);
    if (!m_mirrorContext->create()) {
        qWarning("mirror context creation failed");
    }
    mainLayout->addWidget(m_mirrorViewWidget);

    // Create offscreen surface with real window handle
    m_offscreenSurface = new QWindow();
    m_offscreenSurface->setSurfaceType(QSurface::OpenGLSurface);
    m_offscreenSurface->setGeometry(0, 0, 16, 16);
    m_offscreenSurface->create();
    m_offscreenSurface->setVisible(false);

    m_renderThread = new OculusRenderThread(m_offscreenSurface, m_mirrorContext, this);
    connect(m_renderThread, &OculusRenderThread::finished, m_renderThread, &QObject::deleteLater);
    m_renderThread->start();

    connect(m_renderThread, &OculusRenderThread::isActiveChanged, this, &MainWindow::setupMirroring);
}

MainWindow::~MainWindow()
{
    delete m_mirrorContext;

    m_renderThread->stop();
    m_renderThread->quit();
    m_renderThread->wait();
    m_renderThread->deleteLater();

    delete m_offscreenSurface;
}

void MainWindow::setupMirroring(bool isActive)
{
    if (isActive) {
        m_mirrorRenderer = new MirrorRenderer(m_mirrorView, m_mirrorContext, m_renderThread->mirrorTextureSize(), m_renderThread->mirrorTextureId(), this);
        m_mirrorRenderer->setIsActive(isActive);

    } else {
        // stop rendering mirror
        m_mirrorRenderer->setIsActive(isActive);
        delete m_mirrorRenderer;
        m_mirrorRenderer = nullptr;
    }
}
