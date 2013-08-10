#include "stdafx.hpp"
#include "openmediafolderdialog.hpp"
#include "snapshotdialog.hpp"
#include "mainwindow.hpp"
#include "playeritem.hpp"
#include "rootmenu.hpp"
#include "recentinfo.hpp"
#include "pref.hpp"
#include "abrepeater.hpp"
#include "playlistview.hpp"
#include "playlistmodel.hpp"
#include "historymodel.hpp"
#include "playengine.hpp"
#include "appstate.hpp"
#include "videorendereritem.hpp"
#include "subtitlerendereritem.hpp"
#include "info.hpp"
#include "prefdialog.hpp"
#include "app.hpp"
#include "globalqmlobject.hpp"
#include "subtitleview.hpp"
#include <functional>
#include "playlistmodel.hpp"
#include "translator.hpp"
#include "playlist.hpp"
#include "subtitle_parser.hpp"
#include "subtitlemodel.hpp"
#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

class AskStartTimeEvent : public QEvent {
public:
	static constexpr QEvent::Type Type = (QEvent::Type)(QEvent::User + 1);
	AskStartTimeEvent(const Mrl &mrl, int start): QEvent(Type), mrl(mrl), start(start) {}
	Mrl mrl;	int start;
};

struct MainWindow::Data {
	Data(MainWindow *p): p(p) {preferences.load();}
	MainView *view = nullptr;
	MainWindow *p = nullptr;
	bool visible = false, sotChanging = false, fullScreen = false;
	PlayerItem *player = nullptr;
	RootMenu menu;	RecentInfo recent;
	PlayEngine engine;
	VideoRendererItem renderer;
	SubtitleRendererItem subtitle;
	QPoint prevPos;		QTimer hider;
	Qt::WindowStates winState = Qt::WindowNoState, prevWinState = Qt::WindowNoState;
	bool middleClicked = false, moving = false, changingSub = false;
	bool pausedByHiding = false, dontShowMsg = false, dontPause = false;
	bool stateChanging = false;
	ABRepeater ab = {&engine, &subtitle};
	QMenu contextMenu;
	PrefDialog *prefDlg = nullptr;
	SubtitleView *subtitleView = nullptr;
	HistoryModel history;
	PlaylistModel &playlist = engine.playlist();
//	FavoritesView *favorite;
//	QSystemTrayIcon *tray = nullptr;
	QString filePath;
	Pref preferences;
	const Pref &pref() const {return preferences;}
// methods
	int startFromStopped = -1;
	QAction *subtrackSep = nullptr;

	void openWith(const Pref::OpenMedia &mode, const QList<Mrl> &mrls) {
		if (mrls.isEmpty())
			return;
		const auto mrl = mrls.first();
		auto d = this;
		auto checkAndPlay = [this] (const Mrl &mrl) {
			if (mrl != engine.mrl())
				return false;
			if (!engine.isPlaying())
				engine.play();
			return true;
		};
		if (!checkAndPlay(mrl)) {
			Playlist playlist;
			if (mode.playlist_behavior == Enum::PlaylistBehaviorWhenOpenMedia::AppendToPlaylist) {
				d->playlist.append(mrls);
			} else if (mode.playlist_behavior == Enum::PlaylistBehaviorWhenOpenMedia::ClearAndAppendToPlaylist) {
				d->playlist.clear();
				playlist.append(mrls);
			} else if (mode.playlist_behavior == Enum::PlaylistBehaviorWhenOpenMedia::ClearAndGenerateNewPlaylist) {
				d->playlist.clear();
				playlist = p->generatePlaylist(mrl);
			}
			d->playlist.merge(playlist);
			d->engine.load(mrl, mode.start_playback);
			if (!mrl.isDvd())
				d->recent.stack(mrl);
		}
		if (!p->isVisible())
			p->show();
	}

	void syncSubtitleFileMenu() {
		if (changingSub)
			return;
		changingSub = true;
		auto &list = menu("subtitle")("track");
		auto g = list.g("external");
		const auto loaded = subtitle.loaded();
		while (g->actions().size() < loaded.size()) {
			auto action = g->addAction("");
			action->setCheckable(true);
			list.insertAction(subtrackSep, action);
		}
		while (g->actions().size() > loaded.size())
			delete g->actions().last();
		const auto actions = g->actions();
		Q_ASSERT(loaded.size() == actions.size());
		for (int i=0; i<actions.size(); ++i) {
			actions[i]->setText(loaded[i].name());
			actions[i]->setData(i);
			actions[i]->setChecked(loaded[i].isSelected());
		}
		list.syncActions();
		changingSub = false;
	}

	void updateWindowSizeState() const {
		const auto state = p->windowState();
		if (!(state & Qt::WindowFullScreen) && !(state & Qt::WindowMinimized) && p->isVisible())
			AppState::get().win_size = p->size();
	}
	void updateWindowPosState() const {
		const auto state = p->windowState();
		if (!(state & Qt::WindowFullScreen) && !(state & Qt::WindowMinimized) && p->isVisible()) {
			auto &as = AppState::get();
			const auto screen = p->window()->windowHandle()->size();
			as.win_pos.rx() = qBound(0.0, (double)p->x()/(double)screen.width(), 1.0);
			as.win_pos.ry() = qBound(0.0, (double)p->y()/(double)screen.height(), 1.0);
		}
	}
	template<typename List>
	void updateListMenu(Menu &menu, const List &list, int current, const QString &group = "") {
		if (group.isEmpty())
			menu.setEnabledSync(!list.isEmpty());
		if (!list.isEmpty()) {
			menu.g(group)->clear();
			for (typename List::const_iterator it = list.begin(); it != list.end(); ++it) {
				auto act = menu.addActionToGroupWithoutKey(it->name(), true, group);
				act->setData(it->id()); if (current == it->id()) act->setChecked(true);
			}
		}
		menu.syncActions();
	}
	template<typename F>
	void connectCurrentStreamActions(Menu *menu, F func, QString group = "") {
		auto checkCurrentStreamAction = [this, func, menu, group] () {
			const auto current = (engine.*func)();
			for (auto action : menu->g(group)->actions()) {
				if (action->data().toInt() == current) {
					action->setChecked(true);
					break;
				}
			}
		};
		connect(menu, &QMenu::aboutToShow, checkCurrentStreamAction);
		for (auto copy : menu->copies())
			connect(copy, &QMenu::aboutToShow, checkCurrentStreamAction);
	}
	template <typename T = QObject>
	T* findItem(const QString &name = QString()) {
		return player ? view->rootObject()->findChild<T*>(name) : nullptr;
	}

	void commitData() {
		static bool first = true;
		if (first) {
			const int sot = menu("window").g("stays-on-top")->checkedAction()->data().toInt();
			recent.setLastPlaylist(playlist.playlist());
			recent.setLastMrl(engine.mrl());
			engine.quit();
			auto &as = AppState::get();
			if (!(p->windowState() & Qt::WindowFullScreen))
				updateWindowPosState();
			as.audio_volume = engine.volume();
			as.audio_muted = engine.isMuted();
			as.audio_preamp = engine.preamp();
			as.audio_normalizer = engine.isVolumeNormalized();
			as.audio_scaletempo = engine.isTempoScaled();
			as.video_aspect_ratio = renderer.aspectRatio();
			as.video_crop_ratio = renderer.cropRatio();
			as.video_alignment = renderer.alignment();
			as.video_offset = renderer.offset();
			as.video_effects = renderer.effects();
			as.video_color = renderer.color();
			as.play_speed = engine.speed();
			as.sub_pos = subtitle.pos();
			as.sub_sync_delay = subtitle.delay();
			as.screen_stays_on_top = Enum::StaysOnTop::from(sot, Enum::StaysOnTop::Playing);
			as.sub_letterbox = subtitle.letterboxHint();
			as.sub_align_top = subtitle.isTopAligned();
			as.save();

			if (player)
				player->unplug();
			engine.wait();
			cApp.processEvents();
			first = false;
		}
	}
};

