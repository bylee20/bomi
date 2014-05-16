TEMPLATE = app
CONFIG += link_pkgconfig debug_and_release precompile_header c++11 object_parallel_to_source
macx:CONFIG -= app_bundle

QT = core gui network quick widgets sql xml
PRECOMPILED_HEADER = stdafx.hpp
precompile_header:!isEmpty(PRECOMPILED_HEADER): DEFINES += USING_PCH
DESTDIR = ../../build
LIB_DIR = $${DESTDIR}/lib
INCLUDEPATH += ../mpv ../mpv/build
LIBS += -L$${LIB_DIR} ../../build/lib/libmpv.a -lbz2 -lz

include(configure.pro)
contains( DEFINES, CMPLAYER_RELEASE ) {
    CONFIG += release
    macx:CONFIG += app_bundle
} else:CONFIG -= release

macx {
    QT += macextras
    CONFIG += sdk
    QMAKE_INFO_PLIST = Info.plist
    ICON = ../../icons/cmplayer.icns
    TARGET = CMPlayer
    LIBS += -liconv -framework Cocoa -framework QuartzCore -framework IOKit \
	-framework IOSurface -framework Carbon -framework AudioUnit -framework CoreAudio \
	-framework VideoDecodeAcceleration -framework AudioToolBox
    HEADERS += app_mac.hpp
    OBJECTIVE_SOURCES += app_mac.mm
    INCLUDEPATH += ../ffmpeg ../ffmpeg/libavcodec
} else:unix {
    QT += dbus x11extras
    TARGET = cmplayer
    LIBS += -ldl
	HEADERS += app_x11.hpp misc/mpris.hpp
	SOURCES += app_x11.cpp misc/mpris.cpp
}

QML_IMPORT_PATH += imports

DEFINES += _LARGEFILE_SOURCE "_FILE_OFFSET_BITS=64" _LARGEFILE64_SOURCE

RESOURCES += rsclist.qrc

