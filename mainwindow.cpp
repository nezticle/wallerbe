#include "mainwindow.h"
#include "renderthread.h"

#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent)
    , m_offscreenSurface(nullptr)
{
    setWindowTitle("Qt for Oculus Rift");

    setCentralWidget(new QWidget(nullptr));

    auto mainLayout = new QHBoxLayout(nullptr);
    centralWidget()->setLayout(mainLayout);

    // Create offscreen surface with real window handle
    m_offscreenSurface = new QWindow();
    m_offscreenSurface->setSurfaceType(QSurface::OpenGLSurface);
    m_offscreenSurface->setGeometry(0, 0, 16, 16);
    m_offscreenSurface->create();
    m_offscreenSurface->setVisible(false);

    m_renderThread = new RenderThread(m_offscreenSurface, this);
    connect(m_renderThread, &RenderThread::finished, m_renderThread, &QObject::deleteLater);
    m_renderThread->start();
}

MainWindow::~MainWindow()
{
    m_renderThread->stop();
    m_renderThread->quit();
    m_renderThread->wait();
    m_renderThread->deleteLater();

    delete m_offscreenSurface;
}