#ifdef Q_OS_MAC
void qt_mac_set_dock_menu(QMenu *menu);
#endif

MainWindow::MainWindow(QWidget *parent): QWidget(parent, Qt::WindowFullscreenButtonHint | Qt::Window), d(new Data(this)) {
	setAcceptDrops(true);
	d->view = new MainView(this);
	auto widget = createWindowContainer(d->view, this);
	auto l = new QVBoxLayout;
	l->addWidget(widget);
	l->setMargin(0);
	setLayout(l);

	d->engine.start();
	d->engine.setGetStartTimeFunction([this] (const Mrl &mrl){return getStartTime(mrl);});
	d->engine.setVideoRenderer(&d->renderer);
	d->renderer.setOverlay(&d->subtitle);
	d->subtitleView = new SubtitleView(this);

	resize(400, 300);
	setMinimumSize(QSize(400, 300));
	d->hider.setSingleShot(true);

	connectMenus();

	connect(&d->engine, &PlayEngine::mrlChanged, this, &MainWindow::updateMrl);
	connect(&d->engine, &PlayEngine::stateChanged, [this] (EngineState state) {
		d->stateChanging = true;
		switch (state) {
		case EnginePlaying:
		case EngineBuffering:
		case EngineOpening:
		case EnginePreparing:
			d->menu("play")["pause"]->setText(tr("Pause"));
			break;
		default:
			d->menu("play")["pause"]->setText(tr("Play"));
		}
		cApp.setScreensaverDisabled(d->pref().disable_screensaver && state == EnginePlaying);
		updateStaysOnTop();
		d->stateChanging = false;
	});
	connect(&d->engine, &PlayEngine::tick, &d->subtitle, &SubtitleRendererItem::render);
	connect(&d->engine, &PlayEngine::volumeNormalizedChanged, d->menu("audio")["normalizer"], &QAction::setChecked);
	connect(&d->engine, &PlayEngine::tempoScaledChanged, d->menu("audio")["tempo-scaler"], &QAction::setChecked);
	connect(&d->engine, &PlayEngine::mutedChanged, d->menu("audio")["mute"], &QAction::setChecked);
	connect(&d->recent, &RecentInfo::openListChanged, this, &MainWindow::updateRecentActions);
	connect(&d->hider, &QTimer::timeout, [this] () {setCursorVisible(false);});
	connect(&d->history, &HistoryModel::playRequested, [this] (const Mrl &mrl) {openMrl(mrl);});
	connect(&d->playlist, &PlaylistModel::finished, [this] () {
		if (d->menu("tool")["auto-exit"]->isChecked()) exit();
		if (d->menu("tool")["auto-shutdown"]->isChecked()) cApp.shutdown();
	});
	connect(&d->subtitle, &SubtitleRendererItem::modelsChanged, d->subtitleView, &SubtitleView::setModels);
	connect(&d->engine, &PlayEngine::started, [this] () {
		d->updateListMenu(d->menu("play")("title"), d->engine.dvd().titles, d->engine.currentDvdTitle());
//		d->updateListMenu(d->menu("play")("chapter"), d->engine.chapters(), d->engine.currentChapter());
		d->subtitle.setFps(d->engine.fps());
	});
	connect(&d->engine, &PlayEngine::audioStreamsChanged, [this] (const StreamList &streams) {
		d->updateListMenu(d->menu("audio")("track"), streams, d->engine.currentAudioStream());
	});
	connect(&d->engine, &PlayEngine::videoStreamsChanged, [this] (const StreamList &streams) {
		d->updateListMenu(d->menu("video")("track"), streams, d->engine.currentVideoStream());
	});
	connect(&d->engine, &PlayEngine::subtitleStreamsChanged, [this] (const StreamList &streams) {
		d->updateListMenu(d->menu("subtitle")("track"), streams, d->engine.currentSubtitleStream(), _L("internal"));
	});
	connect(&d->engine, &PlayEngine::chaptersChanged, [this] (const ChapterList &chapters) {
		d->updateListMenu(d->menu("play")("chapter"), chapters, d->engine.currentChapter());
	});

	connect(&d->engine, &PlayEngine::started, &d->history, &HistoryModel::setStarted);
	connect(&d->engine,	&PlayEngine::stopped, &d->history, &HistoryModel::setStopped);
	connect(&d->engine, &PlayEngine::finished, &d->history, &HistoryModel::setFinished);
	connect(&d->renderer, &VideoRendererItem::screenRectChanged, &d->subtitle, &SubtitleRendererItem::setScreenRect);
	d->connectCurrentStreamActions(&d->menu("play")("title"), &PlayEngine::currentDvdTitle);
	d->connectCurrentStreamActions(&d->menu("play")("chapter"), &PlayEngine::currentChapter);
	d->connectCurrentStreamActions(&d->menu("audio")("track"), &PlayEngine::currentAudioStream);
	d->connectCurrentStreamActions(&d->menu("video")("track"), &PlayEngine::currentVideoStream);
	d->connectCurrentStreamActions(&d->menu("subtitle")("track"), &PlayEngine::currentSubtitleStream);
	auto showSize = [this] {
		showMessage(_N(qRound(d->renderer.width())) % _U("\303\227") % _N(qRound(d->renderer.height())), &d->pref().show_osd_on_resized);
	};
	connect(&d->renderer, &VideoRendererItem::widthChanged, showSize);
	connect(&d->renderer, &VideoRendererItem::heightChanged, showSize);

	auto addContextMenu = [this] (Menu &menu) {d->contextMenu.addMenu(menu.copied(&d->contextMenu));};
	addContextMenu(d->menu("open"));
	d->contextMenu.addSeparator();
	addContextMenu(d->menu("play"));
	addContextMenu(d->menu("video"));
	addContextMenu(d->menu("audio"));
	addContextMenu(d->menu("subtitle"));
	d->contextMenu.addSeparator();
	addContextMenu(d->menu("tool"));
	addContextMenu(d->menu("window"));
	d->contextMenu.addSeparator();
	d->contextMenu.addAction(d->menu("help")["about"]);
	d->contextMenu.addAction(d->menu["exit"]);
#ifdef Q_OS_MAC
////	qt_mac_set_dock_menu(&d->menu);
	QMenuBar *mb = cApp.globalMenuBar();
	qDeleteAll(mb->actions());
	auto addMenuBar = [this, mb] (Menu &menu) {mb->addMenu(menu.copied(mb));};
	addMenuBar(d->menu("open"));
	addMenuBar(d->menu("play"));
	addMenuBar(d->menu("video"));
	addMenuBar(d->menu("audio"));
	addMenuBar(d->menu("subtitle"));
	addMenuBar(d->menu("tool"));
	addMenuBar(d->menu("window"));
	addMenuBar(d->menu("help"));
#endif
	while (!d->engine.isInitialized())
		d->engine.msleep(1);

	d->dontShowMsg = true;

	const AppState &as = AppState::get();

	if (as.win_size.isValid()) {
		const auto screen = this->windowHandle()->screen()->size();
		move(screen.width()*as.win_pos.x(), screen.height()*as.win_pos.y());
		resize(as.win_size);
	}

	d->engine.setSpeed(as.play_speed);
	d->engine.setVolume(as.audio_volume);
	d->engine.setMuted(as.audio_muted);
	d->engine.setPreamp(as.audio_preamp);
	d->engine.setVolumeNormalized(as.audio_normalizer);
	d->engine.setTempoScaled(as.audio_scaletempo);

	d->renderer.setOffset(as.video_offset);
	d->renderer.setEffects((VideoRendererItem::Effects)as.video_effects);
	d->renderer.setColor(as.video_color);

	d->subtitle.setPos(as.sub_pos);
	d->subtitle.setDelay(as.sub_sync_delay);

	d->menu("video")("aspect").g()->trigger(as.video_aspect_ratio);
	d->menu("video")("crop").g()->trigger(as.video_crop_ratio);
	d->menu("video")("align").g("horizontal")->trigger(as.video_alignment.id() & 0x0f);
	d->menu("video")("align").g("vertical")->trigger(as.video_alignment.id() & 0xf0);
	for (int i=0; i<16; ++i) {
		if ((as.video_effects >> i) & 1)
			d->menu("video")("filter").g()->setChecked(1 << i, true);
	}
	d->menu("subtitle").g("display")->trigger((int)as.sub_letterbox);
	d->menu("subtitle").g("align")->trigger((int)as.sub_align_top);
	d->menu("tool")["auto-exit"]->setChecked(as.auto_exit);
	d->menu("window").g("stays-on-top")->trigger(as.screen_stays_on_top.id());

	d->dontShowMsg = false;

//	applyPref();

	d->engine.setPlaylist(d->recent.lastPlaylist());
	d->engine.load(d->recent.lastMrl());
	updateRecentActions(d->recent.openList());

	d->winState = d->prevWinState = windowState();

#ifndef Q_OS_MAC
//	d->tray = new QSystemTrayIcon(cApp.defaultIcon(), this);
//	connect(d->tray, &QSystemTrayIcon::activated, [this] (QSystemTrayIcon::ActivationReason reason) {
//		if (reason == QSystemTrayIcon::Trigger)
//			setVisible(!isVisible());
//		else if (reason == QSystemTrayIcon::Context)
//			d->contextMenu.exec(QCursor::pos());
//	});
//	d->tray->setVisible(d->preferences.enable_system_tray);
#endif


//	Currently, session management does not works.
//	connect(&cApp, &App::commitDataRequest, [this] () { d->commitData(); });
//	connect(&cApp, &App::saveStateRequest, [this] (QSessionManager &session) {
//		session.setRestartHint(QSessionManager::RestartIfRunning);
//	});

	QTimer::singleShot(1, this, SLOT(applyPref()));
}

