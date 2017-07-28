QT += widgets

# Oculus SDK
INCLUDEPATH += $$PWD/3rdparty/LibOVR/Include
DEPENDPATH += $$PWD/3rdparty/LibOVR/Include

release:!debug:win32: {
    LIBS += -L$$PWD/3rdparty/LibOVR/Lib/Windows/x64/Release/VS2015/ -lLibOVR
    PRE_TARGETDEPS += $$PWD/3rdparty/LibOVR/Lib/Windows/x64/Release/VS2015/LibOVR.lib
}

debug:win32:!win32-g++: {
    LIBS += -L$$PWD/3rdparty/LibOVR/Lib/Windows/x64/Debug/VS2015/ -lLibOVR
    PRE_TARGETDEPS += $$PWD/3rdparty/LibOVR/Lib/Windows/x64/Debug/VS2015/LibOVR.lib
}

# OGRE SDK
INCLUDEPATH += $$PWD/3rdparty/OGRE/include/OGRE
DEPENDPATH += $$PWD/3rdparty/OGRE/include/OGRE

release:!debug:win32:!win32-g++: {
    LIBS += -L$$PWD/3rdparty/OGRE/lib/Release -lOgreMain -lOgreHlmsPbs -lOgreHlmsUnlit
    PRE_TARGETDEPS += \
        $$PWD/3rdparty/OGRE/lib/Release/OgreMain.lib \
        $$PWD/3rdparty/OGRE/lib/Release/OgreHlmsPbs.lib \
        $$PWD/3rdparty/OGRE/lib/Release/OgreHlmsUnlit.lib

    # Copy necessary files for release
    ogre_release.files = \
        $$PWD/3rdparty/OGRE/bin/Release/OgreMain.dll \
        $$PWD/3rdparty/OGRE/bin/Release/OgreHlmsPbs.dll \
        $$PWD/3rdparty/OGRE/bin/Release/OgreHlmsUnlit.dll \
        $$PWD/3rdparty/OGRE/bin/Release/RenderSystem_GLPlus3.dll \
        $$PWD/ogre.cfg \
        $$PWD/plugins.cfg \
        $$PWD/resources.cfg
    ogre_release.path = $$OUT_PWD/release
    ogre_media.files = $$PWD/3rdparty/OGRE/media
    ogre_media.path = $$OUT_PWD/release
    COPIES += ogre_release ogre_media
}
debug:win32:!win32-g++: {
    LIBS += -L$$PWD/3rdparty/OGRE/lib/Debug -lOgreMain_d -lOgreHlmsPbs_d -lOgreHlmsUnlit_d
    PRE_TARGETDEPS += \
        $$PWD/3rdparty/OGRE/lib/Debug/OgreMain_d.lib \
        $$PWD/3rdparty/OGRE/lib/Debug/OgreHlmsPbs_d.lib \
        $$PWD/3rdparty/OGRE/lib/Debug/OgreHlmsUnlit_d.lib

    # Copy necessary files for debug
    ogre_debug.files = \
        $$PWD/3rdparty/OGRE/bin/Debug/OgreMain_d.dll \
        $$PWD/3rdparty/OGRE/bin/Debug/OgreHlmsPbs_d.dll \
        $$PWD/3rdparty/OGRE/bin/Debug/OgreHlmsUnlit_d.dll \
        $$PWD/3rdparty/OGRE/bin/Debug/RenderSystem_GL3Plus_d.dll \
        $$PWD/ogre.cfg \
        $$PWD/plugins.cfg \
        $$PWD/resources.cfg
    ogre_debug.path = $$OUT_PWD/debug
    ogre_media.files = $$PWD/3rdparty/OGRE/media
    ogre_media.path = $$OUT_PWD/debug
    COPIES += ogre_debug ogre_media
    DEFINES += WALLERBE_DEBUG
}

OTHER_FILES += \
    ogre.cfg \
    plugins.cfg \
    resources.cfg

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    renderthread.cpp \
    renderer.cpp \
    mirrorrenderer.cpp

HEADERS += \
    mainwindow.h \
    renderthread.h \
    renderer.h \
    mirrorrenderer.h
