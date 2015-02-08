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
!isEmpty(BOMI_RELEASE) {
    CONFIG += release
	macx:CONFIG += app_bundle
} else:CONFIG -= release

QMAKE_CXXFLAGS_CXX11 = -std=c++1y

!contains(QMAKE_CXX, clang++) {
QMAKE_CXXFLAGS += -Wno-non-template-friend
}

QMAKE_CXXFLAGS += -fpermissive

!isEmpty(USE_CCACHE): QMAKE_CXX = ccache $${QMAKE_CXX}

macx {
    QT += macextras
    CONFIG += sdk
    QMAKE_INFO_PLIST = Info.plist
	ICON = ../../icons/bomi.icns
	TARGET = bomi
    LIBS += -liconv -framework Cocoa -framework QuartzCore -framework IOKit \
		-framework IOSurface -framework Carbon -framework AudioUnit \
		-framework CoreAudio -framework VideoDecodeAcceleration \
		-framework AudioToolbox
	HEADERS += player/app_mac.hpp
	OBJECTIVE_SOURCES += player/app_mac.mm
    INCLUDEPATH += ../ffmpeg ../ffmpeg/libavcodec
} else:unix {
    QT += dbus x11extras
	TARGET = bomi
	LIBS += -ldl -lxcb-randr
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
	audio/audionormalizeroption.hpp \
	video/videoformat.hpp \
	video/hwacc.hpp \
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
	subtitle/subtitledrawer.hpp \
	subtitle/subtitlerenderingthread.hpp \
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
	enum/deintmethod.hpp \
	enum/deintmode.hpp \
	enum/dithering.hpp \
	enum/enums.hpp \
	enum/generateplaylist.hpp \
	enum/horizontalalignment.hpp \
	enum/interpolator.hpp \
	enum/keymodifier.hpp \
	enum/movetoward.hpp \
	enum/seekingstep.hpp \
	enum/speakerid.hpp \
	enum/staysontop.hpp \
	enum/autoloadmode.hpp \
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
    misc/yledl.hpp \
    audio/audioscaler.hpp \
    audio/audiobuffer.hpp \
    audio/audioanalyzer.hpp \
    audio/audioconverter.hpp \
    audio/audioresampler.hpp \
    audio/audiofilter.hpp \
	misc/osdstyle.hpp \
    quick/themeobject_helper.hpp \
    configure.hpp \
    audio/audioequalizer.hpp \
	dialog/audioequalizerdialog.hpp \
    quick/circularimageitem.hpp \
    quick/maskareaitem.hpp \
	quick/windowobject.hpp \
    misc/autoloader.hpp \
    player/mpv_property.hpp \
    enum/autoselectmode.hpp \
    player/mrlstate_p.hpp \
    subtitle/subtitlerenderer.hpp \
    video/videoprocessor.hpp \
    player/mpv.hpp \
    video/interpolatorparams.hpp \
    video/videofilter.hpp \
    video/motioninterpolator.hpp \
    enum/processor.hpp \
    video/motionintrploption.hpp

SOURCES += \
	stdafx.cpp \
	audio/audiocontroller.cpp \
	audio/channelmanipulation.cpp \
	audio/audiomixer.cpp \
	audio/audionormalizeroption.cpp \
	video/hwacc.cpp \
	video/videoformat.cpp \
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
	subtitle/subtitledrawer.cpp \
	subtitle/subtitlerenderingthread.cpp \
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
	enum/deintmethod.cpp \
	enum/deintmode.cpp \
	enum/dithering.cpp \
	enum/enums.cpp \
	enum/generateplaylist.cpp \
	enum/horizontalalignment.cpp \
	enum/interpolator.cpp \
	enum/keymodifier.cpp \
	enum/movetoward.cpp \
	enum/seekingstep.cpp \
	enum/speakerid.cpp \
	enum/staysontop.cpp \
	enum/autoloadmode.cpp \
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
    misc/yledl.cpp \
    audio/audioscaler.cpp \
    audio/audiobuffer.cpp \
    audio/audioanalyzer.cpp \
    audio/audioconverter.cpp \
    audio/audioresampler.cpp \
    audio/audiofilter.cpp \
	misc/osdstyle.cpp \
	audio/audioequalizer.cpp \
	dialog/audioequalizerdialog.cpp \
    quick/circularimageitem.cpp \
    quick/maskareaitem.cpp \
	quick/windowobject.cpp \
    misc/autoloader.cpp \
    enum/autoselectmode.cpp \
    subtitle/subtitlerenderer.cpp \
    video/videoprocessor.cpp \
    player/mpv.cpp \
    video/interpolatorparams.cpp \
    video/videofilter.cpp \
    video/motioninterpolator.cpp \
    enum/processor.cpp \
    video/motionintrploption.cpp

TRANSLATIONS += translations/bomi_ko.ts \
	translations/bomi_en.ts \
	translations/bomi_de.ts \
	translations/bomi_ru.ts \
	translations/bomi_it.ts \
	translations/bomi_cs.ts \
	translations/bomi_sr.ts \
	translations/bomi_es.ts

FORMS += \
    ui/aboutdialog.ui \
    ui/opendvddialog.ui \
    ui/snapshotdialog.ui \
    ui/prefdialog.ui \
    ui/openmediafolderdialog.ui \
    ui/subtitlefinddialog.ui \
    ui/audionormalizeroptionwidget.ui \
    ui/osdstylewidget.ui \
    ui/osdthemewidget.ui \
    ui/autoloaderwidget.ui

OTHER_FILES += \
	imports/bomi/qmldir \
	imports/bomi/TextOsd.qml \
	imports/bomi/ProgressOsd.qml \
	imports/bomi/PlayInfoView.qml \
	imports/bomi/Osd.qml \
	imports/bomi/Logo.qml \
	skins/simple/bomi.qml \
	imports/bomi/PlaylistDock.qml \
	imports/bomi/HistoryDock.qml \
	imports/bomi/Button.qml \
	imports/bomi/Player.qml \
	imports/bomi/TimeText.qml \
    skins/classic/FramedButton.qml \
	skins/classic/bomi.qml \
	skins/modern/bomi.qml \
    emptyskin.qml \
	imports/bomi/AppWithFloating.qml \
	imports/bomi/AppWithDock.qml \
	imports/bomi/TimeSlider.qml \
	imports/bomi/VolumeSlider.qml \
	imports/bomi/PlayInfoText.qml \
	skins/GaN/bomi.qml \
    skins/GaN/Button.qml \
	imports/bomi/ItemColumn.qml \
	imports/bomi/ModelView.qml \
	imports/bomi/ScrollBar.qml \
	imports/bomi/MessageBox.qml \
	imports/bomi/ProgressBar.qml \
	skins/Faenza-Zukitwo/bomi.qml \
	imports/bomi/PlayInfoVideoOutput.qml \
	imports/bomi/PlayInfoAudioOutput.qml \
	imports/bomi/PlayInfoTrack.qml \
	imports/bomi/PlayInfoSubtitleList.qml \
	skins/one/bomi.qml

evil_hack_to_fool_lupdate {
SOURCES += $${OTHER_FILES}
}

OBJECTIVE_SOURCES +=

DISTFILES += \
	imports/bomi/Slider.qml \
    imports/bomi/Circle.qml \
    skins/one/IconButton.qml \
    imports/bomi/TimeDuration.qml \
    imports/bomi/ChapterMarkerStyle.qml \
    imports/bomi/Text.qml \
    skins/Breeze/bomi.qml \
    skins/Breeze Dark/bomi.qml \
    skins/GaN/TextButton.qml \
    imports/bomi/ButtonIcon.qml \
    skins/GaN/TimeText.qml \
    skins/Ardis/SmallButton.qml \
    skins/Ardis/MediaButton.qml \
    skins/Ardis/bomi.qml \
    imports/bomi/HideTimer.qml \
    skins/Numix/bomi.qml
