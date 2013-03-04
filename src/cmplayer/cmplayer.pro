TEMPLATE = app
CONFIG += link_pkgconfig debug_and_release precompile_header c++11
macx:CONFIG -= app_bundle
!isEmpty(RELEASE): CONFIG += release; macx:CONFIG += app_bundle
QT = core gui network quick widgets
PRECOMPILED_HEADER = stdafx.hpp
precompile_header:!isEmpty(PRECOMPILED_HEADER): DEFINES += USING_PCH
DESTDIR = ../../build
LIB_DIR = $${DESTDIR}/lib
INCLUDEPATH += ../mpv ../../build/include
LIBS += -L$${LIB_DIR}

macx {
    QMAKE_CXXFLAGS -= "-mmacosx-version-min=10.6"
    QMAKE_CXX = clang++ -std=c++11 -stdlib=libc++
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
    QMAKE_MAC_SDK = /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX10.7.sdk
    QMAKE_INFO_PLIST = Info.plist
    ICON = ../../icons/cmplayer.icns
    TARGET = CMPlayer
    BREW = /usr/local/Cellar
    LLIB_DIR = /usr/local/lib
    LIBS +=  $${LIB_DIR}/libavcodec.a $${LIB_DIR}/libavformat.a $${LIB_DIR}/libavutil.a \
        $${LIB_DIR}/libswscale.a $${LIB_DIR}/libcmplayer_mpv.a $${LIB_DIR}/libchardet.a \
        $${LLIB_DIR}/libmpg123.a $${LLIB_DIR}/libquvi.a $${LLIB_DIR}/liblua52.a \
        $${LLIB_DIR}/libdvdread.a $${LLIB_DIR}/libcdio.a $${LLIB_DIR}/libcdio_paranoia.a \
        $${LLIB_DIR}/libcdio_cdda.a $${LLIB_DIR}/libdvdcss.a -lcurl -liconv \
        -framework VideoDecodeAcceleration -framework CoreVideo -framework Cocoa \
        -framework CoreFoundation -framework AudioUnit -framework CoreAudio -framework OpenAL \
	-framework IOKit -framework Carbon
    HEADERS += app_mac.hpp
    OBJECTIVE_SOURCES += app_mac.mm
    INCLUDEPATH += /opt/local/include /usr/local/include
} else:unix {
    QT += dbus gui-private
    QMAKE_CC = "gcc -std=c99 -w"
    QMAKE_CXXFLAGS += -std=c++11
    TARGET = cmplayer
    LIBS += -lX11 -lxcb -lva -lva-glx -lchardet -lcmplayer_mpv -lswscale -lavcodec -lavformat -lavutil \
        -lmpg123 -lquvi -ldvdread -lcdio -lcdio_paranoia -lcdio_cdda -lopenal -lasound -ldl
    HEADERS += app_x11.hpp
    SOURCES += app_x11.cpp
}

LIBS += -lbz2 -lz

QML_IMPORT_PATH += imports

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
    stdafx.hpp \
    videorendereritem.hpp \
    playinfoitem.hpp \
    texturerendereritem.hpp \
    subtitlerendereritem.hpp \
    playeritem.hpp \
    videoformat.hpp \
    mposditem.hpp \
    globalqmlobject.hpp \
    historymodel.hpp \
    shadervar.h \
    hwacc.hpp \
    subtitlestyle.hpp \
    audiocontroller.hpp \
    subtitledrawer.hpp \
    skin.hpp

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
    playengine.cpp \
    videorendereritem.cpp \
    playinfoitem.cpp \
    texturerendereritem.cpp \
    subtitlerendereritem.cpp \
    playeritem.cpp \
    mposditem.cpp \
    globalqmlobject.cpp \
    historymodel.cpp \
    mpv-main.c \
    shadervar.cpp \
    subtitlestyle.cpp \
    hwacc.cpp \
    videoformat.cpp \
    audiocontroller.cpp \
    subtitledrawer.cpp \
    skin.cpp

TRANSLATIONS += translations/cmplayer_ko.ts \
    translations/cmplayer_en.ts \
    translations/cmplayer_ru.ts

FORMS += \
    ui/aboutdialog.ui \
    ui/opendvddialog.ui \
    ui/snapshotdialog.ui \
    ui/prefdialog.ui

OTHER_FILES += \
    imports/CMPlayerSkin/qmldir \
    imports/CMPlayerSkin/TextOsd.qml \
    imports/CMPlayerSkin/ScrollBar.qml \
    imports/CMPlayerSkin/ProgressOsd.qml \
    imports/CMPlayerSkin/PlaylistView.qml \
    imports/CMPlayerSkin/Osd.qml \
    imports/CMPlayerSkin/Logo.qml \
    imports/CMPlayerSkin/HistoryView.qml \
    imports/CMPlayerSkin/FlickableListView.qml \
    imports/CMPlayerSkin/ColumnHeader.qml \
    skins/simple/Slider.qml \
    skins/simple/cmplayer.qml \
    imports/CMPlayerSkin/PlaylistDock.qml \
    imports/CMPlayerSkin/HistoryDock.qml \
    imports/CMPlayerSkin/HorizontalLayout.qml \
    imports/CMPlayerSkin/Button.qml \
    imports/CMPlayerSkin/Player.qml \
    imports/CMPlayerSkin/MouseCatcher.qml \
    imports/CMPlayerSkin/TimeText.qml \
    skins/classic/Slider.qml \
    skins/classic/FramedButton.qml \
    skins/classic/cmplayer.qml \
    skins/modern/cmplayer.qml \
    skins/modern/Slider.qml \
    imports/CMPlayerSkin/PlayInfoView.qml \
    emptyskin.qml \
    imports/CMPlayerSkin/AppWithFloating.qml

evil_hack_to_fool_lupdate {
SOURCES += $${OTHER_FILES}
}
