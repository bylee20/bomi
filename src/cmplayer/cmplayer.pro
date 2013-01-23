TEMPLATE = app
CONFIG += link_pkgconfig debug_and_release precompile_header
QT = core gui network quick widgets gui-private

#LIBS +=  -lz -lbz2 -lpthread -lm -ldvdread -lmad -lvorbis -logg -lfaad -ldv -ldvdnavmini \
#    -lxvidcore -lvorbis -logg -ltheora -la52 -ldca -lcdio_paranoia -lcdio_cdda -lcdio -lquvi

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
	../../build/lib/libavformat.a ../../build/lib/libavutil.a \
	../../build/lib/libswscale.a ../../build/lib/libcmplayer_mpv.a \
        ../../build/lib/libchardet.a ../../build/lib/libcmplayer_sigar.a \
        -L/opt/local/lib \
        -framework VideoDecodeAcceleration -framework CoreVideo -framework Cocoa \
        -framework CoreFoundation -framework AudioUnit -framework CoreAudio -framework OpenAL \
        -framework IOKit -framework Carbon
    HEADERS += app_mac.hpp
    OBJECTIVE_SOURCES += app_mac.mm
    INCLUDEPATH += /opt/local/include /usr/local/include
} else:unix {
    QT += dbus
    TARGET = cmplayer
    LIBS += -lX11 -lxcb \
        -L../../build/lib -lcmplayer_mpv -lcmplayer_sigar -lchardet -lcmplayer_av \
        -lopenal -lasound -ldl -lva -lva-x11
    HEADERS += app_x11.hpp
    SOURCES += app_x11.cpp
}
LIBS += -lz -lbz2 -lmpg123 -lquvi -lpthread -lm -ldvdread -lmad -lfaad -la52 -ldca -lcdio_paranoia -lcdio_cdda -lcdio

INCLUDEPATH += ../mpv ../../build/include ../sigar/include

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
    subtitle.hpp \
    subtitle_parser.hpp \
    subtitlerenderer.hpp \
    info.hpp \
    charsetdetector.hpp \
    abrepeater.hpp \
    playlist.hpp \
    playlistmodel.hpp \
    playlistview.hpp \
    recentinfo.hpp \
    subtitleview.hpp \
    osdstyle.hpp \
    simplelistwidget.hpp \
    appstate.hpp \
    dialogs.hpp \
    favoritesview.hpp \
    downloader.hpp \
    subtitlemodel.hpp \
    tagiterator.hpp \
    subtitle_parser_p.hpp \
    enums.hpp \
    snapshotdialog.hpp \
    listmodel.hpp \
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
    playeritem.hpp \
    videoformat.hpp \
    qwindowwidget.hpp \
    mposditem.hpp \
    globalqmlobject.hpp \
    historymodel.hpp \
    shadervar.h \
    subtitlestyle.h

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
    subtitle.cpp \
    subtitle_parser.cpp \
    subtitlerenderer.cpp \
    info.cpp \
    charsetdetector.cpp \
    abrepeater.cpp \
    playlist.cpp \
    playlistmodel.cpp \
    playlistview.cpp \
    recentinfo.cpp \
    subtitleview.cpp \
    osdstyle.cpp \
    simplelistwidget.cpp \
    appstate.cpp \
    dialogs.cpp \
    favoritesview.cpp \
    downloader.cpp \
    subtitlemodel.cpp \
    tagiterator.cpp \
    subtitle_parser_p.cpp \
    enums.cpp \
    snapshotdialog.cpp \
    listmodel.cpp \
    widgets.cpp \
    qtcolorpicker.cpp \
    record.cpp \
    actiongroup.cpp \
    rootmenu.cpp \
    app.cpp \
    videooutput.cpp \
    prefdialog.cpp \
    richtexthelper.cpp \
    richtextblock.cpp \
    richtextdocument.cpp \
    mpmessage.cpp \
    hwaccel.cpp \
    playengine.cpp \
    videorendereritem.cpp \
    playinfoitem.cpp \
    texturerendereritem.cpp \
    subtitlerendereritem.cpp \
    playeritem.cpp \
    qwindowwidget.cpp \
    mposditem.cpp \
    globalqmlobject.cpp \
    historymodel.cpp \
    mpv-vd.c \
    mpv-main.c \
    shadervar.cpp \
    subtitlestyle.cpp

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
    skins/simple/qml/TextOsd.qml \
    skins/simple/qml/Osd.qml \
    skins/simple/qml/ProgressOsd.qml \
    skins/simple/qml/PlayInfoOsd.qml \
    skins/simple/qml/Slider.qml \
    skins/simple/qml/Logo.qml \
    skins/classic/qml/TextOsd.qml \
    skins/classic/qml/Slider.qml \
    skins/classic/qml/ProgressOsd.qml \
    skins/classic/qml/PlayInfoOsd.qml \
    skins/classic/qml/Osd.qml \
    skins/classic/qml/Logo.qml \
    skins/classic/qml/ImageButton.qml \
    skins/classic/cmplayer.qml \
    skins/simple/cmplayer.qml \
    skins/simple/PlaylistView.qml \
    skins/simple/qml/ScrollBar.qml \
    skins/simple/HistoryView.qml \
    skins/simple/qml/FlickableListView.qml \
    skins/simple/qml/ColumnHeader.qml

