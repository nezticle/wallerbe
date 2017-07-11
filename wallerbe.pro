QT += widgets

release:!debug:win32: LIBS += -L$$PWD/3rdparty/LibOVR/Lib/Windows/x64/Release/VS2015/ -lLibOVR
debug:win32: LIBS += -L$$PWD/3rdparty/LibOVR/Lib/Windows/x64/Debug/VS2015/ -lLibOVR

INCLUDEPATH += $$PWD/3rdparty/LibOVR/Include
DEPENDPATH += $$PWD/3rdparty/LibOVR/Include

release:!debug:win32:!win32-g++: PRE_TARGETDEPS += $$PWD/3rdparty/LibOVR/Lib/Windows/x64/Release/VS2015/LibOVR.lib
debug:win32:!win32-g++: PRE_TARGETDEPS += $$PWD/3rdparty/LibOVR/Lib/Windows/x64/Debug/VS2015/LibOVR.lib

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    renderthread.cpp \
    renderer.cpp

FORMS +=

HEADERS += \
    mainwindow.h \
    renderthread.h \
    renderer.h
