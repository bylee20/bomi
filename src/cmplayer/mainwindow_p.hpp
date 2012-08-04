#ifndef MAINWINDOW_P_HPP
#define MAINWINDOW_P_HPP

#include "app.hpp"
#include "avmisc.hpp"
#include "timelineosdrenderer.hpp"
#include "playinfoview.hpp"
#include "colorproperty.hpp"
#include "playlistview.hpp"
#include "historyview.hpp"
#include "playlistmodel.hpp"
#include "subtitlerenderer.hpp"
#include "charsetdetector.hpp"
#include "snapshotdialog.hpp"
#include "subtitle_parser.hpp"
#include "audiocontroller.hpp"
#include "controlwidget.hpp"
#include "pref_dialog.hpp"
#include "recentinfo.hpp"
#include "abrepeater.hpp"
#include "mainwindow.hpp"
#include "playengine.hpp"
#include "translator.hpp"
#include "videorenderer.hpp"
#include "appstate.hpp"
#include "pref.hpp"
#include "playlist.hpp"
#include "dialogs.hpp"
#include "toolbox.hpp"
#include "rootmenu.hpp"
#include "info.hpp"
#include <QtGui/QMouseEvent>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenuBar>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <qmath.h>
#include "skin.hpp"

struct MainWindow::Data {
	RootMenu &menu = RootMenu::get();
	RecentInfo &recent = RecentInfo::get();
	const Pref &p = Pref::get();

	PlayEngine &engine = PlayEngine::get();
	AudioController audio;
	VideoRenderer &video = engine.renderer();

	SubtitleRenderer subtitle;
	TimeLineOsdRenderer timeLine;
	TextOsdRenderer message = {Qt::AlignTop | Qt::AlignLeft};

	QPoint prevPos;		QTimer hider;
	bool moving = false, changingSub = false;
	bool pausedByHiding = false, dontShowMsg = false, dontPause = false;

	ABRepeater ab = {&engine, &subtitle};
	PlayInfoView playInfo = {&engine, &audio, &video};
	VideoScreen *screen = new VideoScreen;
	Skin skin;
//	ControlWidget *control = create_control_widget();
//	QWidget *center = create_central_widget();
	QMenu *context;
	PlaylistView *playlist;
	HistoryView *history;
//	FavoritesView *favorite;
#ifndef Q_WS_MAC
	QSystemTrayIcon *tray;
#endif
	QString filePath;
// methods
	void sync_subtitle_file_menu() {
		if (changingSub)
			return;
		changingSub = true;
		Menu &list = menu("subtitle")("list");
		ActionGroup *g = list.g();
		const QList<SubtitleRenderer::Loaded> loaded = subtitle.loaded();
		while (g->actions().size() < loaded.size()) {
			list.addActionToGroupWithoutKey("", true);
		}
		while (g->actions().size() > loaded.size()) {
			delete g->actions().last();
		}
		const QList<QAction*> actions = g->actions();
		Q_ASSERT(loaded.size() == actions.size());
		for (int i=0; i<actions.size(); ++i) {
			actions[i]->setText(loaded[i].name());
			actions[i]->setData(i);
			actions[i]->setChecked(loaded[i].isSelected());
		}
		changingSub = false;
	}

	void load_state() {
		dontShowMsg = true;

		const AppState &as = AppState::get();

		engine.setSpeed(as.speed);

		menu("video")("aspect").g()->trigger(as.aspect_ratio);
		menu("video")("crop").g()->trigger(as.crop_ratio);
		menu("video")("overlay").g()->trigger(as.overlay.id());
		menu("video")("align").g("horizontal")->trigger(as.screen_alignment.id() & 0x0f);
		menu("video")("align").g("vertical")->trigger(as.screen_alignment.id() & 0xf0);
		video.setOffset(as.screen_offset);
		video.setEffects((VideoRenderer::Effects)as.video_effects);
		for (int i=0; i<16; ++i) {
			if ((as.video_effects >> i) & 1)
				menu("video")("filter").g()->setChecked(1 << i, true);
		}
		video.setColorProperty(as.video_color);

		audio.setVolume(as.volume);
		audio.setMuted(as.muted);
		audio.setPreAmp(as.amp);
		audio.setVolumeNormalized(as.volume_normalized);

		menu("subtitle").g("display")->trigger((int)as.sub_letterbox);
		menu("subtitle").g("align")->trigger((int)as.sub_align_top);
		subtitle.setPos(as.sub_pos);
		subtitle.setDelay(as.sub_sync_delay);

		menu("window").g("sot")->trigger(as.stays_on_top.id());

		dontShowMsg = false;
	}

	void save_state() const {
		AppState &as = AppState::get();
		as.aspect_ratio = video.aspectRatio();
		as.crop_ratio = video.cropRatio();
		as.screen_alignment.set(video.alignment());
		as.screen_offset = video.offset();
		as.video_effects = video.effects();
		as.video_color = video.colorProperty();
		as.volume = audio.volume();
		as.volume_normalized = audio.isVolumeNormalized();
		as.muted = audio.isMuted();
		as.amp = audio.preAmp();
		as.speed = engine.speed();
		as.sub_pos = subtitle.pos();
		as.sub_sync_delay = subtitle.delay();
		as.stays_on_top = stay_on_top_mode();
		as.sub_letterbox = subtitle.osd().letterboxHint();
		as.sub_align_top = subtitle.isTopAligned();
		QAction *act = menu("video")("overlay").g()->checkedAction();
		if (act)
			as.overlay.set(act->data().toInt());
		as.save();
	}

	Enum::StaysOnTop stay_on_top_mode() const {
		const int id = menu("window").g("sot")->checkedAction()->data().toInt();
		return Enum::StaysOnTop::from(id, Enum::StaysOnTop::Playing);
	}

	void apply_pref() {
		SubtitleParser::setMsPerCharactor(p.ms_per_char);
		Translator::load(p.locale);
		subtitle.osd().setStyle(p.sub_style);
		menu.update();
		menu.save();
	#ifndef Q_WS_MAC
		tray->setVisible(p.enable_system_tray);
	#endif
	}
private:
//	ControlWidget *create_control_widget() {
//		ControlWidget *w = new ControlWidget(&engine, &audio, 0);
//		w->setAcceptDrops(false);
//		Menu &play = menu("play");
//		w->connectMute(menu("audio")["mute"]);
//		w->connectPlay(play["pause"]);
//		w->connectPrevious(play["prev"]);
//		w->connectNext(play["next"]);
//		w->connectForward(play("seek")["forward1"]);
//		w->connectBackward(play("seek")["backward1"]);
//		return w;
//	}

//	QWidget *create_central_widget() {
//		QWidget *w = new QWidget;
//		w->setAcceptDrops(false);
//		w->setMouseTracking(true);
//		w->setAutoFillBackground(false);
//		w->setAttribute(Qt::WA_OpaquePaintEvent, true);

//		QVBoxLayout *vbox = new QVBoxLayout(w);
//		vbox->addWidget(screen);
//		vbox->addWidget(control);
//		vbox->setContentsMargins(0, 0, 0, 0);
//		vbox->setSpacing(0);
//		return w;
//	}
};

#endif // MAINWINDOW_P_HPP