HEADERS += \
	audio/audiocontroller.hpp \
	audio/channelmanipulation.hpp \
	audio/audiomixer.hpp \
	audio/audio_helper.hpp \
	video/videoframe.hpp \
	video/videooutput.hpp \
	video/videoformat.hpp \
	video/hwacc.hpp \
	video/hwacc_vaapi.hpp \
	video/hwacc_vdpau.hpp \
	video/hwacc_vda.hpp \
	video/videofilter.hpp \
	video/deintinfo.hpp \
	video/letterboxitem.hpp \
	video/ffmpegfilters.hpp \
	video/softwaredeinterlacer.hpp \
	video/vaapipostprocessor.hpp \
	video/videoframeshader.glsl.hpp \
	video/videoframeshader.hpp \
	video/videocolor.hpp \
	video/hwacc_helper.hpp \
	video/videorendereritem.hpp \
	video/mposditem.hpp \
	video/mposdbitmap.hpp \
	subtitle/subtitle.hpp \
	subtitle/subtitle_parser.hpp \
	subtitle/subtitleview.hpp \
	subtitle/subtitlemodel.hpp \
	subtitle/subtitle_parser_p.hpp \
	subtitle/richtexthelper.hpp \
	subtitle/richtextblock.hpp \
	subtitle/richtextdocument.hpp \
	subtitle/subtitlerendereritem.hpp \
	subtitle/subtitlestyle.hpp \
	subtitle/subtitledrawer.hpp \
	subtitle/subtitlerenderingthread.hpp \
	subtitle/submisc.hpp \
	quick/highqualitytextureitem.hpp \
	quick/busyiconitem.hpp \
	quick/toplevelitem.hpp \
	quick/itemwrapper.hpp \
	quick/buttonboxitem.hpp \
	quick/appobject.hpp \
	quick/settingsobject.hpp \
	quick/opengldrawitem.hpp \
	quick/simpletextureitem.hpp \
	quick/simplevertexitem.hpp \
	quick/simplefboitem.hpp \
	opengl/openglcompat.hpp \
	opengl/interpolator.hpp \
	opengl/openglmisc.hpp \
	opengl/openglvertex.hpp \
	playengine.hpp \
    mainwindow.hpp \
    mrl.hpp \
    global.hpp \
    menu.hpp \
    translator.hpp \
    pref.hpp \
	info.hpp \
    charsetdetector.hpp \
    abrepeater.hpp \
    playlist.hpp \
    playlistmodel.hpp \
    recentinfo.hpp \
    appstate.hpp \
    favoritesview.hpp \
	misc/downloader.hpp \
    enums.hpp \
	dialog/snapshotdialog.hpp \
    record.hpp \
    actiongroup.hpp \
    rootmenu.hpp \
    app.hpp \
	globalqmlobject.hpp \
	dialog/prefdialog.hpp \
    stdafx.hpp \
    skin.hpp \
	misc/dataevent.hpp \
	dialog/openmediafolderdialog.hpp \
	tmp.hpp \
	undostack.hpp \
	quick/geometryitem.hpp \
    playengine_p.hpp \
    mediamisc.hpp \
	trayicon.hpp \
    mrlstate.hpp \
    historymodel.hpp \
	misc/xmlrpcclient.hpp \
	subtitle/opensubtitlesfinder.hpp \
	dialog/subtitlefinddialog.hpp \
	misc/simplelistmodel.hpp \
    mrlstate_old.hpp \
    log.hpp \
    mpv_helper.hpp \
	misc/udf25.hpp \
    audio/audionormalizeroption.hpp \
    video/videoimagepool.hpp \
    opengl/opengloffscreencontext.hpp \
	widget/simplelistwidget.hpp \
	widget/qtcolorpicker.hpp \
    video/videoframebufferobject.hpp \
    mainquickview.hpp \
	quick/osdtheme.hpp \
    quick/themeobject.hpp \
	widget/mouseactiongroupbox.hpp \
    misc/keymodifieractionmap.hpp \
    dialog/bbox.hpp \
    dialog/mbox.hpp \
    dialog/shortcutdialog.hpp \
    dialog/encodingfiledialog.hpp \
    widget/encodingcombobox.hpp \
    widget/colorselectwidget.hpp \
    dialog/urldialog.hpp \
    widget/enumcombobox.hpp \
    widget/channelmanipulationwidget.hpp \
    widget/datacombobox.hpp \
    widget/verticallabel.hpp \
    widget/localecombobox.hpp \
    widget/fontoptionwidget.hpp \
    dialog/aboutdialog.hpp \
    dialog/opendiscdialog.hpp

