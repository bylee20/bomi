TEMPLATE = app
CONFIG += link_pkgconfig debug_and_release precompile_header
QT = core gui opengl network uitools
QT += quick widgets

LIBS +=  -lz -lbz2 -lpthread -lm -ldvdread -lmad -lvorbis -logg -lfaad -ldv -ldvdnavmini \
    -lxvidcore -lvorbis -logg -ltheora -la52 -ldca -lcdio_paranoia -lcdio_cdda -lcdio -lquvi

PRECOMPILED_HEADER = stdafx.hpp

precompile_header:!isEmpty(PRECOMPILED_HEADER) {
    DEFINES += USING_PCH
}

!isEmpty(RELEASE) {
        CONFIG += release
        macx:CONFIG += app_bundle
} else {
        CONFIG += debug
        macx:CONFIG -= app_bundle
}

macx {
    QMAKE_CXXFLAGS_X86_64 -= -arch x86_64 -Xarch_x86_64
    QMAKE_CXXFLAGS_X86_64 += -m64
    QMAKE_CXX = /opt/local/bin/g++-mp-4.8
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
    QMAKE_MAC_SDK = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk
    QMAKE_INFO_PLIST = Info.plist
    ICON = ../../icons/cmplayer.icns
    TARGET = CMPlayer
    LIBS +=  ../../build/lib/libavcodec.a \
	../../build/lib/libavformat.a ../../build/lib/libavutil.a ../../build/lib/libavresample.a \
	../../build/lib/libswscale.a ../../build/lib/libcmplayer_mplayer2.a \
        ../../build/lib/libchardet.a ../../build/lib/libcmplayer_sigar.a \
        -L/opt/local/lib \
        -framework VideoDecodeAcceleration -framework CoreVideo -framework Cocoa \
        -framework CoreFoundation -framework AudioUnit -framework CoreAudio -framework OpenAL \
        -framework IOKit -framework Carbon
#    LIBS += ../../build/lib/libcmplayer_skin.a
    HEADERS += app_mac.hpp
    OBJECTIVE_SOURCES += app_mac.mm
    INCLUDEPATH += /opt/local/include /usr/local/include
} else:unix {
    QT += dbus
    TARGET = cmplayer
    LIBS += -lX11 \
        -L../../build/lib -lcmplayer_skin -lcmplayer_mplayer2 -lcmplayer_sigar -lchardet -lcmplayer_av \
        -lz -lopenal -lasound -lva -lva-x11 -lva-glx -lQtDBus
    HEADERS += app_x11.hpp
    SOURCES += app_x11.cpp
}

LIBS +=  -lz -lbz2 -lpthread -lm -ldvdread -lmad -lvorbis -logg -lfaad -ldv -ldvdnavmini \
    -lxvidcore -lvorbis -logg -ltheora -la52 -ldca -lcdio_paranoia -lcdio_cdda -lcdio


INCLUDEPATH += ../mplayer2 ../../build/include ../sigar/include ../mplayer2/libmpcodecs

QMAKE_CC = "gcc -std=c99 -ffast-math -w"

QMAKE_CXXFLAGS += -std=c++11

DESTDIR = ../../build

DEFINES += _LARGEFILE_SOURCE "_FILE_OFFSET_BITS=64" _LARGEFILE64_SOURCE

