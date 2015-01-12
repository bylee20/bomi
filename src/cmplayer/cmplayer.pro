TEMPLATE = app
CONFIG += link_pkgconfig debug_and_release precompile_header \
	c++11 object_parallel_to_source
macx:CONFIG -= app_bundle

QT = core gui network quick widgets sql xml
PRECOMPILED_HEADER = stdafx.hpp
precompile_header:!isEmpty(PRECOMPILED_HEADER): DEFINES += USING_PCH
DESTDIR = ../../build
LIB_DIR = $${DESTDIR}/lib
INCLUDEPATH += ../mpv ../mpv/build
LIBS += -L$${LIB_DIR} -lbz2 -lz

include(configure.pro)
contains( DEFINES, CMPLAYER_RELEASE ) {
    CONFIG += release
    macx:CONFIG += app_bundle
} else:CONFIG -= release

QMAKE_CXXFLAGS_CXX11 = -std=c++1y

!contains(QMAKE_CXX, clang++) {
QMAKE_CXXFLAGS += -Wno-non-template-friend
}

!isEmpty(USE_CCACHE): QMAKE_CXX = ccache $${QMAKE_CXX}

macx {
    QT += macextras
    CONFIG += sdk
    QMAKE_INFO_PLIST = Info.plist
    ICON = ../../icons/cmplayer.icns
    TARGET = CMPlayer
    LIBS += -liconv -framework Cocoa -framework QuartzCore -framework IOKit \
		-framework IOSurface -framework Carbon -framework AudioUnit \
		-framework CoreAudio -framework VideoDecodeAcceleration \
		-framework AudioToolbox
	HEADERS += player/app_mac.hpp
	OBJECTIVE_SOURCES += player/app_mac.mm
    INCLUDEPATH += ../ffmpeg ../ffmpeg/libavcodec
} else:unix {
    QT += dbus x11extras
    TARGET = cmplayer
	LIBS += -ldl -lXrandr
	HEADERS += player/app_x11.hpp player/mpris.hpp
	SOURCES += player/app_x11.cpp player/mpris.cpp
}

QML_IMPORT_PATH += imports

DEFINES += _LARGEFILE_SOURCE "_FILE_OFFSET_BITS=64" _LARGEFILE64_SOURCE \
	QT_NO_CAST_FROM_ASCII

RESOURCES += rsclist.qrc

