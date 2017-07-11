#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "renderthread.h"

#include <QtWidgets>



MainWindow::MainWindow(ovrSession session, QWidget *parent) :
    QMainWindow(parent)
    , m_session(session)
    , m_offscreenSurface(nullptr)
{
    setWindowTitle("Qt for Oculus Rift");

    setCentralWidget(new QWidget(nullptr));

    auto mainLayout = new QHBoxLayout(nullptr);
    centralWidget()->setLayout(mainLayout);

    m_timer.setInterval(5);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(readTrackingState()));
    m_timer.start();

    m_offscreenSurface = new QOffscreenSurface();
    m_offscreenSurface->create();

    m_renderThread = new RenderThread(session, m_offscreenSurface, this);
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
