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
class RenderThread;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private:
    RenderThread *m_renderThread;
    QOffscreenSurface *m_offscreenSurface;
};

#endif // MAINWINDOW_H