HEADERS += \
	stdafx.hpp \
	audio/audiocontroller.hpp \
	audio/channelmanipulation.hpp \
	audio/audiomixer.hpp \
	audio/audio_helper.hpp \
	audio/audionormalizeroption.hpp \
	video/videoformat.hpp \
	video/hwacc.hpp \
	video/videofilter.hpp \
	video/deintoption.hpp \
	video/letterboxitem.hpp \
	video/ffmpegfilters.hpp \
	video/softwaredeinterlacer.hpp \
	video/videocolor.hpp \
	video/deintcaps.hpp \
	video/kernel3x3.hpp \
	subtitle/subtitle.hpp \
	subtitle/subtitle_parser.hpp \
	subtitle/subtitleview.hpp \
	subtitle/subtitlemodel.hpp \
	subtitle/subtitle_parser_p.hpp \
	subtitle/richtexthelper.hpp \
	subtitle/richtextblock.hpp \
	subtitle/richtextdocument.hpp \
	subtitle/subtitlerendereritem.hpp \
	misc/osdtheme.hpp \
	subtitle/subtitledrawer.hpp \
	subtitle/subtitlerenderingthread.hpp \
	subtitle/submisc.hpp \
	subtitle/opensubtitlesfinder.hpp \
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
	quick/geometryitem.hpp \
	quick/osdthemeobject.hpp \
	quick/themeobject.hpp \
	quick/globalqmlobject.hpp \
	opengl/openglmisc.hpp \
	opengl/openglvertex.hpp \
	opengl/opengltexturebase.hpp \
	opengl/openglframebufferobject.hpp \
	opengl/opengltexture2d.hpp \
	opengl/opengltexture1d.hpp \
	opengl/opengltexturebinder.hpp \
	opengl/opengloffscreencontext.hpp \
	dialog/snapshotdialog.hpp \
	dialog/openmediafolderdialog.hpp \
	dialog/prefdialog.hpp \
	dialog/subtitlefinddialog.hpp \
	dialog/bbox.hpp \
	dialog/mbox.hpp \
	dialog/shortcutdialog.hpp \
	dialog/encodingfiledialog.hpp \
	dialog/urldialog.hpp \
	dialog/aboutdialog.hpp \
	dialog/opendiscdialog.hpp \
	widget/simplelistwidget.hpp \
	widget/qtcolorpicker.hpp \
	widget/encodingcombobox.hpp \
	widget/colorselectwidget.hpp \
	widget/enumcombobox.hpp \
	widget/channelmanipulationwidget.hpp \
	widget/datacombobox.hpp \
	widget/verticallabel.hpp \
	widget/localecombobox.hpp \
	widget/fontoptionwidget.hpp \
	widget/menu.hpp \
	widget/deintwidget.hpp \
	enum/audiodriver.hpp \
	enum/changevalue.hpp \
	enum/channellayout.hpp \
	enum/clippingmethod.hpp \
	enum/colorrange.hpp \
	enum/decoderdevice.hpp \
	enum/deintdevice.hpp \
	enum/deintmethod.hpp \
	enum/deintmode.hpp \
	enum/dithering.hpp \
	enum/enums.hpp \
	enum/generateplaylist.hpp \
	enum/horizontalalignment.hpp \
	enum/interpolator.hpp \
	enum/keymodifier.hpp \
	enum/movetoward.hpp \
	enum/osdscalepolicy.hpp \
	enum/seekingstep.hpp \
	enum/speakerid.hpp \
	enum/staysontop.hpp \
	enum/subtitleautoload.hpp \
	enum/subtitleautoselect.hpp \
	enum/subtitledisplay.hpp \
	enum/textthemestyle.hpp \
	enum/verticalalignment.hpp \
	enum/videoratio.hpp \
	enum/videoeffect.hpp \
	misc/charsetdetector.hpp \
	misc/downloader.hpp \
	misc/actiongroup.hpp \
	misc/dataevent.hpp \
	misc/xmlrpcclient.hpp \
	misc/simplelistmodel.hpp \
	misc/log.hpp \
	misc/udf25.hpp \
	misc/keymodifieractionmap.hpp \
	misc/enumaction.hpp \
	misc/stepaction.hpp \
	misc/record.hpp \
	misc/tmp.hpp \
	misc/undostack.hpp \
	misc/trayicon.hpp \
	player/app.hpp \
	player/mrl.hpp \
	player/pref.hpp \
	player/skin.hpp \
	player/appstate.hpp \
	player/playlist.hpp \
	player/rootmenu.hpp \
	player/mrlstate.hpp \
	player/mediamisc.hpp \
	player/playengine.hpp \
	player/mainwindow.hpp \
	player/translator.hpp \
	player/recentinfo.hpp \
	player/mpv_helper.hpp \
	player/playengine_p.hpp \
	player/mrlstate_old.hpp \
	player/historymodel.hpp \
	player/playlistmodel.hpp \
	player/mainquickview.hpp \
    audio/channellayoutmap.hpp \
    player/openmediainfo.hpp \
    enum/openmediabehavior.hpp \
    misc/json.hpp \
    misc/stepactionpair.hpp \
    opengl/opengltexturetransferinfo.hpp \
    opengl/opengllogger.hpp \
    enum/enumflags.hpp \
    misc/is_convertible.hpp \
    misc/jsonstorage.hpp \
    player/mrlstatesqlfield.hpp \
    tmp/algorithm.hpp \
    tmp/arithmetic_type.hpp \
    tmp/static_for.hpp \
    tmp/type_info.hpp \
    tmp/type_test.hpp \
    misc/localconnection.hpp \
    player/abrepeatchecker.hpp \
    widget/openmediabehaviorgroupbox.hpp \
    tmp/static_op.hpp \
    dialog/prefdialog_p.hpp \
    opengl/openglbenchmarker.hpp \
    enum/colorspace.hpp \
    player/mainwindow_p.hpp \
    enum/quicksnapshotsave.hpp \
    video/mpimage.hpp \
    enum/mousebehavior.hpp \
    misc/speedmeasure.hpp \
	player/avinfoobject.hpp \
    audio/audioformat.hpp \
    player/streamtrack.hpp \
    misc/locale.hpp \
    misc/matchstring.hpp \
    misc/simplelistdelegate.hpp \
    player/pref_helper.hpp \
    widget/prefwidgets.hpp \
    enum/colorenumdata.hpp \
    video/videorenderer.hpp \
    misc/youtubedl.hpp \
    quick/playlistthemeobject.hpp \
    misc/yledl.hpp

