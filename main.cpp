#include <QtWidgets/QApplication>
#include <OVR_CAPI.h>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    MainWindow mainWindow(nullptr);
    mainWindow.show();

    int exitStatus = application.exec();

    return exitStatus;
}
