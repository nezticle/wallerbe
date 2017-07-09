#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

#include <OVR_CAPI.h>

namespace Ui {
class MainWindow;
}

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
};

#endif // MAINWINDOW_H
