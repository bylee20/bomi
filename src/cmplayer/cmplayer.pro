TARGET = cmplayer

!isEmpty(RELEASE) {
	CONFIG += release
	macx {
		CONFIG += app_bundle
	}
} else {
	CONFIG += debug
	macx {
		CONFIG -= app_bundle
	}
}

macx {
	QMAKE_CXXFLAGS += -std=c++0x
	QMAKE_CXX = /opt/local/bin/g++-mp-4.6
	#LIBS += -L/opt/local/lib/gcc46 -lstdc++
	#INCLUDEPATH += /opt/local/include/gcc46/c++/x86_64-apple-darwin10
	#INCLUDEPATH += /opt/local/include/gcc46/c++


	QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.5
	#QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.5.sdk
	#INCLUDEPATH += /usr/include/c++/4.2.1
	isEmpty(VLC_INCLUDE_PATH) {
		VLC_INCLUDE_PATH = /Applications/VLC.app/Contents/MacOS/include
	}
	isEmpty(VLC_LIB_PATH) {
		VLC_LIB_PATH = /Applications/VLC.app/Contents/MacOS/lib
	}
	ICON = ../../icons/cmplayer.icns
	LIBS += -framework Cocoa -framework IOKit

	QMAKE_INFO_PLIST = Info.plist
	TARGET = CMPlayer
	HEADERS += application_mac.hpp
	SOURCES += application_mac.mm

} else:unix {
	LIBS += -lX11
        HEADERS += application_x11.hpp
        SOURCES += application_x11.cpp
}

TEMPLATE = app
CONFIG += link_pkgconfig debug_and_release

QT = core gui opengl network

INCLUDEPATH += ../libchardet-1.0.1/src

LIBS += -lvlc -L../libchardet-1.0.1/src/.libs -lchardet

!isEmpty(VLC_INCLUDE_PATH) {
	INCLUDEPATH += $${VLC_INCLUDE_PATH}
}
!isEmpty(VLC_LIB_PATH) {
	LIBS += -L$${VLC_LIB_PATH}
}

RESOURCES += rsclist.qrc
HEADERS += playengine.hpp \
    mainwindow.hpp \
    mrl.hpp \
    controlwidget.hpp \
    global.hpp \
    menu.hpp \
    squeezedlabel.hpp \
    colorproperty.hpp \
    qtsingleapplication/qtsingleapplication.h \
    qtsingleapplication/qtlockedfile.h \
    qtsingleapplication/qtlocalpeer.h \
    qtsingleapplication/qtsinglecoreapplication.h \
    application.hpp \
    translator.hpp \
    pref.hpp \
    videoframe.hpp \
    osdrenderer.hpp \
    subtitle.hpp \
    richstring.hpp \
    subtitle_parser.hpp \
    subtitlerenderer.hpp \
    textosdrenderer.hpp \
    timelineosdrenderer.hpp \
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
    pref_dialog.hpp \
    osdstyle.hpp \
    simplelistwidget.hpp \
    appstate.hpp \
    dialogs.hpp \
    favoritesview.hpp \
    downloader.hpp \
    logodrawer.hpp \
    libvlc.hpp \
    vlcmedia.hpp \
    framebufferobjectoverlay.hpp \
    pixelbufferoverlay.hpp \
    pixmapoverlay.hpp \
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
    fragmentprogram.hpp \
    i420_to_rgb_simple.hpp \
    i420_to_rgb_filter.hpp \
    i420_to_rgb_kernel.hpp \
    mainwindow_p.hpp \
    pref_widget.hpp \
    playinfoview.hpp \
    widgets.hpp \
    qtcolorpicker.hpp \
    richtext.hpp \
    record.hpp \
    actiongroup.hpp \
    rootmenu.hpp
SOURCES += main.cpp \
    playengine.cpp \
    mainwindow.cpp \
    mrl.cpp \
    controlwidget.cpp \
    global.cpp \
    menu.cpp \
    colorproperty.cpp \
    qtsingleapplication/qtsingleapplication.cpp \
    qtsingleapplication/qtlockedfile_win.cpp \
    qtsingleapplication/qtlockedfile_unix.cpp \
    qtsingleapplication/qtlockedfile.cpp \
    qtsingleapplication/qtlocalpeer.cpp \
    qtsingleapplication/qtsinglecoreapplication.cpp \
    application.cpp \
    translator.cpp \
    pref.cpp \
    videoframe.cpp \
    osdrenderer.cpp \
    subtitle.cpp \
    richstring.cpp \
    subtitle_parser.cpp \
    subtitlerenderer.cpp \
    textosdrenderer.cpp \
    timelineosdrenderer.cpp \
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
    pref_dialog.cpp \
    osdstyle.cpp \
    simplelistwidget.cpp \
    appstate.cpp \
    dialogs.cpp \
    favoritesview.cpp \
    downloader.cpp \
    libvlc.cpp \
    vlcmedia.cpp \
    logodrawer.cpp \
    framebufferobjectoverlay.cpp \
    pixelbufferoverlay.cpp \
    pixmapoverlay.cpp \
    overlay.cpp \
    videorenderer.cpp \     
    subtitlemodel.cpp \
    tagiterator.cpp \
    subtitle_parser_p.cpp \
    enums.cpp \
    snapshotdialog.cpp \
    events.cpp \
    listmodel.cpp \
    fragmentprogram.cpp \
    mainwindow_p.cpp \
    pref_widget.cpp \
    playinfoview.cpp \
    widgets.cpp \
    qtcolorpicker.cpp \
    richtext.cpp \
    record.cpp \
    actiongroup.cpp \
    rootmenu.cpp
TRANSLATIONS += translations/cmplayer_ko.ts \
    translations/cmplayer_en.ts \
    translations/cmplayer_ja.ts
FORMS += \
    ui/aboutdialog.ui \
    ui/opendvddialog.ui \
    ui/snapshotdialog.ui \
    ui/pref_widget.ui
