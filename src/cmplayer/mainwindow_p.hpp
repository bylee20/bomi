#ifndef MAINWINDOW_P_HPP
#define MAINWINDOW_P_HPP

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
#include "application.hpp"
#include "recentinfo.hpp"
#include "abrepeater.hpp"
#include "mainwindow.hpp"
#include "playengine.hpp"
#include "translator.hpp"
#include "videorenderer.hpp"
#include "appstate.hpp"
#include "playlist.hpp"
#include "dialogs.hpp"
#include "toolbox.hpp"
#include "libvlc.hpp"
#include "rootmenu.hpp"
#include "info.hpp"
#include <QtGui/QMouseEvent>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenuBar>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <qmath.h>

class MainWindow::Data {

	bool moving, changingSub, pausedByHiding, dontShowMsg, dontPause;
	QMenu *context;
	RootMenu menu;			const Pref &pref;
	PlayEngine *engine;		VideoRenderer *video;
	SubtitleRenderer *subtitle;	AudioController *audio;
	TimeLineOsdRenderer *timeLine;	TextOsdRenderer *message;
	PlayInfoView *playInfo;
	QPoint prevPos;			QTimer *hider;
	RecentInfo recent;		ABRepeater *ab;
	PlaylistView *playlist;		HistoryView *history;
//	FavoritesView *favorite;
	QWidget *center;		ControlWidget *control;
#ifndef Q_WS_MAC
	QSystemTrayIcon *tray;
#endif
	friend class MainWindow;
// methods
	Data(): pref(Pref::get()) {}

	void sync_subtitle_file_menu() {
		if (changingSub)
			return;
		changingSub = true;
		Menu &list = menu("subtitle")("list");
		ActionGroup *g = list.g();
		const QList<SubtitleRenderer::Loaded> loaded = subtitle->loaded();
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

	ControlWidget *create_control_widget() {
		ControlWidget *w = new ControlWidget(engine, 0);
		Menu &play = menu("play");
		w->connectMute(menu("audio")["mute"]);
		w->connectPlay(play["pause"]);
		w->connectPrevious(play["prev"]);
		w->connectNext(play["next"]);
		w->connectForward(play("seek")["forward1"]);
		w->connectBackward(play("seek")["backward1"]);
		return w;
	}

	QWidget *create_central_widget(QWidget *video, QWidget *control) {
		QWidget *w = new QWidget;
		w->setMouseTracking(true);
		w->setAutoFillBackground(false);
		w->setAttribute(Qt::WA_OpaquePaintEvent, true);

		QVBoxLayout *vbox = new QVBoxLayout(w);
		vbox->addWidget(video);
		vbox->addWidget(control);
		vbox->setContentsMargins(0, 0, 0, 0);
		vbox->setSpacing(0);
		return w;
	}

	void load_state() {
		dontShowMsg = true;
		const AppState &as = AppState::get();
		menu("video")("aspect").g()->trigger(as.aspect_ratio);
		menu("video")("crop").g()->trigger(as.crop_ratio);
		menu("video")("overlay").g()->trigger(as.overlay.id());
		menu("subtitle").g("display")->trigger((int)as.sub_letterbox);
		menu("subtitle").g("align")->trigger((int)as.sub_align_top);
		menu("window").g("sot")->trigger(as.stays_on_top.id());

		audio->setVolume(as.volume);
		audio->setMuted(as.muted);
		audio->setPreAmp(as.amp);
		audio->setVolumeNormalized(as.volume_normalized);

		engine->setSpeed(as.speed);
		subtitle->setPos(as.sub_pos);
		subtitle->setDelay(as.sub_sync_delay);
		dontShowMsg = false;
	}

	void save_state() const {
		AppState &as = AppState::get();
		as.aspect_ratio = video->aspectRatio();
		as.crop_ratio = video->cropRatio();
		as.volume = audio->volume();
		as.volume_normalized = audio->isVolumeNormalized();
		as.muted = audio->isMuted();
		as.amp = audio->preAmp();
		as.speed = engine->speed();
		as.sub_pos = subtitle->pos();
		as.sub_sync_delay = subtitle->delay();
		as.stays_on_top = stay_on_top_mode();
		as.sub_letterbox = subtitle->osd()->letterboxHint();
		as.sub_align_top = subtitle->isTopAligned();
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
		Subtitle::Parser::setMsPerCharactor(pref.ms_per_char);
		Translator::load(pref.locale);
		subtitle->osd()->setStyle(pref.sub_style);
		audio->setTargetGain((double)pref.normalizer_gain/100.0);
		audio->setNormalizerSmoothness(pref.normalizer_smoothness);
		menu.update();
		menu.save();
	#ifndef Q_WS_MAC
		tray->setVisible(pref.enable_system_tray);
	#endif
	}
};

#endif // MAINWINDOW_P_HPP
