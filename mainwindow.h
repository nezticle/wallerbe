#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

class QWindow;
class OculusRenderThread;
class MirrorRenderer;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void setupMirroring(bool isActive);

private:
    OculusRenderThread *m_renderThread;
    QWindow *m_offscreenSurface;
    QWindow *m_mirrorView;
    QWidget *m_mirrorViewWidget;
    QOpenGLContext *m_mirrorContext;
    MirrorRenderer *m_mirrorRenderer;

};

#endif // MAINWINDOW_H