MainWindow::~MainWindow() {
	exit();
	delete d;
}

void MainWindow::connectMenus() {
	Menu &open = d->menu("open");
	connect(open["file"], &QAction::triggered, [this] () {
		AppState &as = AppState::get();
		const QString filter = Info::mediaExtFilter();
		const QString dir = QFileInfo(as.open_last_file).absolutePath();
		const QString file = _GetOpenFileName(this, tr("Open File"), dir, filter);
		if (!file.isEmpty()) {openMrl(Mrl(file)); as.open_last_file = file;}
	});
	connect(open["folder"], &QAction::triggered, [this] () {
		OpenMediaFolderDialog dlg(this);
		if (dlg.exec()) {
			const auto list = dlg.playlist();
			if (!list.isEmpty()) {
				d->engine.setPlaylist(list);
				d->engine.load(list.first(), true);
				d->recent.stack(list.first());
			}
		}
	});
	connect(open["url"], &QAction::triggered, [this] () {
		GetUrlDialog dlg; if (dlg.exec()) {openMrl(dlg.url().toString(), dlg.encoding());}
	});
	connect(open["dvd"], &QAction::triggered, [this] () {
		OpenDvdDialog dlg; dlg.setDevices(cApp.devices());
		if (dlg.exec()) {openMrl(Mrl(_L("dvd://") % (dlg.device().isEmpty() ? QString("") : ("/" % dlg.device()))));}
	});
	connect(open("recent").g(), &ActionGroup::triggered, [this] (QAction *a) {openMrl(Mrl(a->data().toString()));});
	connect(open("recent")["clear"], &QAction::triggered, &d->recent, &RecentInfo::clear);

	Menu &play = d->menu("play");
	connect(play["stop"], &QAction::triggered, [this] () {d->engine.stop();});
	connect(play("speed").g(), &ActionGroup::triggered, [this] (QAction *a) {
		const int diff = a->data().toInt();
		d->engine.setSpeed(qBound(0.1, diff ? (d->engine.speed() + diff/100.0) : 1, 10.0));
		showMessage(tr("Speed"), QString::fromUtf8("\303\227%1").arg(d->engine.speed()));
	});
	connect(play["pause"], &QAction::triggered, [this] () {
		if (!d->stateChanging) {
			if (d->pref().pause_to_play_next_image && d->pref().image_duration == 0 && d->engine.mrl().isImage())
				d->menu("play")["next"]->trigger();
			else {
				const auto state = d->engine.state();
				switch (state) {
				case EnginePlaying:
				case EngineOpening:
				case EngineBuffering:
				case EnginePreparing:
					d->engine.pause();
					break;
				default:
					d->engine.play();
					break;
				}
			}
		}
	});
	connect(play("repeat").g(), &ActionGroup::triggered, [this] (QAction *a) {
		const int key = a->data().toInt();
		auto msg = [this] (const QString &ex) {showMessage(tr("A-B Repeat"), ex);};
		if (key == 'r') {
			if (d->engine.isStopped()) return;
			if (!d->ab.hasA()) msg(tr("Set A to %1").arg(_Chopped(_MSecToString(d->ab.setAToCurrentTime(), "h:mm:ss.zzz"), 2)));
			else if (!d->ab.hasB()) {
				const int at = d->ab.setBToCurrentTime();
				if ((at - d->ab.a()) < 100) {d->ab.setB(-1); msg(tr("Range is too short!"));}
				else {d->ab.start(); msg(tr("Set B to %1. Start to repeat!").arg(_Chopped(_MSecToString(at, "h:mm:ss.zzz"), 2)));}
			}
		} else if (key == 'q') {d->ab.stop(); d->ab.setA(-1); d->ab.setB(-1); msg(tr("Quit repeating"));}
		else if (key == 's') {d->ab.setAToSubtitleTime(); d->ab.setBToSubtitleTime(); d->ab.start(); msg(tr("Repeat current subtitle"));}
	});
	connect(play["prev"], &QAction::triggered, &d->playlist, &PlaylistModel::playPrevious);
	connect(play["next"], &QAction::triggered, &d->playlist, &PlaylistModel::playNext);
	connect(play("seek").g("relative"), &ActionGroup::triggered, [this] (QAction *a) {
		const int diff = a->data().toInt();
		if (diff && !d->engine.isStopped() && d->engine.isSeekable()) {
			d->engine.relativeSeek(diff);
			showMessage(tr("Seeking"), diff/1000, tr("sec"), true);
			if (d->player) d->player->doneSeeking();
		}
	});
	connect(play("seek").g("subtitle"), &ActionGroup::triggered, [this] (QAction *a) {
		const int key = a->data().toInt();
		const int time = (key < 0 ? d->subtitle.previous() : (key > 0 ? d->subtitle.next() : d->subtitle.current()));
		if (time >= 0) d->engine.seek(time-100);
	});
	connect(play("title").g(), &ActionGroup::triggered, [this] (QAction *a) {
		a->setChecked(true); d->engine.setCurrentDvdTitle(a->data().toInt()); showMessage(tr("Current DVD Title"), a->text());
	});
	connect(play("chapter").g(), &ActionGroup::triggered, [this] (QAction *a) {
		a->setChecked(true); d->engine.setCurrentChapter(a->data().toInt()); showMessage(tr("Current Chapter"), a->text());
	});

	Menu &video = d->menu("video");
	connect(video("track").g(), &ActionGroup::triggered, [this] (QAction *a) {
		a->setChecked(true); d->engine.setCurrentVideoStream(a->data().toInt()); showMessage(tr("Current Video Track"), a->text());
	});
	connect(video("aspect").g(), &ActionGroup::triggered, [this] (QAction *a) {d->renderer.setAspectRatio(a->data().toDouble());});
	connect(video("crop").g(), &ActionGroup::triggered, [this] (QAction *a) {d->renderer.setCropRatio(a->data().toDouble());});
	connect(video["snapshot"], &QAction::triggered, [this] () {
		static SnapshotDialog *dlg = new SnapshotDialog(this);
		dlg->setVideoRenderer(&d->renderer); dlg->setSubtitleRenderer(&d->subtitle); dlg->take();
		if (!dlg->isVisible()) {dlg->adjustSize(); dlg->show();}
	});
	connect(video["drop-frame"], &QAction::toggled, [this] (bool enabled) {
		d->engine.setFrameDrop(enabled); showMessage(tr("Drop Frame"), enabled);
	});
	connect(&video("align"), &Menu::triggered, [this] () {
		int key = 0;
		for (auto a : d->menu("video")("align").actions()) {if (a->isChecked()) key |= a->data().toInt();}
		d->renderer.setAlignment(key);
	});

	connect(&video("move"), &Menu::triggered, [this] (QAction *action) {
		const int move = action->data().toInt();
		if (move == Qt::NoArrow) {
			d->renderer.setOffset(QPoint(0, 0));
		} else {
			const double x = move == Qt::LeftArrow ? -1 : (move == Qt::RightArrow ? 1 : 0);
			const double y = move == Qt::UpArrow ? -1 : (move == Qt::DownArrow ? 1 : 0);
			d->renderer.setOffset(d->renderer.offset() += QPoint(x, y));
		}
	});
	connect(&video("filter"), &Menu::triggered, [this] () {
		VideoRendererItem::Effects effects = 0;
		for (auto act : d->menu("video")("filter").actions()) {
			if (act->isChecked()) effects |= static_cast<VideoRendererItem::Effect>(act->data().toInt());
		} d->renderer.setEffects(effects);
	});
	connect(video("color").g(), &ActionGroup::triggered, [this] (QAction *action) {
		const auto data = action->data().toList();
		const auto prop = ColorProperty::Value(data[0].toInt());
		if (prop == ColorProperty::PropMax) {
			d->renderer.setColor(ColorProperty());
			showMessage(tr("Reset brightness, contrast, saturation and hue"));
		} else {
			QString cmd;
			switch(prop) {
			case ColorProperty::Brightness: cmd = tr("Brightness"); break;
			case ColorProperty::Saturation: cmd = tr("Saturation"); break;
			case ColorProperty::Hue: cmd = tr("Hue"); break;
			case ColorProperty::Contrast: cmd = tr("Contrast"); break;
			default: return;}
			ColorProperty color = d->renderer.color();
			color.setValue(prop, color.value(prop) + data[1].toInt()*0.01);
			d->renderer.setColor(color);
			showMessage(cmd, qRound(d->renderer.color()[prop]*100.0), "%", true);
		}
	});

	Menu &audio = d->menu("audio");
	connect(audio("track").g(), &ActionGroup::triggered, [this] (QAction *a) {
		a->setChecked(true); d->engine.setCurrentAudioStream(a->data().toInt()); showMessage(tr("Current Audio Track"), a->text());
	});
	connect(audio.g("volume"), &ActionGroup::triggered, [this] (QAction *a) {
		if (const int diff = a->data().toInt()) {
			const int volume = qBound(0, d->engine.volume() + diff, 100);
			d->engine.setVolume(volume); showMessage(tr("Volume"), volume, "%");
		}
	});
	connect(audio["mute"], &QAction::toggled, [this] (bool on) {d->engine.setMuted(on); showMessage(tr("Mute"), on);});
	connect(audio.g("sync"), &ActionGroup::triggered, [this] (QAction *a) {
		const int diff = a->data().toInt(); const int sync = diff ? d->engine.audioSync() + diff : 0;
		d->engine.setAudioSync(sync); showMessage(tr("Audio Sync"), sync*0.001, "sec", true);
	});
	connect(audio.g("amp"), &ActionGroup::triggered, [this] (QAction *a) {
		const int amp = qBound(0, qRound(d->engine.preamp()*100 + a->data().toInt()), 1000);
		d->engine.setPreamp(amp*0.01); showMessage(tr("Amplifier"), amp, "%");
	});
	connect(audio["normalizer"], &QAction::toggled, [this] (bool on) {
		d->engine.setVolumeNormalized(on); showMessage(tr("Volume Normalizer"), on);
	});
	connect(audio["tempo-scaler"], &QAction::toggled, [this] (bool on) {
		d->engine.setTempoScaled(on); showMessage(tr("Tempo Scaler"), on);
	});
	auto  selectNext = [this] (const QList<QAction*> &actions) {
		if (!actions.isEmpty()) {
			auto it = actions.begin();
			while (it != actions.end() && !(*it++)->isChecked()) ;
			if (it == actions.end())
				it = actions.begin();
			(*it)->trigger();
		}
	};
	connect(audio("track")["next"], &QAction::triggered, [this, selectNext] () {
		selectNext(d->menu("audio")("track").g()->actions());
	});

	Menu &sub = d->menu("subtitle");
	d->subtrackSep = sub("track").addSeparator();
	connect(sub("track")["next"], &QAction::triggered, [this] () {
		int checked = -1;
		auto list = d->menu("subtitle")("track").g("external")->actions();
		list += d->menu("subtitle")("track").g("internal")->actions();
		if (list.isEmpty() || (list.size() == 1 && list.first()->isChecked()))
			return;
		d->changingSub = true;
		for (int i=0; i<list.size(); ++i) {
			if (list[i]->isChecked()) {
				checked = i;
				list[i]->setChecked(false);
			}
		}
		d->changingSub = false;
		d->subtitle.deselect(-1);
		if (++checked >= list.size())
			checked = 0;
		list[checked]->trigger();
		if (!d->menu("subtitle")("track").g("internal")->checkedAction())
			d->engine.setCurrentSubtitleStream(-2);
	});
	connect(sub("track")["all"], &QAction::triggered, [this] () {
		d->subtitle.select(-1);
		for (auto action : d->menu("subtitle")("track").g("external")->actions())
			action->setChecked(true);
		showMessage(tr("Select All Subtitles"), tr("%1 Subtitle(s)").arg(d->subtitle.loaded().size()));
	});
	connect(sub("track")["hide"], &QAction::toggled, [this] (bool hide) {
		d->subtitle.setVisible(!hide);
		d->engine.setSubtitleStreamsVisible(!hide);
	});
	connect(sub("track")["open"], &QAction::triggered, [this] () {
		const QString filter = tr("Subtitle Files") % ' ' % Info::subtitleExt().toFilter();
		const auto dir = d->engine.mrl().isLocalFile() ? QFileInfo(d->engine.mrl().toLocalFile()).absolutePath() : _L("");
		QString enc = d->pref().sub_enc;
		const auto files = EncodingFileDialog::getOpenFileNames(nullptr, tr("Open Subtitle"), dir, filter, &enc);
		if (!files.isEmpty())
			appendSubFiles(files, true, enc);
	});
	connect(sub("track")["clear"], &QAction::triggered, this, &MainWindow::clearSubtitleFiles);
	connect(sub("track").g("external"), &ActionGroup::triggered, [this] (QAction *a) {
		if (!d->changingSub) {
			if (a->isChecked())
				d->subtitle.select(a->data().toInt());
			else
				d->subtitle.deselect(a->data().toInt());
		}
		showMessage(tr("Selected Subtitle"), a->text());
	});
	connect(sub("track").g("internal"), &ActionGroup::triggered, [this] (QAction *a) {
		a->setChecked(true); d->engine.setCurrentSubtitleStream(a->data().toInt());
		showMessage(tr("Selected Subtitle"), a->text());
	});
	connect(sub.g("display"), &ActionGroup::triggered, [this] (QAction *a) {d->subtitle.setLetterboxHint(a->data().toInt());});
	connect(sub.g("align"), &ActionGroup::triggered, [this] (QAction *a) {d->subtitle.setTopAlignment(a->data().toInt());});
	connect(sub.g("pos"), &ActionGroup::triggered, [this] (QAction *a) {
		const int pos = qBound(0, qRound(d->subtitle.pos()*100.0 + a->data().toInt()), 100);
		d->subtitle.setPos(pos*0.01); showMessage(tr("Subtitle Position"), pos, "%");
	});
	connect(sub.g("sync"), &ActionGroup::triggered, [this] (QAction *a) {
		const int diff = a->data().toInt(); const int delay = diff ? d->subtitle.delay() + diff : 0;
		d->subtitle.setDelay(delay); showMessage("Subtitle Sync", delay*0.001, "sec", true);
	});

	Menu &tool = d->menu("tool");
	auto toggleTool = [this] (const char *name, bool &visible) {
		visible = !visible;
		if (auto item = d->findItem<QObject>(name))
			item->setProperty("show", visible);
	};
	auto selectedIndex = [this] (const char *name) {
		QObject *item = nullptr;
		return (item = d->findItem<QObject>(name)) ? item->property("selectedIndex").toInt() : -1;
	};
	auto selectIndex = [this] (const char *name, int idx) {
		if (auto item = d->findItem<QObject>(name))
			item->setProperty("selectedIndex", idx);
	};
	auto &playlist = tool("playlist");
	connect(playlist["toggle"], &QAction::triggered, [toggleTool] () {toggleTool("playlist", AppState::get().playlist_visible);});
	connect(playlist["open"], &QAction::triggered, [this] () {
		QString enc;
		const QString filter = tr("Playlist") +' '+ Info::playlistExt().toFilter();
		const QString file = EncodingFileDialog::getOpenFileName(this, tr("Open File"), QString(), filter, &enc);
		if (!file.isEmpty())
			d->playlist.setPlaylist(Playlist(file, enc));
	});
	connect(playlist["save"], &QAction::triggered, [this] () {
		const Playlist &list = d->playlist.playlist();
		if (!list.isEmpty()) {
			auto file = _GetSaveFileName(this, tr("Save File"), QString(), tr("Playlist") + " (*.pls)");
			if (!file.isEmpty()) {
				if (QFileInfo(file).suffix().compare("pls", Qt::CaseInsensitive) != 0)
					file += ".pls";
				list.save(file);
			}
		}
	});
	connect(playlist["clear"], &QAction::triggered, [this] () { d->playlist.clear(); });
	connect(playlist["append-file"], &QAction::triggered, [this] () {
		const auto filter = Info::mediaExtFilter();
		auto files = _GetOpenFileNames(this, tr("Open File"), QString(), filter);
		Playlist list;
		for (int i=0; i<files.size(); ++i)
			list << Mrl(files[i]);
		d->playlist.append(list);
	});
	connect(playlist["append-url"], &QAction::triggered, [this] () {
		GetUrlDialog dlg(this);
		if (dlg.exec()) {
			const Mrl mrl = dlg.url().toString();
			if (mrl.isPlaylist()) {
				Playlist list;
				list.load(mrl, dlg.encoding());
				d->playlist.append(list);
			} else
				d->playlist.append(mrl);
		}
	});
	connect(playlist["remove"], &QAction::triggered, [this, selectedIndex] () {
		d->playlist.erase(selectedIndex("playlist"));
	});
	connect(playlist["move-up"], &QAction::triggered, [this, selectedIndex, selectIndex] () {
		const auto idx = selectedIndex("playlist");
		if (d->playlist.swap(idx, idx-1))
			selectIndex("playlist", idx-1);
	});
	connect(playlist["move-down"], &QAction::triggered, [this, selectedIndex, selectIndex] () {
		const auto idx = selectedIndex("playlist");
		if (d->playlist.swap(idx, idx+1))
			selectIndex("playlist", idx+1);
	});

	connect(tool["history"], &QAction::triggered, [toggleTool] () {toggleTool("history", AppState::get().history_visible);});
	connect(tool["playinfo"], &QAction::triggered, [toggleTool] () {toggleTool("playinfo", AppState::get().playinfo_visible);});
	connect(tool["subtitle"], &QAction::triggered, [this] () {d->subtitleView->setVisible(!d->subtitleView->isVisible());});
	connect(tool["pref"], &QAction::triggered, [this] () {
		if (!d->prefDlg) {
			d->prefDlg = new PrefDialog(this);
			connect(d->prefDlg, &PrefDialog::applyRequested, [this] {d->prefDlg->get(d->preferences); applyPref();});
			connect(d->prefDlg, &PrefDialog::resetRequested, [this] {d->prefDlg->set(d->pref());});
		}
		d->prefDlg->set(d->pref());
		d->prefDlg->show();
	});
	connect(tool["reload-skin"], &QAction::triggered, this, &MainWindow::reloadSkin);
	connect(tool["auto-exit"], &QAction::toggled, [this] (bool on) {
		showMessage((AppState::get().auto_exit = on) ? tr("Exit CMPlayer when the playlist has finished.") : tr("Auto-exit is canceled."));
	});
	connect(tool["auto-shutdown"], &QAction::toggled, [this] (bool on) {
		if (on) {
			if (QMessageBox::warning(nullptr, tr("Auto-shutdown")
					, tr("The system will shut down when the play list has finished.")
					, QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
				d->menu("tool")["auto-shutdown"]->setChecked(false);
			} else
				showMessage("The system will shut down when the play list has finished.");
		} else
			showMessage("Auto-shutdown is canceled.");
	});



	Menu &win = d->menu("window");		Menu &help = d->menu("help");

	connect(win.g("stays-on-top"), &ActionGroup::triggered, this, &MainWindow::updateStaysOnTop);
	connect(win.g("size"), &ActionGroup::triggered, [this] (QAction *a) {setVideoSize(a->data().toDouble());});
	connect(win["minimize"], &QAction::triggered, this, &MainWindow::showMinimized);
	connect(win["maximize"], &QAction::triggered, this, &MainWindow::showMaximized);
	connect(win["close"], &QAction::triggered, [this] () { d->menu.hide(); close(); });

	connect(help["about"], &QAction::triggered, [this] () {AboutDialog dlg(this); dlg.exec();});
	connect(d->menu["exit"], &QAction::triggered, this, &MainWindow::exit);
}

Playlist MainWindow::generatePlaylist(const Mrl &mrl) const {
	if (!mrl.isLocalFile() || !d->pref().enable_generate_playist)
		return Playlist(mrl);
	const auto mode = d->pref().generate_playlist;
	const QFileInfo file(mrl.toLocalFile());
	const QDir dir = file.dir();
	if (mode == Enum::GeneratePlaylist::Folder)
		return Playlist().loadAll(dir);
	const auto filter = Info::mediaNameFilter();
	const auto files = dir.entryInfoList(filter, QDir::Files, QDir::Name);
	const auto fileName = file.fileName();
	Playlist list;
	bool prefix = false, suffix = false;
	auto it = files.cbegin();
	for(; it != files.cend(); ++it) {
		static QRegExp rxs("(\\D*)\\d+(.*)");
		static QRegExp rxt("(\\D*)\\d+(.*)");
		if (rxs.indexIn(fileName) == -1)
			continue;
		if (rxt.indexIn(it->fileName()) == -1)
			continue;
		if (!prefix && !suffix) {
			if (rxs.cap(1) == rxt.cap(1))
				prefix = true;
			else if (rxs.cap(2) == rxt.cap(2))
				suffix = true;
			else
				continue;
		} else if (prefix) {
			if (rxs.cap(1) != rxt.cap(1))
				continue;
		} else if (suffix) {
			if (rxs.cap(2) != rxt.cap(2))
				continue;
		}
		list.append(it->absoluteFilePath());
	}
	if (list.isEmpty())
		return Playlist(mrl);
	return list;
}

void MainWindow::openFromFileManager(const Mrl &mrl) {
	d->openWith(d->pref().open_media_from_file_manager, QList<Mrl>() << mrl);
}

void MainWindow::exit() {
	static bool done = false;
	if (!done) {
		cApp.setScreensaverDisabled(false);
		d->commitData();
		d->renderer.setOverlay(nullptr);
		cApp.quit();
		done = true;
	}
}

void MainWindow::updateRecentActions(const QList<Mrl> &list) {
	Menu &recent = d->menu("open")("recent");
	ActionGroup *group = recent.g();
	const int diff = group->actions().size() - list.size();
	if (diff < 0) {
		QList<QAction*> acts = recent.actions();
		QAction *sprt = acts[acts.size()-2];
		for (int i=0; i<-diff; ++i) {
			QAction *action = new QAction(&recent);
			recent.insertAction(sprt, action);
			recent.g()->addAction(action);
		}
	} else if (diff > 0) {
		QList<QAction*> acts = recent.g()->actions();
		for (int i=0; i<diff; ++i)
			delete acts.takeLast();
	}
	QList<QAction*> acts = group->actions();
	for (int i=0; i<list.size(); ++i) {
		QAction *act = acts[i];
		act->setData(list[i].toString());
		act->setText(list[i].displayName());
		act->setVisible(!list[i].isEmpty());
	}
}

void MainWindow::openMrl(const Mrl &mrl) {
	openMrl(mrl, QString());
}

void MainWindow::openMrl(const Mrl &mrl, const QString &enc) {
	if (mrl == d->engine.mrl()) {
		if (!d->engine.isPlaying())
			d->engine.play();
	} else {
		if (mrl.isPlaylist()) {
			d->engine.setPlaylist(Playlist(mrl, enc));
		} else {
			d->engine.setPlaylist(generatePlaylist(mrl));
			d->engine.load(mrl, true);
			if (!mrl.isDvd())
				d->recent.stack(mrl);
		}
	}
}

void MainWindow::showMessage(const QString &message, const bool *force) {
	if (force) {
		if (!*force)
			return;
	} else if (!d->pref().show_osd_on_action)
		return;
	if (!d->dontShowMsg && d->player)
		d->player->requestMessage(message);
}

void MainWindow::checkWindowState() {
	const auto state = windowState();
	d->updateWindowSizeState();
	setWindowFilePath(d->filePath);
	if (state != d->winState) {
		d->prevWinState = d->winState;
		d->winState = state;
	}
	d->dontPause = true;
	d->moving = false;
	d->prevPos = QPoint();
	const auto full = d->fullScreen || state &  Qt::WindowFullScreen;
	if (full) {
		cApp.setAlwaysOnTop(window()->windowHandle(), false);
		setVisible(true);
		if (d->pref().hide_cursor)
			d->hider.start(d->pref().hide_cursor_delay);
	} else {
		d->hider.stop();
		setCursorVisible(true);
		updateStaysOnTop();
		setVisible(true);
	}
	d->dontPause = false;
	if (!d->stateChanging)
		doVisibleAction(state != Qt::WindowMinimized);
	UtilObject::setFullScreen(full);
}

void MainWindow::setFullScreen(bool full) {
	d->dontPause = true;
	if (full != d->fullScreen) {
		d->fullScreen = full;
		d->updateWindowPosState();
#ifdef Q_OS_MAC
		if (!d->pref().lion_style_fullscreen) {
			static Qt::WindowFlags flags = this->flags();
			static QRect geometry;
			if (full) {
				flags = this->flags();
				geometry = this->geometry();
				setFlags(flags | Qt::FramelessWindowHint);
				setGeometry(QRect(QPoint(0, 0), screen()->size()));
				SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
			} else {
				setFlags(flags);
				setGeometry(geometry);
				SetSystemUIMode(kUIModeNormal, 0);
			}
			checkWindowState();
			updateTitle();
		} else
#endif
			setWindowState(full ? Qt::WindowFullScreen : d->prevWinState);
	}
	d->dontPause = false;
}

bool MainWindow::isFullScreen() const {return d->fullScreen;}

void MainWindow::setVideoSize(double rate) {
	if (rate < 0) {
		setFullScreen(!isFullScreen());
	} else {
		// patched by Handrake
		const QSizeF video = d->renderer.sizeHint();
		const QSizeF desktop = window()->windowHandle()->screen()->availableVirtualSize();
		const double target = 0.15;
		if (isFullScreen())
			setFullScreen(false);
		if (rate == 0.0)
			rate = desktop.width()*desktop.height()*target/(video.width()*video.height());
		const QSize size = (this->size() - d->renderer.size() + d->renderer.sizeHint()*qSqrt(rate)).toSize();
		if (size != this->size()) {
			if (isMaximized())
				showNormal();
			resize(size);
			int dx = 0;
			const int rightDiff = desktop.width() - (x() + width());
			if (rightDiff < 10) {
				if (rightDiff < 0)
					dx = desktop.width() - x() - size.width();
				else
					dx = width() - size.width();
			}
			if (dx && !isFullScreen()) {
				int x = this->x() + dx;
				if (x < 0)
					x = 0;
				move(x, this->y());
			}
		}
	}
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
	QWidget::mouseMoveEvent(event);
	d->hider.stop();
	setCursorVisible(true);
	const bool full = isFullScreen();
	if (full) {
		if (d->moving) {
			d->moving = false;
			d->prevPos = QPoint();
		}
		if (d->pref().hide_cursor)
			d->hider.start(d->pref().hide_cursor_delay);
	} else {
		if (d->moving) {
			const QPoint pos = event->globalPos();
			move(this->pos() + pos - d->prevPos);
			d->prevPos = pos;
		}
	}
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event) {
	QWidget::mouseDoubleClickEvent(event);
	if (event->buttons() & Qt::LeftButton) {
		if (auto action = d->menu.doubleClickAction(d->pref().double_click_map[event->modifiers()])) {
#ifdef Q_OS_MAC
			if (action == d->menu("window")["full"])
				QTimer::singleShot(300, action, SLOT(trigger()));
			else
#endif
				action->trigger();
		}
	}
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
	QWidget::mouseReleaseEvent(event);
	if (d->moving) {
		d->moving = false;
		d->prevPos = QPoint();
	}
	const auto rect = geometry();
	if (d->middleClicked && event->button() == Qt::MiddleButton && rect.contains(event->localPos().toPoint()+rect.topLeft())) {
		if (auto action = d->menu.middleClickAction(d->pref().middle_click_map[event->modifiers()]))
			action->trigger();
	}
	UtilObject::setMouseReleased(event->localPos());
}
void MainWindow::mousePressEvent(QMouseEvent *event) {
	QWidget::mousePressEvent(event);
	if (event->isAccepted())
		return;
	d->middleClicked = false;
	bool showContextMenu = false;
	switch (event->button()) {
	case Qt::LeftButton:
		if (isFullScreen())
			break;
		d->moving = true;
		d->prevPos = event->globalPos();
		break;
	case Qt::MiddleButton:
		d->middleClicked = true;
		break;
	case Qt::RightButton:
		showContextMenu = true;
		break;
	default:
		break;
	}
	if (showContextMenu)
		d->contextMenu.exec(QCursor::pos());
	else
		d->contextMenu.hide();
}
void MainWindow::wheelEvent(QWheelEvent *event) {
	QWidget::wheelEvent(event);
	if (!event->isAccepted() && event->delta()) {
		if (auto action = d->menu.wheelScrollAction(d->pref().wheel_scroll_map[event->modifiers()], event->delta() > 0)) {
			action->trigger();
			event->accept();
		}
	}
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event) {
	if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}

void MainWindow::dropEvent(QDropEvent *event) {
	const auto urls = event->mimeData()->urls();
	if (!event->mimeData()->hasUrls() || urls.isEmpty())
		return;
	Playlist playlist;
	QStringList subList;
	for (int i=0; i<urls.size(); ++i) {
		const QString suffix = QFileInfo(urls[i].path()).suffix().toLower();
		if (Info::playlistExt().contains(suffix)) {
			Playlist list;
			list.load(urls[i]);
			playlist += list;
		} else if (Info::subtitleExt().contains(suffix)) {
			subList << urls[i].toLocalFile();
		} else if (Info::videoExt().contains(suffix)
				|| Info::audioExt().contains(suffix)) {
			playlist.append(urls[i]);
		}
	}
	if (!playlist.isEmpty()) {
		d->openWith(d->pref().open_media_by_drag_and_drop, playlist);
	} else if (!subList.isEmpty())
		appendSubFiles(subList, true, d->pref().sub_enc);
}

void MainWindow::reloadSkin() {
	if (d->player)
		d->player->unplug();
	d->player = nullptr;
	d->view->engine()->clearComponentCache();
	PlayerItem::registerItems();
	d->view->rootContext()->setContextProperty("history", &d->history);
	d->view->rootContext()->setContextProperty("playlist", &d->playlist);
	Skin::apply(d->view, d->pref().skin_name);
	if (d->view->status() == QQuickView::Error) {
		auto errors = d->view->errors();
		for (auto error : errors)
			qDebug() << error.toString();
		d->view->setSource(QUrl("qrc:/emptyskin.qml"));
	}
	if (!(d->player = qobject_cast<PlayerItem*>(d->view->rootObject())))
		d->player = d->view->rootObject()->findChild<PlayerItem*>();
	if (d->player) {
		d->player->plugTo(&d->engine);
		if (auto item = d->findItem("history"))
			item->setProperty("show", AppState::get().history_visible);
		if (auto item = d->findItem("playlist"))
			item->setProperty("show", AppState::get().playlist_visible);
		if (auto item = d->findItem("playinfo"))
			item->setProperty("show", AppState::get().playinfo_visible);
		if (auto item = d->findItem("logo")) {
			item->setProperty("show", d->pref().show_logo);
			item->setProperty("color", d->pref().bg_color);
		}
	}
}

void MainWindow::applyPref() {
	int time = -1;
	switch (d->engine.state()) {
	case EnginePlaying:
	case EngineBuffering:
	case EnginePaused:
		time = d->engine.position();
		break;
	default:
		break;
	}
	auto &p = d->pref();
	Translator::load(p.locale);
	d->history.setRememberImage(p.remember_image);
	d->engine.setHwAccCodecs(p.enable_hwaccel ? p.hwaccel_codecs : QList<int>());
	d->engine.setVolumeNormalizer(p.normalizer_length, p.normalizer_target, p.normalizer_silence, p.normalizer_min, p.normalizer_max);
	d->engine.setImageDuration(p.image_duration);
	d->renderer.setLumaRange(p.remap_luma_min, p.remap_luma_max);
	d->renderer.setKernel(p.blur_kern_c, p.blur_kern_n, p.blur_kern_d, p.sharpen_kern_c, p.sharpen_kern_n, p.sharpen_kern_d);
	SubtitleParser::setMsPerCharactor(p.ms_per_char);
	d->subtitle.setPriority(p.sub_priority);
	d->subtitle.setStyle(p.sub_style);
	d->menu.update(p);
	d->menu.syncTitle();
	d->menu.resetKeyMap();

	reloadSkin();
	if (time >= 0)
		d->engine.reload();

//	if (d->tray)
//		d->tray->setVisible(p.enable_system_tray);
	d->preferences.save();
}

template<typename Slot>
void connectCopies(Menu &menu, const Slot &slot) {
	QObject::connect(&menu, &Menu::aboutToShow, slot);
	for (QMenu *copy : menu.copies()) {
		QObject::connect(copy, &QMenu::aboutToShow, slot);
	}
}

void MainWindow::resizeEvent(QResizeEvent *event) {
	QWidget::resizeEvent(event);
	if (!d->fullScreen)
		d->updateWindowSizeState();
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
	QWidget::keyPressEvent(event);
	if (!event->isAccepted()) {
		constexpr int modMask = Qt::SHIFT | Qt::CTRL | Qt::ALT | Qt::META;
		auto action = RootMenu::instance().action(QKeySequence(event->key() + (event->modifiers() & modMask)));
		if (action) {
			if (action->isCheckable())
				action->toggle();
			else
				action->trigger();
		}
		event->accept();
	}
}


void MainWindow::doVisibleAction(bool visible) {
	d->visible = visible;
	if (d->visible) {
		if (d->pausedByHiding && d->engine.isPaused()) {
			d->engine.play();
			d->pausedByHiding = false;
		}
		setWindowFilePath(d->filePath);
#ifndef Q_OS_MAC
		setWindowIcon(cApp.defaultIcon());
#endif
	} else {
		if (!d->pref().pause_minimized || d->dontPause)
			return;
		if (!d->engine.isPlaying() || (d->pref().pause_video_only && !d->engine.hasVideo()))
			return;
		d->pausedByHiding = true;
		d->engine.pause();
	}
}

void MainWindow::showEvent(QShowEvent *event) {
	QWidget::showEvent(event);
	doVisibleAction(true);
}

void MainWindow::hideEvent(QHideEvent *event) {
	QWidget::hideEvent(event);
	doVisibleAction(false);
}

void MainWindow::changeEvent(QEvent *event) {
	QWidget::changeEvent(event);
	if (event->type() == QEvent::WindowStateChange)
		checkWindowState();
}

void MainWindow::closeEvent(QCloseEvent *event) {
	QWidget::closeEvent(event);
#ifndef Q_OS_MAC
	if (d->pref().enable_system_tray && d->pref().hide_rather_close) {
		hide();
		AppState &as = AppState::get();
		if (as.ask_system_tray) {
			CheckDialog dlg(this);
			dlg.setChecked(true);
			dlg.setLabelText(tr("CMPlayer will be running in the system tray "
					"when the window closed.<br>"
					"You can change this behavior in the preferences.<br>"
					"If you want to exit CMPlayer, please use 'Exit' menu."));
			dlg.setCheckBoxText(tr("Do not display this message again"));
			dlg.exec();
			as.ask_system_tray = !dlg.isChecked();
		}
		event->ignore();
	} else {
		event->accept();
		exit();
	}
#else
	event->accept();
#endif
}

void MainWindow::updateStaysOnTop() {
	if (windowState() & Qt::WindowMinimized)
		return;
	d->sotChanging = true;
	const int id = d->menu("window").g("stays-on-top")->checkedAction()->data().toInt();
	bool onTop = false;
	if (!isFullScreen()) {
		if (id == Enum::StaysOnTop::Always)
			onTop = true;
		else if (id == Enum::StaysOnTop::Never)
			onTop = false;
		else
			onTop = d->engine.isPlaying();
	}
	cApp.setAlwaysOnTop(window()->windowHandle(), onTop);
	d->sotChanging = false;
}

void MainWindow::updateTitle() {
	const auto mrl = d->engine.mrl();
	setWindowFilePath(QString());
	QString fileName;
	if (!mrl.isEmpty()) {
		if (mrl.isLocalFile()) {
			const QFileInfo file(mrl.toLocalFile());
			d->filePath = file.absoluteFilePath();
			fileName = file.fileName();
			if (isVisible())
				setWindowFilePath(d->filePath);
		} else {
			if (mrl.isDvd()) {
				fileName = d->engine.dvd().volume;
				if (fileName.isEmpty())
					fileName = _L("DVD");
			}
		}
	}
	cApp.setFileName(fileName);
//	cApp.setApplicationTitle(title);
}

void MainWindow::updateMrl(const Mrl &mrl) {
	if (mrl.isLocalFile()) {
		const auto &p = d->pref();
		auto autoselection = [this, &mrl, &p] (const QList<LoadedSubtitle> &loaded) {
			QList<int> selected;
			if (loaded.isEmpty() || !mrl.isLocalFile() || !p.sub_enable_autoselect)
				return selected;

			QSet<QString> langSet;
			const QString base = QFileInfo(mrl.toLocalFile()).completeBaseName();
			for (int i=0; i<loaded.size(); ++i) {
				bool select = false;
				if (p.sub_autoselect == Enum::SubtitleAutoselect::Matched) {
					select = QFileInfo(loaded[i].component().fileName()).completeBaseName() == base;
				} else if (p.sub_autoselect == Enum::SubtitleAutoselect::All) {
					select = true;
				} else if (p.sub_autoselect == Enum::SubtitleAutoselect::EachLanguage) {
		//			const QString lang = loaded[i].m_comp.language().id();
					const QString lang = loaded[i].component().language();
					if ((select = (!langSet.contains(lang))))
						langSet.insert(lang);
				}
				if (select)
					selected.append(i);
			}
			if (p.sub_autoselect == Enum::SubtitleAutoselect::Matched
					&& !selected.isEmpty() && !p.sub_ext.isEmpty()) {
				for (int i=0; i<selected.size(); ++i) {
					const QString fileName = loaded[selected[i]].component().fileName();
					const QString suffix = QFileInfo(fileName).suffix().toLower();
					if (p.sub_ext == suffix) {
						const int idx = selected[i];
						selected.clear();
						selected.append(idx);
						break;
					}
				}
			}
			return selected;
		};
		auto autoload = [this, mrl, &autoselection, &p](bool autoselect) {
			QList<LoadedSubtitle> loaded;
			if (!p.sub_enable_autoload)
				return loaded;
			const QStringList filter = Info::subtitleNameFilter();
			const QFileInfo fileInfo(mrl.toLocalFile());
			const QFileInfoList all = fileInfo.dir().entryInfoList(filter, QDir::Files, QDir::Name);
			const QString base = fileInfo.completeBaseName();
			for (int i=0; i<all.size(); ++i) {
				if (p.sub_autoload != Enum::SubtitleAutoload::Folder) {
					if (p.sub_autoload == Enum::SubtitleAutoload::Matched) {
						if (base != all[i].completeBaseName())
							continue;
					} else if (!all[i].fileName().contains(base))
						continue;
				}
				Subtitle sub;
				if (load(sub, all[i].absoluteFilePath(), p.sub_enc)) {
					for (int i=0; i<sub.size(); ++i)
						loaded.push_back(LoadedSubtitle(sub[i]));
				}
			}
			if (autoselect) {
				const QList<int> selected = autoselection(loaded);
				for (int i=0; i<selected.size(); ++i)
					loaded[selected[i]].selection() = true;
			}
			return loaded;
		};
		d->subtitle.setLoaded(autoload(true));
	} else
		clearSubtitleFiles();
	updateTitle();
	d->syncSubtitleFileMenu();
}

void MainWindow::clearSubtitleFiles() {
	d->subtitle.unload();
	qDeleteAll(d->menu("subtitle")("track").g("external")->actions());
	for (auto action : d->menu("subtitle")("track").g("internal")->actions()) {
		auto id = action->data().toInt();
		if (!d->engine.subtitleStreams()[id].fileName().isEmpty())
			d->engine.removeSubtitleStream(id);
	}
}

bool MainWindow::load(Subtitle &sub, const QString &fileName, const QString &encoding) {
	if (sub.load(fileName, encoding, d->pref().sub_enc_accuracy*0.01))
		return true;
	d->engine.addSubtitleStream(fileName, encoding);
	return false;
}

void MainWindow::appendSubFiles(const QStringList &files, bool checked, const QString &enc) {
	if (!files.isEmpty()) {
		Subtitle sub;
		for (auto file : files) {
			if (load(sub, file, enc))
				d->subtitle.load(sub, checked);
		}
		d->syncSubtitleFileMenu();
	}
}

void MainWindow::customEvent(QEvent *event) {
	if (event->type() == AskStartTimeEvent::Type) {
		const auto ev = static_cast<AskStartTimeEvent*>(event);
		const QDateTime date = d->history.stoppedDate(ev->mrl);

		CheckDialog dlg(this, QDialogButtonBox::Yes | QDialogButtonBox::No);
		dlg.setChecked(false);
		dlg.setLabelText(tr("Do you want to resume the playback at the last played position?\n"
			"Played Date: %1\nStopped Position: %2\n")
			.arg(date.toString(Qt::ISODate)).arg(_MSecToString(ev->start, "h:mm:ss")));
		dlg.setCheckBoxText("Don't ask again");
		dlg.setWindowTitle(tr("Resume Playback"));
		d->startFromStopped = dlg.exec() == QDialogButtonBox::Yes;
		if (_Change(d->preferences.ask_record_found, !dlg.isChecked()))
			d->preferences.save();
	}
}

int MainWindow::getStartTime(const Mrl &mrl) {
	if (!d->pref().remember_stopped || mrl.isImage())
		return 0;
	const int start = d->history.stoppedTime(mrl);
	if (start <= 0)
		return 0;
	if (d->pref().ask_record_found) {
		if (QThread::currentThread() == &d->engine) {
			d->startFromStopped = -1;
			qApp->postEvent(this, new AskStartTimeEvent(mrl, start));
			while (d->startFromStopped == -1)
				d->engine.msleep(50);
		} else {
			AskStartTimeEvent event(mrl, start);
			qApp->sendEvent(this, &event);
		}
		if (d->startFromStopped <= 0)
			return 0;
	}
	return start;
}

void MainWindow::setCursorVisible(bool visible) {
	if (visible && cursor().shape() == Qt::BlankCursor) {
		unsetCursor();
		UtilObject::setCursorVisible(true);
	} else if (!visible && cursor().shape() != Qt::BlankCursor) {
		setCursor(Qt::BlankCursor);
		UtilObject::setCursorVisible(false);
	}
}

void MainWindow::moveEvent(QMoveEvent *event) {
	QWidget::moveEvent(event);
	if (!d->fullScreen)
		d->updateWindowPosState();
}