SOURCES += \
	audio/audiocontroller.cpp \
	audio/channelmanipulation.cpp \
	audio/audiomixer.cpp \
	audio/audio_helper.cpp \
	video/videoframe.cpp \
	video/videooutput.cpp \
	video/videorendereritem.cpp \
	video/hwacc.cpp \
	video/videoformat.cpp \
	video/hwacc_vaapi.cpp \
	video/hwacc_vdpau.cpp \
	video/hwacc_vda.cpp \
	video/videofilter.cpp \
	video/deintinfo.cpp \
	video/letterboxitem.cpp \
	video/videoframeshader.cpp \
	video/ffmpegfilters.cpp \
	video/softwaredeinterlacer.cpp \
	video/vaapipostprocessor.cpp \
	video/videocolor.cpp \
	video/hwacc_helper.cpp \
	video/mposditem.cpp \
	video/mposdbitmap.cpp \
	subtitle/subtitle.cpp \
	subtitle/subtitle_parser.cpp \
	subtitle/subtitleview.cpp \
	subtitle/subtitlemodel.cpp \
	subtitle/subtitle_parser_p.cpp \
	subtitle/richtexthelper.cpp \
	subtitle/richtextblock.cpp \
	subtitle/richtextdocument.cpp \
	subtitle/subtitlerendereritem.cpp \
	subtitle/subtitlestyle.cpp \
	subtitle/subtitledrawer.cpp \
	subtitle/subtitlerenderingthread.cpp \
	subtitle/submisc.cpp \
	quick/highqualitytextureitem.cpp \
	quick/geometryitem.cpp \
	quick/busyiconitem.cpp \
	quick/toplevelitem.cpp \
	quick/itemwrapper.cpp \
	quick/buttonboxitem.cpp \
	quick/appobject.cpp \
	quick/settingsobject.cpp \
	quick/opengldrawitem.cpp \
	quick/simpletextureitem.cpp \
	quick/simplevertexitem.cpp \
	quick/simplefboitem.cpp \
	opengl/openglcompat.cpp \
	opengl/interpolator.cpp \
	opengl/openglmisc.cpp \
	opengl/openglvertex.cpp \
	main.cpp \
    mainwindow.cpp \
    mrl.cpp \
    global.cpp \
    menu.cpp \
    translator.cpp \
    pref.cpp \
    info.cpp \
	misc/charsetdetector.cpp \
    abrepeater.cpp \
    playlist.cpp \
    playlistmodel.cpp \
    recentinfo.cpp \
	widget/simplelistwidget.cpp \
    appstate.cpp \
    favoritesview.cpp \
	misc/downloader.cpp \
    enums.cpp \
	dialog/snapshotdialog.cpp \
	widget/qtcolorpicker.cpp \
    record.cpp \
    actiongroup.cpp \
    rootmenu.cpp \
    app.cpp \
	dialog/prefdialog.cpp \
    playengine.cpp \
    globalqmlobject.cpp \
    skin.cpp \
	dialog/openmediafolderdialog.cpp \
    undostack.cpp \
    mediamisc.cpp \
    trayicon.cpp \
    mrlstate.cpp \
    historymodel.cpp \
	misc/xmlrpcclient.cpp \
	subtitle/opensubtitlesfinder.cpp \
	dialog/subtitlefinddialog.cpp \
	misc/simplelistmodel.cpp \
    mrlstate_old.cpp \
    log.cpp \
    mpv_helper.cpp \
	misc/udf25.cpp \
    audio/audionormalizeroption.cpp \
    video/videoimagepool.cpp \
    opengl/opengloffscreencontext.cpp \
    video/videoframebufferobject.cpp \
    mainquickview.cpp \
	quick/osdtheme.cpp \
    quick/themeobject.cpp \
	widget/mouseactiongroupbox.cpp \
    misc/keymodifieractionmap.cpp \
    dialog/bbox.cpp \
    dialog/mbox.cpp \
    dialog/shortcutdialog.cpp \
    dialog/encodingfiledialog.cpp \
    widget/encodingcombobox.cpp \
    widget/colorselectwidget.cpp \
    dialog/urldialog.cpp \
    widget/enumcombobox.cpp \
    widget/channelmanipulationwidget.cpp \
    widget/datacombobox.cpp \
    widget/verticallabel.cpp \
    widget/localecombobox.cpp \
    widget/fontoptionwidget.cpp \
    dialog/aboutdialog.cpp \
    dialog/opendiscdialog.cpp


TRANSLATIONS += translations/cmplayer_ko.ts \
    translations/cmplayer_en.ts \
    translations/cmplayer_de.ts \
    translations/cmplayer_ru.ts \
    translations/cmplayer_it.ts \
    translations/cmplayer_cs.ts \
    translations/cmplayer_sr.ts \
    translations/cmplayer_es.ts

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
    imports/CMPlayer/ScrollBar.qml \
    imports/CMPlayer/MessageBox.qml \
    imports/CMPlayer/ProgressBar.qml \
    skins/Faenza-Zukitwo/cmplayer.qml

evil_hack_to_fool_lupdate {
SOURCES += $${OTHER_FILES}
}