SOURCES += \
	stdafx.cpp \
	audio/audiocontroller.cpp \
	audio/channelmanipulation.cpp \
	audio/audiomixer.cpp \
	audio/audio_helper.cpp \
	audio/audionormalizeroption.cpp \
	video/hwacc.cpp \
	video/videoformat.cpp \
	video/videofilter.cpp \
	video/deintoption.cpp \
	video/letterboxitem.cpp \
	video/ffmpegfilters.cpp \
	video/softwaredeinterlacer.cpp \
	video/videocolor.cpp \
	video/deintcaps.cpp \
	video/kernel3x3.cpp \
	subtitle/subtitle.cpp \
	subtitle/subtitle_parser.cpp \
	subtitle/subtitleview.cpp \
	subtitle/subtitlemodel.cpp \
	subtitle/subtitle_parser_p.cpp \
	subtitle/richtexthelper.cpp \
	subtitle/richtextblock.cpp \
	subtitle/richtextdocument.cpp \
	subtitle/subtitlerendereritem.cpp \
	misc/osdtheme.cpp \
	subtitle/subtitledrawer.cpp \
	subtitle/subtitlerenderingthread.cpp \
	subtitle/submisc.cpp \
	subtitle/opensubtitlesfinder.cpp \
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
	quick/osdthemeobject.cpp \
	quick/themeobject.cpp \
	quick/globalqmlobject.cpp \
	opengl/openglmisc.cpp \
	opengl/openglvertex.cpp \
	opengl/opengltexturebase.cpp \
	opengl/openglframebufferobject.cpp \
	opengl/opengltexture2d.cpp \
	opengl/opengltexture1d.cpp \
	opengl/opengltexturebinder.cpp \
	opengl/opengloffscreencontext.cpp \
	dialog/snapshotdialog.cpp \
	dialog/prefdialog.cpp \
	dialog/bbox.cpp \
	dialog/mbox.cpp \
	dialog/shortcutdialog.cpp \
	dialog/encodingfiledialog.cpp \
	dialog/openmediafolderdialog.cpp \
	dialog/subtitlefinddialog.cpp \
	dialog/aboutdialog.cpp \
	dialog/opendiscdialog.cpp \
	dialog/urldialog.cpp \
	widget/simplelistwidget.cpp \
	widget/qtcolorpicker.cpp \
	widget/encodingcombobox.cpp \
	widget/colorselectwidget.cpp \
	widget/enumcombobox.cpp \
	widget/channelmanipulationwidget.cpp \
	widget/datacombobox.cpp \
	widget/verticallabel.cpp \
	widget/localecombobox.cpp \
	widget/fontoptionwidget.cpp \
	widget/menu.cpp \
	widget/deintwidget.cpp \
	enum/audiodriver.cpp \
	enum/changevalue.cpp \
	enum/channellayout.cpp \
	enum/clippingmethod.cpp \
	enum/colorrange.cpp \
	enum/decoderdevice.cpp \
	enum/deintdevice.cpp \
	enum/deintmethod.cpp \
	enum/deintmode.cpp \
	enum/dithering.cpp \
	enum/enums.cpp \
	enum/generateplaylist.cpp \
	enum/horizontalalignment.cpp \
	enum/interpolator.cpp \
	enum/keymodifier.cpp \
	enum/movetoward.cpp \
	enum/osdscalepolicy.cpp \
	enum/seekingstep.cpp \
	enum/speakerid.cpp \
	enum/staysontop.cpp \
	enum/subtitleautoload.cpp \
	enum/subtitleautoselect.cpp \
	enum/subtitledisplay.cpp \
	enum/textthemestyle.cpp \
	enum/verticalalignment.cpp \
	enum/videoratio.cpp \
	enum/videoeffect.cpp \
	misc/enumaction.cpp \
	misc/charsetdetector.cpp \
	misc/downloader.cpp \
	misc/actiongroup.cpp \
	misc/xmlrpcclient.cpp \
	misc/log.cpp \
	misc/simplelistmodel.cpp \
	misc/udf25.cpp \
	misc/keymodifieractionmap.cpp \
	misc/stepaction.cpp \
	misc/record.cpp \
	misc/undostack.cpp \
	misc/trayicon.cpp \
	player/main.cpp \
	player/mainwindow.cpp \
	player/mrl.cpp \
	player/translator.cpp \
	player/pref.cpp \
	player/playlist.cpp \
	player/playlistmodel.cpp \
	player/recentinfo.cpp \
	player/appstate.cpp \
	player/rootmenu.cpp \
	player/app.cpp \
	player/playengine.cpp \
	player/skin.cpp \
	player/mediamisc.cpp \
	player/mrlstate.cpp \
	player/historymodel.cpp \
	player/mrlstate_old.cpp \
	player/mpv_helper.cpp \
	player/mainquickview.cpp \
    audio/channellayoutmap.cpp \
    player/openmediainfo.cpp \
    enum/openmediabehavior.cpp \
    misc/json.cpp \
    misc/stepactionpair.cpp \
    opengl/opengltexturetransferinfo.cpp \
    opengl/opengllogger.cpp \
    misc/jsonstorage.cpp \
    player/mrlstatesqlfield.cpp \
    misc/localconnection.cpp \
    player/abrepeatchecker.cpp \
    widget/openmediabehaviorgroupbox.cpp \
    dialog/prefdialog_p.cpp \
    opengl/openglbenchmarker.cpp \
    enum/colorspace.cpp \
    player/mainwindow_p.cpp \
    player/mainwindow_m.cpp \
    player/playengine_p.cpp \
    enum/quicksnapshotsave.cpp \
    video/mpimage.cpp \
    enum/mousebehavior.cpp \
    misc/speedmeasure.cpp \
	player/avinfoobject.cpp \
    audio/audioformat.cpp \
    player/streamtrack.cpp \
    misc/locale.cpp \
    misc/matchstring.cpp \
    misc/simplelistdelegate.cpp \
	player/pref_helper.cpp \
    widget/prefwidgets.cpp \
    video/videorenderer.cpp \
    misc/youtubedl.cpp \
    quick/playlistthemeobject.cpp \
    misc/yledl.cpp

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
    ui/subtitlefinddialog.ui \
    ui/osdthemewidget.ui \
    ui/audionormalizeroptionwidget.ui

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
    skins/Faenza-Zukitwo/cmplayer.qml \
    imports/CMPlayer/PlayInfoVideoOutput.qml \
    imports/CMPlayer/PlayInfoAudioOutput.qml \
    imports/CMPlayer/PlayInfoTrack.qml \
    imports/CMPlayer/PlayInfoSubtitleList.qml

evil_hack_to_fool_lupdate {
SOURCES += $${OTHER_FILES}
}

OBJECTIVE_SOURCES +=

DISTFILES += \
    imports/CMPlayer/Slider.qml
