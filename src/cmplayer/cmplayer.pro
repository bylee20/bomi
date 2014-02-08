TEMPLATE = app
CONFIG += link_pkgconfig debug_and_release precompile_header c++11
macx:CONFIG -= app_bundle

!isEmpty(RELEASE) {
    DEFINES += CMPLAYER_RELEASE
    CONFIG += release
    macx:CONFIG += app_bundle
} else {
    isEmpty(LIBQUVI_SUFFIX): LIBQUVI_SUFFIX = $$system(if `pkg-config --exists libquvi-0.9`; then echo "-0.9"; fi)
}

QT = core gui network quick widgets sql xml
PRECOMPILED_HEADER = stdafx.hpp
precompile_header:!isEmpty(PRECOMPILED_HEADER): DEFINES += USING_PCH
DESTDIR = ../../build
LIB_DIR = $${DESTDIR}/lib
INCLUDEPATH += ../mpv ../mpv/build $${DESTDIR}/include
LIBS += -L$${LIB_DIR} -lchardet -lcmplayer_mpv -lbz2 -lz

PKGCONFIG += dvdread dvdnav libswresample libswscale libavfilter libavcodec libpostproc libavformat libavutil \
    libmpg123 libass libquvi$${LIBQUVI_SUFFIX} icu-uc
HAVE_PORTAUDIO = $$system(if `pkg-config --exists libcdio_paranoia`; then echo "yes"; fi)
!isEmpty(HAVE_PORTAUDIO) {
    PKGCONFIG += $$system(if `pkg-config --exists libcdio_paranoia`; then echo "libcdio_paranoia libcdio libcdio_cdda"; fi)
    DEFINES += HAVE_PORTAUDIO=1
}

PKGCONFIG += $$system(if `pkg-config --exists portaudio-2.0`; then echo "portaudio-2.0"; fi)

macx {
    QT += macextras
    CONFIG += sdk
    QT_CONFIG -= no-pkg-config
    QMAKE_INFO_PLIST = Info.plist
    ICON = ../../icons/cmplayer.icns
    TARGET = CMPlayer
    LIBS += -liconv -framework Cocoa -framework CoreVideo -framework IOKit \
	-framework IOSurface -framework Carbon -framework AudioUnit -framework CoreAudio \
	-framework VideoDecodeAcceleration -framework AudioToolBox
    HEADERS += app_mac.hpp
    OBJECTIVE_SOURCES += app_mac.mm
    INCLUDEPATH += ../ffmpeg ../ffmpeg/libavcodec
    LIBS -= -L/opt/X11/lib
} else:unix {
    QT += dbus x11extras
    QMAKE_CC = "gcc -std=c99 -w"
    QMAKE_CXXFLAGS += -std=c++11
    PKGCONFIG += glib-2.0 libva libva-glx libva-x11 xcb xcb-icccm x11 alsa openal libpulse gobject-2.0 jack
    TARGET = cmplayer
    LIBS += -ldl
    HEADERS += app_x11.hpp
    SOURCES += app_x11.cpp
}

QML_IMPORT_PATH += imports

DEFINES += _LARGEFILE_SOURCE "_FILE_OFFSET_BITS=64" _LARGEFILE64_SOURCE

RESOURCES += rsclist.qrc
HEADERS += playengine.hpp \
    mainwindow.hpp \
    mrl.hpp \
    global.hpp \
    menu.hpp \
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
    simplelistwidget.hpp \
    appstate.hpp \
    dialogs.hpp \
    favoritesview.hpp \
    downloader.hpp \
    subtitlemodel.hpp \
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
    prefdialog.hpp \
    richtexthelper.hpp \
    richtextblock.hpp \
    richtextdocument.hpp \
    stdafx.hpp \
    videorendereritem.hpp \
    texturerendereritem.hpp \
    subtitlerendereritem.hpp \
    videoformat.hpp \
    mposditem.hpp \
    globalqmlobject.hpp \
    hwacc.hpp \
    subtitlestyle.hpp \
    audiocontroller.hpp \
    subtitledrawer.hpp \
    skin.hpp \
    subtitlerenderingthread.hpp \
    dataevent.hpp \
    openmediafolderdialog.hpp \
    hwacc_vaapi.hpp \
    hwacc_vdpau.hpp \
    hwacc_vda.hpp \
    tmp.hpp \
    videofilter.hpp \
    deintinfo.hpp \
    letterboxitem.hpp \
    mposdbitmap.hpp \
    openglcompat.hpp \
    videoframeshader.glsl.hpp \
    videoframeshader.hpp \
    undostack.hpp \
    ffmpegfilters.hpp \
    softwaredeinterlacer.hpp \
    vaapipostprocessor.hpp \
    texturenode.hpp \
    geometryitem.hpp \
    playengine_p.hpp \
    mediamisc.hpp \
    ../mpv/video/out/dither.h \
    trayicon.hpp \
    videocolor.hpp \
    interpolator.hpp \
    channelmanipulation.hpp \
    mrlstate.hpp \
    submisc.hpp \
    historymodel.hpp \
    xmlrpcclient.hpp \
    opensubtitlesfinder.hpp \
    subtitlefinddialog.hpp \
    simplelistmodel.hpp \
    audiomixer.hpp \
    audiomixer_p.hpp

