TEMPLATE = app
CONFIG += link_pkgconfig debug_and_release precompile_header \
	c++11 object_parallel_to_source
macx:CONFIG -= app_bundle

QT = core gui network quick widgets sql xml svg
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
} else {
	CONFIG -= release
}

QMAKE_CXXFLAGS_CXX11 = -std=c++1y

contains(QMAKE_CXX, clang++) {
QMAKE_CXXFLAGS += -Wno-missing-braces
# clang bug: cannot build in debug mode
QMAKE_CXXFLAGS -= -g
CONFIG -= debug
CONFIG += release
} else {
QMAKE_CXXFLAGS += -Wno-non-template-friend
}

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
    HEADERS += os/mac.hpp
    OBJECTIVE_SOURCES += os/mac.mm
    INCLUDEPATH += ../ffmpeg ../ffmpeg/libavcode
} else:unix {
    QT += dbus x11extras
	TARGET = bomi
	LIBS += -ldl
	HEADERS += player/mpris.hpp
	SOURCES += player/mpris.cpp
} else:win32 {
    RC_ICONS = ../../icons/bomi.ico
    LIBS += -lopengl32 -lgdi32 -limm32 -lwinmm -lole32 -ldvdcss -lrpcrt4
    HEADERS += os/win.hpp
    SOURCES += os/win.cpp
    CONFIG -= debug
    CONFIG += release
    CONFIG += static
    contains(CONFIG, static) {
        LIBS += -lfontconfig -lfreetype -lfribidi -lxml2 -lz \
            -lnettle -lhogweed -lgmp -ltasn1 -lintl -liconv -lexpat -lharfbuzz
    }
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
	video/deintoption.hpp \
	video/letterboxitem.hpp \
	video/ffmpegfilters.hpp \
	video/softwaredeinterlacer.hpp \
	video/videocolor.hpp \
	video/deintcaps.hpp \
	video/kernel3x3.hpp \
	subtitle/subtitle.hpp \
	subtitle/subtitle_parser.hpp \
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
	pref/prefdialog.hpp \
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
	pref/pref.hpp \
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
    tmp/static_for.hpp \
    misc/localconnection.hpp \
    player/abrepeatchecker.hpp \
    widget/openmediabehaviorgroupbox.hpp \
    tmp/static_op.hpp \
	pref/prefdialog_p.hpp \
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
	pref/pref_helper.hpp \
	pref/prefwidgets.hpp \
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
    video/motionintrploption.hpp \
    enum/logoutput.hpp \
    widget/pathbutton.hpp \
	misc/logoption.hpp \
    misc/logviewer.hpp \
	tmp/type_traits.hpp \
    os/os.hpp \
    os/x11.hpp \
    enum/codecid.hpp \
    global.hpp \
    global_def.hpp \
    player/shortcutmap.hpp \
    misc/objectstorage.hpp \
    quick/formatobject.hpp \
    misc/encodinginfo.hpp \
    widget/checklistwidget.hpp \
    subtitle/subtitleviewer.hpp \
    dialog/videocolordialog.hpp \
    quick/algorithmobject.hpp \
    player/main.hpp \
    misc/stepinfo.hpp \
    player/mrlstate_old.hpp \
    misc/smbauth.hpp \
    widget/fontcombobox.hpp \
    json/jrserver.hpp \
    json/jrclient.hpp \
    json/jrcommon.hpp \
	json/jriface.hpp \
    player/jrplayer.hpp \
    enum/jrprotocol.hpp \
    enum/jrconnection.hpp \
    http-parser/http_parser.h \
    video/mpvosdrenderer.hpp \
    misc/windowsize.hpp \
    enum/framebufferobjectformat.hpp \
    video/videopreview.hpp \
    dialog/fileassocdialog.hpp

SOURCES += \
	stdafx.cpp \
	audio/audiocontroller.cpp \
	audio/channelmanipulation.cpp \
	audio/audiomixer.cpp \
	audio/audionormalizeroption.cpp \
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
	pref/prefdialog.cpp \
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
	pref/pref.cpp \
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
	pref/prefdialog_p.cpp \
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
	pref/pref_helper.cpp \
	pref/prefwidgets.cpp \
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
    video/motionintrploption.cpp \
    enum/logoutput.cpp \
    widget/pathbutton.cpp \
	misc/logoption.cpp \
	misc/logviewer.cpp \
    os/x11.cpp \
    os/os.cpp \
    enum/codecid.cpp \
    global.cpp \
    player/shortcutmap.cpp \
    misc/objectstorage.cpp \
    quick/formatobject.cpp \
    misc/encodinginfo.cpp \
    widget/checklistwidget.cpp \
    subtitle/subtitleviewer.cpp \
    dialog/videocolordialog.cpp \
    quick/algorithmobject.cpp \
    misc/stepinfo.cpp \
    player/mrlstate_old.cpp \
    misc/smbauth.cpp \
    widget/fontcombobox.cpp \
    json/jrserver.cpp \
    json/jrclient.cpp \
    json/jrcommon.cpp \
	json/jriface.cpp \
    player/jrplayer.cpp \
    enum/jrprotocol.cpp \
    enum/jrconnection.cpp \
    http-parser/http_parser.c \
    video/mpvosdrenderer.cpp \
    misc/windowsize.cpp \
    enum/framebufferobjectformat.cpp \
    video/videopreview.cpp \
    dialog/fileassocdialog.cpp

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
    ui/autoloaderwidget.ui \
    ui/logviewer.ui \
    ui/subtitleviewer.ui \
    ui/controlsthemewidget.ui \
    ui/fileassocdialog.ui

OTHER_FILES += \
	imports/bomi/qmldir \
	imports/bomi/TextOsd.qml \
	imports/bomi/ProgressOsd.qml \
	imports/bomi/PlayInfoView.qml \
	imports/bomi/Osd.qml \
	imports/bomi/Logo.qml \
	skins/simple/bomi.qml \
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
	imports/bomi/ItemColumn.qml \
	imports/bomi/ModelView.qml \
	imports/bomi/ScrollBar.qml \
	imports/bomi/MessageBox.qml \
	imports/bomi/ProgressBar.qml \
	skins/Faenza-Zukitwo/bomi.qml \
	imports/bomi/PlayInfoVideoOutput.qml \
	imports/bomi/PlayInfoAudioOutput.qml \
	imports/bomi/PlayInfoTrack.qml \
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
    imports/bomi/Text.qml \
    skins/Breeze/bomi.qml \
	"skins/Breeze Dark/bomi.qm"l \
    skins/GaN/TextButton.qml \
    imports/bomi/ButtonIcon.qml \
    skins/GaN/TimeText.qml \
    skins/Ardis/SmallButton.qml \
    skins/Ardis/MediaButton.qml \
    skins/Ardis/bomi.qml \
    imports/bomi/HideTimer.qml \
    skins/Numix/bomi.qml \
    skins/native/bomi.qml \
    skins/native/MediaButton.qml \
    imports/bomi/AutoDisplayZone.qml \
    imports/bomi/BaseApp.qml \
    imports/bomi/PlaylistView.qml \
    imports/bomi/TextStyle.qml \
    skins/air/bomi.qml \
    skins/air/ImageButton.qml \
    skins/air/IconTextButton.qml \
	imports/bomi/PlayInfoAvOutput.qml \
    imports/bomi/VideoPreviewStyle.qml \
    skins/Freya/bomi.qml \
    imports/bomi/SimpleBlur.qml \
    imports/bomi/ToolPlaneStyle.qml \
    imports/bomi/HistoryView.qml \
    skins/Tethys/bomi.qml
