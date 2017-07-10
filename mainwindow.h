#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

namespace Ui {
class MainWindow;
}

class QOffscreenSurface;
class QOpenGLContext;
class RenderThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ovrSession session, QWidget *parent = 0);
    ~MainWindow();

    void setResolution(const QSize &size);
    void setRefreshRate(float refresh);

private slots:
    void readTrackingState();

private:
    QTimer m_timer;
    ovrSession m_session;
    ovrTextureSwapChain m_textureSwapChain;
    QOpenGLContext *m_glContext;
    QOffscreenSurface *m_offscreenSurface;
    RenderThread *m_renderThread;
};

#endif // MAINWINDOW_H
