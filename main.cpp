#include <QtWidgets/QApplication>
#include <OVR_CAPI.h>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication application(argc, argv);

    ovrResult result = ovr_Initialize(nullptr);
    if (OVR_FAILURE(result))
        return -1;

    ovrSession session;
    ovrGraphicsLuid luid;
    result = ovr_Create(&session, &luid);
    if (OVR_FAILURE(result)) {
        ovr_Shutdown();
        return -1;
    }

    ovrHmdDesc desc = ovr_GetHmdDesc(session);
    ovrSizei resolution = desc.Resolution;

    MainWindow mainWindow(session);
    mainWindow.show();
    mainWindow.setResolution(QSize(resolution.w, resolution.h));
    mainWindow.setRefreshRate(desc.DisplayRefreshRate);

    int exitStatus = application.exec();

    // Cleanup
    ovr_Destroy(session);
    ovr_Shutdown();

    return exitStatus;
}