RESOURCES += rsclist.qrc
HEADERS += playengine.hpp \
    mainwindow.hpp \
    mrl.hpp \
    global.hpp \
    menu.hpp \
    colorproperty.hpp \
    qtsingleapplication/qtsingleapplication.h \
    qtsingleapplication/qtlockedfile.h \
    qtsingleapplication/qtlocalpeer.h \
    qtsingleapplication/qtsinglecoreapplication.h \
    translator.hpp \
    pref.hpp \
    videoframe.hpp \
    osdrenderer.hpp \
    subtitle.hpp \
    subtitle_parser.hpp \
    subtitlerenderer.hpp \
    audiocontroller.hpp \
    info.hpp \
    charsetdetector.hpp \
    abrepeater.hpp \
    playlist.hpp \
    playlistmodel.hpp \
    playlistview.hpp \
    recentinfo.hpp \
    historyview.hpp \
    subtitleview.hpp \
    osdstyle.hpp \
    simplelistwidget.hpp \
    appstate.hpp \
    dialogs.hpp \
    favoritesview.hpp \
    downloader.hpp \
    logodrawer.hpp \
    overlay.hpp \
    videorenderer.hpp \
    avmisc.hpp \
    subtitlemodel.hpp \
    tagiterator.hpp \
    subtitle_parser_p.hpp \
    enums.hpp \
    snapshotdialog.hpp \
    events.hpp \
    listmodel.hpp \
    mainwindow_p.hpp \
    widgets.hpp \
    qtcolorpicker.hpp \
    record.hpp \
    actiongroup.hpp \
    rootmenu.hpp \
    app.hpp \
    videooutput.hpp \
    mpcore.hpp \
    prefdialog.hpp \
    richtexthelper.hpp \
    richtextblock.hpp \
    richtextdocument.hpp \
    mpmessage.hpp \
    hwaccel.hpp \
    stdafx.hpp \
    videorendereritem.hpp \
    playinfoitem.hpp \
    texturerendereritem.hpp \
    subtitlerendereritem.hpp \
    playeritem.hpp

SOURCES += main.cpp \
    mainwindow.cpp \
    mrl.cpp \
    global.cpp \
    menu.cpp \
    colorproperty.cpp \
    qtsingleapplication/qtsingleapplication.cpp \
    qtsingleapplication/qtlockedfile_win.cpp \
    qtsingleapplication/qtlockedfile_unix.cpp \
    qtsingleapplication/qtlockedfile.cpp \
    qtsingleapplication/qtlocalpeer.cpp \
    qtsingleapplication/qtsinglecoreapplication.cpp \
    translator.cpp \
    pref.cpp \
    videoframe.cpp \
    osdrenderer.cpp \
    subtitle.cpp \
    subtitle_parser.cpp \
    subtitlerenderer.cpp \
    audiocontroller.cpp \
    info.cpp \
    charsetdetector.cpp \
    abrepeater.cpp \
    playlist.cpp \
    playlistmodel.cpp \
    playlistview.cpp \
    recentinfo.cpp \
    historyview.cpp \
    subtitleview.cpp \
    osdstyle.cpp \
    simplelistwidget.cpp \
    appstate.cpp \
    dialogs.cpp \
    favoritesview.cpp \
    downloader.cpp \
    logodrawer.cpp \
    overlay.cpp \
    videorenderer.cpp \
    subtitlemodel.cpp \
    tagiterator.cpp \
    subtitle_parser_p.cpp \
    enums.cpp \
    snapshotdialog.cpp \
    events.cpp \
    listmodel.cpp \
    widgets.cpp \
    qtcolorpicker.cpp \
    record.cpp \
    actiongroup.cpp \
    rootmenu.cpp \
    app.cpp \
    mplayer-main.c \
    videooutput.cpp \
    prefdialog.cpp \
    richtexthelper.cpp \
    richtextblock.cpp \
    richtextdocument.cpp \
    mpmessage.cpp \
    mplayer-vd_ffmpeg.c \
    hwaccel.cpp \
    playengine.cpp \
    videorendereritem.cpp \
    playinfoitem.cpp \
    texturerendereritem.cpp \
    subtitlerendereritem.cpp \
    playeritem.cpp

HEADERS += skin.hpp
SOURCES += skin.cpp

TRANSLATIONS += translations/cmplayer_ko.ts \
    translations/cmplayer_en.ts \
    translations/cmplayer_ru.ts

FORMS += \
    ui/aboutdialog.ui \
    ui/opendvddialog.ui \
    ui/snapshotdialog.ui \
    ui/prefdialog.ui

OTHER_FILES += \
    test.qml \
    qml/TextOsd.qml \
    qml/Osd.qml \
    qml/ProgressOsd.qml \
    qml/PlayInfoOsd.qml