SOURCES += main.cpp \
    mainwindow.cpp \
    mrl.cpp \
    global.cpp \
    menu.cpp \
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
    simplelistwidget.cpp \
    appstate.cpp \
    dialogs.cpp \
    favoritesview.cpp \
    downloader.cpp \
    subtitlemodel.cpp \
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
    playengine.cpp \
    videorendereritem.cpp \
    texturerendereritem.cpp \
    subtitlerendereritem.cpp \
    mposditem.cpp \
    globalqmlobject.cpp \
    subtitlestyle.cpp hwacc.cpp \
    videoformat.cpp \
    audiocontroller.cpp \
    subtitledrawer.cpp \
    skin.cpp \
    subtitlerenderingthread.cpp \
    openmediafolderdialog.cpp \
    hwacc_vaapi.cpp \
    hwacc_vdpau.cpp \
    hwacc_vda.cpp \
    videofilter.cpp \
    deintinfo.cpp \
    letterboxitem.cpp \
    mposdbitmap.cpp \
    openglcompat.cpp \
    videoframeshader.cpp \
    undostack.cpp \
    ffmpegfilters.cpp \
    softwaredeinterlacer.cpp \
    vaapipostprocessor.cpp \
    texturenode.cpp \
    geometryitem.cpp \
    mediamisc.cpp \
    ../mpv/video/out/dither.c \
    trayicon.cpp \
    videocolor.cpp \
    interpolator.cpp \
    channelmanipulation.cpp \
    mrlstate.cpp \
    historymodel.cpp \
    submisc.cpp \
    xmlrpcclient.cpp \
    opensubtitlesfinder.cpp \
    subtitlefinddialog.cpp \
    simplelistmodel.cpp \
    audiomixer.cpp

TRANSLATIONS += translations/cmplayer_ko.ts \
    translations/cmplayer_en.ts \
    translations/cmplayer_de.ts \
    translations/cmplayer_ru.ts \
    translations/cmplayer_it.ts \
    translations/cmplayer_cs.ts \
    translations/cmplayer_sr.ts

FORMS += \
    ui/aboutdialog.ui \
    ui/opendvddialog.ui \
    ui/snapshotdialog.ui \
    ui/prefdialog.ui \
    ui/openmediafolderdialog.ui \
    ui/subtitlefinddialog.ui

OTHER_FILES += \
    imports/CMPlayer/qmldir \
    imports/CMPlayer/TextOsd.qml \
    imports/CMPlayer/ProgressOsd.qml \
    imports/CMPlayer/PlayInfoView.qml \
    imports/CMPlayer/Osd.qml \
    imports/CMPlayer/Logo.qml \
    skins/simple/cmplayer.qml \
    imports/CMPlayer/PlaylistDock.qml \
    imports/CMPlayer/HistoryDock.qml \
    imports/CMPlayer/Button.qml \
    imports/CMPlayer/Player.qml \
    imports/CMPlayer/TimeText.qml \
    skins/classic/FramedButton.qml \
    skins/classic/cmplayer.qml \
    skins/modern/cmplayer.qml \
    emptyskin.qml \
    imports/CMPlayer/AppWithFloating.qml \
    imports/CMPlayer/AppWithDock.qml \
    imports/CMPlayer/TimeSlider.qml \
    imports/CMPlayer/VolumeSlider.qml \
    imports/CMPlayer/PlayInfoText.qml \
    skins/GaN/cmplayer.qml \
    skins/GaN/TimeText.qml \
    skins/GaN/Button.qml \
    imports/CMPlayer/ItemColumn.qml \
    imports/CMPlayer/ModelView.qml \
    imports/CMPlayer/ScrollBar.qml

evil_hack_to_fool_lupdate {
SOURCES += $${OTHER_FILES}
}
