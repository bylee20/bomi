#include "stdafx.hpp"
#include "openmediafolderdialog.hpp"
#include "snapshotdialog.hpp"
#include "mainwindow.hpp"
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
#include "trayicon.hpp"
#include "playlist.hpp"
#include "subtitle_parser.hpp"
#include "subtitlemodel.hpp"
#include "openglcompat.hpp"
#include "videoformat.hpp"
#ifdef Q_OS_MAC
#include <Carbon/Carbon.h>
#endif

extern void initialize_vaapi();
extern void finalize_vaapi();
extern void initialize_vdpau();
extern void finalize_vdpau();

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
	QQuickItem *player = nullptr;
	RootMenu menu;	RecentInfo recent;
	AppState &as = AppState::get();
	PlayEngine engine;
	VideoRendererItem renderer;
	SubtitleRendererItem subtitle;
	QPoint prevPos;
	Qt::WindowStates winState = Qt::WindowNoState, prevWinState = Qt::WindowNoState;
	bool middleClicked = false, moving = false, changingSub = false;
	bool pausedByHiding = false, dontShowMsg = true, dontPause = false;
	bool stateChanging = false, loading = false;
	QTimer loadingTimer, hider, initializer;
	ABRepeater ab = {&engine, &subtitle};
	QMenu contextMenu;
	PrefDialog *prefDlg = nullptr;
	SubtitleView *subtitleView = nullptr;
	HistoryModel history;
	PlaylistModel &playlist = engine.playlist();
	QUndoStack *undo = nullptr;
//	FavoritesView *favorite;
	TrayIcon *tray = nullptr;
	QString filePath;
	Pref preferences;
	const Pref &pref() const {return preferences;}
// methods
	int startFromStopped = -1;
	QAction *subtrackSep = nullptr;

	void initContextMenu() {
		auto d = this;
		auto addContextMenu = [d] (Menu &menu) {d->contextMenu.addMenu(menu.copied(&d->contextMenu));};
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
	}

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
			if (mode.playlist_behavior == PlaylistBehaviorWhenOpenMedia::AppendToPlaylist) {
				d->playlist.append(mrls);
			} else if (mode.playlist_behavior == PlaylistBehaviorWhenOpenMedia::ClearAndAppendToPlaylist) {
				d->playlist.clear();
				playlist.append(mrls);
			} else if (mode.playlist_behavior == PlaylistBehaviorWhenOpenMedia::ClearAndGenerateNewPlaylist) {
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
		const auto components = subtitle.components();
		while (g->actions().size() < components.size()) {
			auto action = g->addAction("");
			action->setCheckable(true);
			list.insertAction(subtrackSep, action);
		}
		while (g->actions().size() > components.size())
			delete g->actions().last();
		const auto actions = g->actions();
		Q_ASSERT(components.size() == actions.size());
		for (int i=0; i<actions.size(); ++i) {
			actions[i]->setText(components[i]->name());
			actions[i]->setData(i);
			actions[i]->setChecked(components[i]->selection());
		}
		list.syncActions();
		changingSub = false;
	}

	void updateWindowSizeState() const {
		if (!p->isFullScreen() && !p->isMinimized() && p->isVisible())
			AppState::get().win_size = p->size();
	}
	void updateWindowPosState() const {
		if (!p->isFullScreen() && !p->isMinimized() && p->isVisible()) {
			auto &as = AppState::get();
			const auto screen = p->window()->windowHandle()->screen()->size();
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
		MainWindow::connect(menu, &QMenu::aboutToShow, checkCurrentStreamAction);
		for (auto copy : menu->copies())
			MainWindow::connect(copy, &QMenu::aboutToShow, checkCurrentStreamAction);
	}
	template <typename T = QObject>
	T* findItem(const QString &name = QString()) {
		return player ? view->rootObject()->findChild<T*>(name) : nullptr;
	}

	void commitData() {
		static bool first = true;
		if (first) {
			recent.setLastPlaylist(playlist.playlist());
			recent.setLastMrl(engine.mrl());
			engine.quit();
			auto &as = AppState::get();
			if (!p->isFullScreen())
				updateWindowPosState();
			as.video_effects = renderer.effects();
			as.save();

			engine.waitUntilTerminated();
			cApp.processEvents();
			first = false;
		}
	}

	template<typename T, typename Func>
	QUndoCommand *push(const T &to, const T &from, const Func &func) {
		if (undo) {
			auto cmd = new ValueCmd<Func, T>(to, from, func);
			undo->push(cmd);
			return cmd;
		} else {
			func(to);
			return nullptr;
		}
	}
	void showMessageBox(const QVariant &msg) {
		if (player)
			QMetaObject::invokeMethod(player, "showMessageBox", Q_ARG(QVariant, msg));
	}
	void showOSD(const QVariant &msg) {
		if (player)
			QMetaObject::invokeMethod(player, "showOSD", Q_ARG(QVariant, msg));
	}
	template<typename T, typename F, typename OnTriggered>
	void connectEnumActions(Menu &menu, OnTriggered onTriggered, const char *asprop, void(AppState::*sig)(), F f) {
		Q_ASSERT(as.property(asprop).isValid());
		const int size = EnumInfo<T>::size();
		auto cycle = _C(menu)[size > 2 ? "next" : "toggle"];
		auto group = menu.g(_L(EnumInfo<T>::typeKey()));
		if (cycle) {
			connect(cycle, &QAction::triggered, [this, group] () {
				auto actions = group->actions();
				int i = 0;
				for (; i<actions.size(); ++i)
					if (actions[i]->isChecked())
						break;
				if (++i >= actions.size())
					i = 0;
				actions[i]->trigger();
			});
		}
		connect(group, &QActionGroup::triggered, onTriggered);
		connect(&as, sig, f);
		group->trigger(as.property(asprop));
		emit (as.*sig)();
	}
	template<typename T, typename F>
	void connectEnumActions(Menu &menu, const char *asprop, void(AppState::*sig)(), F f) {
		auto group = menu.g(_L(EnumInfo<T>::typeKey()));
		connectEnumActions<T, F>(menu, [this, asprop, group, &menu] (QAction *a) {
			auto action = static_cast<EnumAction<T>*>(a);
			auto t = action->enum_();
			auto current = as.property(asprop).value<T>();
			if (current != t)
				push<T>(t, current, [this, asprop, group, &menu] (T t) {
					if (auto action = group->find(t)) {
						action->setChecked(true);
						p->showMessage(menu.title(), action->text());
						as.setProperty(asprop, action->data());
					}
				});
		}, asprop, sig, f);
	}
	template<typename T, typename F, typename GetNew>
	void connectEnumDataActions(Menu &menu, const char *asprop, GetNew getNew, void(AppState::*sig)(), F f) {
		using Data = typename EnumInfo<T>::Data;
		connectEnumActions<T, F>(menu, [this, asprop, getNew, &menu] (QAction *a) {
			auto action = static_cast<EnumAction<T>*>(a);
			auto old = as.property(asprop).value<Data>();
			auto new_ = getNew(action->data());
			auto step = dynamic_cast<StepAction*>(action);
			if (step)
				new_ = step->clamp(new_);
			if (old != new_)
				push<Data>(new_, old, [this, asprop, &menu, step] (const Data &data) {
					as.setProperty(asprop, QVariant::fromValue<Data>(data));
					if (step)
						p->showMessage(menu.title(), step->format(data));
				});
		}, asprop, sig, f);
	}
	template<typename T, typename F>
	void connectEnumMenu(Menu &parent, const char *asprop, void(AppState::*sig)(), F f) {
		connectEnumActions<T, F>(parent(EnumInfo<T>::typeKey()), asprop, sig, f);
	}
	template<typename F>
	void connectStepActions(Menu &menu, const char *asprop, void(AppState::*sig)(), F f) {
		connectEnumDataActions<ChangeValue, F>(menu, asprop, [this, asprop] (int diff) {
			return diff ? (as.property(asprop).toInt() + diff) : 0;
		}, sig, f);
	}

	void initWidget() {
		view = new MainView(p);
		view->setPersistentOpenGLContext(true);
		view->setPersistentSceneGraph(true);

		auto widget = createWindowContainer(view, p);
		auto l = new QVBoxLayout;
		l->addWidget(widget);
		l->setMargin(0);
		p->setLayout(l);

		subtitleView = new SubtitleView(p);
		p->setAcceptDrops(true);
		p->resize(400, 300);
		p->setMinimumSize(QSize(400, 300));

		MainWindow::connect(view, &QQuickView::sceneGraphInitialized, [this] () {
			OpenGLCompat::initialize(view->openglContext());
		});
	}
	void initEngine() {
		engine.setVideoRenderer(&renderer);
		engine.setGetStartTimeFunction([this] (const Mrl &mrl) {
			if (!pref().remember_stopped || mrl.isImage())
				return 0;
			const int start = history.stoppedTime(mrl);
			if (start <= 0)
				return 0;
			if (pref().ask_record_found) {
				if (QThread::currentThread() == engine.thread()) {
					startFromStopped = -1;
					qApp->postEvent(p, new AskStartTimeEvent(mrl, start));
					while (startFromStopped == -1)
						QThread::msleep(50);
				} else {
					AskStartTimeEvent event(mrl, start);
					qApp->sendEvent(p, &event);
				}
				if (startFromStopped <= 0)
					return 0;
			}
			return start;
		});
		engine.setGetCacheFunction([this] (const Mrl &mrl) {
			if (mrl.isLocalFile()) {
				auto path = mrl.toLocalFile();
				if (_ContainsIf(pref().network_folders, [path] (const QString &folder) {
					return path.startsWith(folder);
				}))
					return pref().cache_network;
				return pref().cache_local;
			} if (mrl.isDvd())
				return pref().cache_dvd;
			return pref().cache_network;
		});

		connect(&engine, &PlayEngine::mrlChanged, p, &MainWindow::updateMrl);
		connect(&engine, &PlayEngine::stateChanged, [this] (PlayEngine::State state) {
			stateChanging = true;
			showMessageBox(QString());
			if ((loading = state == PlayEngine::Loading))
				loadingTimer.start();
			else
				loadingTimer.stop();
			if (state == PlayEngine::Error)
				showMessageBox(tr("Error!\nCannot open the media."));
			switch (state) {
			case PlayEngine::Loading:
			case PlayEngine::Playing:
				menu("play")["pause"]->setText(tr("Pause"));
				break;
			default:
				menu("play")["pause"]->setText(tr("Play"));
			}
			cApp.setScreensaverDisabled(pref().disable_screensaver && state == PlayEngine::Playing);
			p->updateStaysOnTop();
			stateChanging = false;
		});
		connect(&engine, &PlayEngine::tick, &subtitle, &SubtitleRendererItem::render);
		connect(&engine, &PlayEngine::volumeChanged, [this] (int volume) { _Change(as.audio_volume, volume); });
		connect(&engine, &PlayEngine::volumeNormalizerActivatedChanged, menu("audio")["normalizer"], &QAction::setChecked);
		connect(&engine, &PlayEngine::tempoScaledChanged, menu("audio")["tempo-scaler"], &QAction::setChecked);
		connect(&engine, &PlayEngine::mutedChanged, menu("audio")("volume")["mute"], &QAction::setChecked);
		connect(&engine, &PlayEngine::started, [this] () { subtitle.setFPS(engine.fps()); });
		connect(&engine, &PlayEngine::dvdInfoChanged, [this] () {
			updateListMenu(menu("play")("title"), engine.dvd().titles, engine.currentDvdTitle());
		});
		connect(&engine, &PlayEngine::audioStreamsChanged, [this] (const StreamList &streams) {
			updateListMenu(menu("audio")("track"), streams, engine.currentAudioStream());
		});
		connect(&engine, &PlayEngine::videoStreamsChanged, [this] (const StreamList &streams) {
			updateListMenu(menu("video")("track"), streams, engine.currentVideoStream());
		});
		connect(&engine, &PlayEngine::subtitleStreamsChanged, [this] (const StreamList &streams) {
			updateListMenu(menu("subtitle")("track"), streams, engine.currentSubtitleStream(), _L("internal"));
		});
		connect(&engine, &PlayEngine::chaptersChanged, [this] (const ChapterList &chapters) {
			updateListMenu(menu("play")("chapter"), chapters, engine.currentChapter());
		});

		connect(&engine, &PlayEngine::started, &history, &HistoryModel::setStarted);
		connect(&engine, &PlayEngine::stopped, &history, &HistoryModel::setStopped);
		connect(&engine, &PlayEngine::finished, &history, &HistoryModel::setFinished);

		connect(&engine, &PlayEngine::videoFormatChanged, [this] (const VideoFormat &format) {
			if (pref().fit_to_video && !format.displaySize().isEmpty())
				setVideoSize(format.displaySize());
		});

		engine.waitUntilInitilaized();
	}
	void initItems() {
		connect(&recent, &RecentInfo::openListChanged, p, &MainWindow::updateRecentActions);
		connect(&hider, &QTimer::timeout, [this] () { p->setCursorVisible(false); });
		connect(&history, &HistoryModel::playRequested, [this] (const Mrl &mrl) { p->openMrl(mrl); });
		connect(&playlist, &PlaylistModel::finished, [this] () {
			if (menu("tool")["auto-exit"]->isChecked()) p->exit();
			if (menu("tool")["auto-shutdown"]->isChecked()) cApp.shutdown();
		});
		connect(&subtitle, &SubtitleRendererItem::modelsChanged, subtitleView, &SubtitleView::setModels);

		renderer.setOverlay(&subtitle);
		auto showSize = [this] {
			p->showMessage(_N(qRound(renderer.width())) % _U("\303\227") % _N(qRound(renderer.height())), &pref().show_osd_on_resized);
		};
		connect(&renderer, &VideoRendererItem::widthChanged, showSize);
		connect(&renderer, &VideoRendererItem::heightChanged, showSize);
	}
	void initTimers() {
		hider.setSingleShot(true);

		loadingTimer.setInterval(500);
		loadingTimer.setSingleShot(true);
		connect(&loadingTimer, &QTimer::timeout, [this] () {
			if (loading) showMessageBox(tr("Loading ...\nPlease wait for a while."));
		});

		initializer.setSingleShot(true);
		connect(&initializer, &QTimer::timeout, [this] () { p->applyPref(); cApp.runCommands(); });
		initializer.start(1);
	}

	template<typename T, typename F, typename GetNew>
	void connectPropertyDiff(ActionGroup *g, const char *asprop, GetNew getNew, void(AppState::*sig)(), F f) {
		Q_ASSERT(as.property(asprop).isValid());
		connect(g, &ActionGroup::triggered, [this, getNew, asprop] (QAction *a) {
			const auto old = as.property(asprop).value<T>();
			const auto new_ = getNew(a, old);
			if (old != new_) {
				push(new_, old, [this, asprop] (const T &t) {
					as.setProperty(asprop, QVariant::fromValue<T>(t));
				});
			}
		});
		connect(&as, sig, f);
		emit (as.*sig)();
	}
	template<typename T, typename F>
	void connectPropertyDiff(ActionGroup *g, const char *asprop, T min, T max, void(AppState::*sig)(), F f) {
		connectPropertyDiff<T, F>(g, asprop, [min, max] (QAction *a, const T &old) { return qBound<T>(min, a->data().value<T>() + old, max); }, sig, f);
	}
	template<typename F>
	void connectPropertyCheckable(QAction *action, const char *asprop, void(AppState::*sig)(), F f) {
		Q_ASSERT(as.property(asprop).isValid() && as.property(asprop).type() == QVariant::Bool);
		Q_ASSERT(action->isCheckable());
		connect(action, &QAction::triggered, [action, this, asprop] (bool new_) {
			const bool old = as.property(asprop).toBool();
			if (new_ != old) {
				push(new_, old, [action, asprop, this](bool checked) {
					as.setProperty(asprop, checked);
					action->setChecked(checked);
					p->showMessage(action->text(), checked);
				});
			}
		});
		connect(&as, sig, f);
		emit (as.*sig)();
	}

	void setVideoSize(const QSize &video) {
		if (p->isFullScreen() || p->isMaximized())
			return;
		// patched by Handrake
		const QSizeF desktop = p->window()->windowHandle()->screen()->availableVirtualSize();
		const QSize size = (p->size() - renderer.size().toSize() + video);
		if (size != p->size()) {
			p->resize(size);
			int dx = 0;
			const int rightDiff = desktop.width() - (p->x() + p->width());
			if (rightDiff < 10) {
				if (rightDiff < 0)
					dx = desktop.width() - p->x() - size.width();
				else
					dx = p->width() - size.width();
			}
			if (dx) {
				int x = p->x() + dx;
				if (x < 0)
					x = 0;
				p->move(x, p->y());
			}
		}
	}
};

#ifdef Q_OS_MAC
void qt_mac_set_dock_menu(QMenu *menu);
#endif

MainWindow::MainWindow(QWidget *parent): QWidget(parent, Qt::Window), d(new Data(this)) {
	initialize_vaapi();
	initialize_vdpau();

	d->engine.run();
	d->initWidget();
	d->initContextMenu();
	d->initTimers();
	d->initItems();
	d->initEngine();

	d->dontShowMsg = true;

	connectMenus();

	const AppState &as = AppState::get();

	if (as.win_size.isValid()) {
		const auto screen = this->windowHandle()->screen()->size();
		move(screen.width()*as.win_pos.x(), screen.height()*as.win_pos.y());
		resize(as.win_size);
	}

	d->renderer.setEffects((VideoRendererItem::Effects)as.video_effects);
	for (int i=0; i<16; ++i) {
		if ((as.video_effects >> i) & 1)
			d->menu("video")("filter").g()->setChecked(1 << i, true);
	}
	d->menu("tool")["auto-exit"]->setChecked(as.auto_exit);

	d->dontShowMsg = false;

	d->engine.setPlaylist(d->recent.lastPlaylist());
	if (!d->recent.lastMrl().isEmpty())
		d->engine.load(d->recent.lastMrl());
	updateRecentActions(d->recent.openList());

	d->winState = d->prevWinState = windowState();

	if (TrayIcon::isAvailable()) {
		d->tray = new TrayIcon(cApp.defaultIcon(), this);
		connect(d->tray, &TrayIcon::activated, [this] (TrayIcon::ActivationReason reason) {
			if (reason == TrayIcon::Trigger)
				setVisible(!isVisible());
			else if (reason == TrayIcon::Context)
				d->contextMenu.exec(QCursor::pos());
			else if (reason == TrayIcon::Show)
				setVisible(true);
			else if (reason == TrayIcon::Quit)
				exit();
		});
		d->tray->setVisible(d->preferences.enable_system_tray);
	}

//	Currently, session management does not works.
//	connect(&cApp, &App::commitDataRequest, [this] () { d->commitData(); });
//	connect(&cApp, &App::saveStateRequest, [this] (QSessionManager &session) {
//		session.setRestartHint(QSessionManager::RestartIfRunning);
//	});

	d->undo = new QUndoStack(this);
	auto undo = d->menu("tool")["undo"];
	auto redo = d->menu("tool")["redo"];
	connect(d->undo, &QUndoStack::canUndoChanged, undo, &QAction::setEnabled);
	connect(d->undo, &QUndoStack::canRedoChanged, redo, &QAction::setEnabled);
	connect(undo, &QAction::triggered, d->undo, &QUndoStack::undo);
	connect(redo, &QAction::triggered, d->undo, &QUndoStack::redo);
	d->menu("tool")["undo"]->setEnabled(d->undo->canUndo());
	d->menu("tool")["redo"]->setEnabled(d->undo->canRedo());

	d->dontShowMsg = false;
}

MainWindow::~MainWindow() {
	d->view->engine()->clearComponentCache();
	exit();
	delete d->view;
	delete d;
	finalize_vdpau();
	finalize_vaapi();
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
	d->connectStepActions(play("speed"), "play_speed", &AppState::playSpeedChanged, [this]() {
		d->engine.setSpeed(1e-2*d->as.playback_speed);
	});
	connect(play["pause"], &QAction::triggered, [this] () {
		if (!d->stateChanging) {
			if (d->pref().pause_to_play_next_image && d->pref().image_duration == 0 && d->engine.mrl().isImage())
				d->menu("play")["next"]->trigger();
			else {
				const auto state = d->engine.state();
				switch (state) {
				case PlayEngine::Playing:
				case PlayEngine::Loading:
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
	auto seekChapter = [this] (int offset) {
		int chapter = d->engine.currentChapter() + offset;
		if (chapter < 0)
			d->engine.seek(0);
		else
			d->engine.setCurrentChapter(chapter);
	};
	connect(play("chapter")["prev"], &QAction::triggered, [seekChapter] () { seekChapter(-1); });
	connect(play("chapter")["next"], &QAction::triggered, [seekChapter] () { seekChapter(+1); });

	Menu &video = d->menu("video");
	connect(video("track").g(), &ActionGroup::triggered, [this] (QAction *a) {
		a->setChecked(true); d->engine.setCurrentVideoStream(a->data().toInt()); showMessage(tr("Current Video Track"), a->text());
	});
	d->connectEnumActions<VideoRatio>(video("aspect"), "video_aspect_ratio", &AppState::videoAspectRatioChanged, [this] () {
		d->renderer.setAspectRatio(VideoRatioInfo::data(d->as.video_aspect_ratio));
	});
	d->connectEnumActions<VideoRatio>(video("crop"), "video_crop_ratio", &AppState::videoCropRatioChanged, [this] () {
		d->renderer.setCropRatio(VideoRatioInfo::data(d->as.video_crop_ratio));
	});
	connect(video["snapshot"], &QAction::triggered, [this] () {
		static SnapshotDialog *dlg = new SnapshotDialog(this);
		dlg->setVideoRenderer(&d->renderer); dlg->setSubtitleRenderer(&d->subtitle); dlg->take();
		if (!dlg->isVisible()) {dlg->adjustSize(); dlg->show();}
	});
	auto setVideoAlignment = [this] () { d->renderer.setAlignment(VerticalAlignmentInfo::data(d->as.video_vertical_alignment) | HorizontalAlignmentInfo::data(d->as.video_horizontal_alignment)); };
	d->connectEnumActions<VerticalAlignment>(video("align"), "video_vertical_alignment", &AppState::videoVerticalAlignmentChanged, setVideoAlignment);
	d->connectEnumActions<HorizontalAlignment>(video("align"), "video_horizontal_alignment", &AppState::videoHorizontalAlignmentChanged, setVideoAlignment);
	d->connectEnumDataActions<MoveToward>(video("move")
		, "video_offset", [this] (const QPoint &diff) { return diff.isNull() ? diff : d->as.video_offset + diff; }
		, &AppState::videoOffsetChanged, [this] () { d->renderer.setOffset(d->as.video_offset); });
	d->connectEnumMenu<DeintMode>(video, "video_deinterlacing", &AppState::videoDeinterlacingChanged, [this] () {
		d->engine.setDeintMode(d->as.video_deinterlacing);
	});
	d->connectEnumActions<InterpolatorType>(video("interpolator"), "video_interpolator", &AppState::videoInterpolatorChanged, [this] () {
		d->renderer.setInterpolator(d->as.video_interpolator);
	});
	d->connectEnumActions<InterpolatorType>(video("chroma-upscaler"), "video_chroma_upscaler", &AppState::videoChromaUpscalerChanged, [this] () {
		d->renderer.setChromaUpscaler(d->as.video_chroma_upscaler);
	});
	d->connectEnumMenu<Dithering>(video, "video_dithering", &AppState::videoDitheringChanged, [this] () {
		d->renderer.setDithering(d->as.video_dithering);
	});
	d->connectEnumMenu<ColorRange>(video, "video_range", &AppState::videoRangeChanged, [this] () {
		d->renderer.setRange(d->as.video_range);
	});

	connect(&video("filter"), &Menu::triggered, [this] () {
		VideoRendererItem::Effects effects = 0;
		for (auto act : d->menu("video")("filter").actions()) {
			if (act->isChecked())
				effects |= static_cast<VideoRendererItem::Effect>(act->data().toInt());
		}
		if (d->renderer.effects() != effects)
			d->push(effects, d->renderer.effects(), [this] (VideoRendererItem::Effects effects) {
				d->renderer.setEffects(effects);
				for (auto action : d->menu("video")("filter").actions())
					action->setChecked(action->data().toInt() & effects);
			});
	});
	d->connectEnumDataActions<AdjustColor>(video("color"), "video_color"
		, [this] (const VideoColor &diff) { return diff.isZero() ? diff : d->as.video_color + diff; }, &AppState::videoColorChanged
		, [this] () {
		showMessage(tr("Adjust Video Color"), d->as.video_color.getText(d->as.video_color & d->renderer.color()));
		d->renderer.setColor(d->as.video_color);
	});

	Menu &audio = d->menu("audio");
	connect(audio("track").g(), &ActionGroup::triggered, [this] (QAction *a) {
		a->setChecked(true); d->engine.setCurrentAudioStream(a->data().toInt()); showMessage(tr("Current Audio Track"), a->text());
	});
	d->connectStepActions(audio("volume"), "audio_volume", &AppState::audioVolumeChanged, [this] () {
		auto v = d->as.audio_volume; d->engine.setVolume(v);
	});
	d->connectPropertyCheckable(audio("volume")["mute"], "audio_muted", &AppState::audioMutedChanged, [this] () {
		d->engine.setMuted(d->as.audio_muted);
	});
	d->connectStepActions(audio("sync"), "audio_sync", &AppState::audioSyncChanged, [this] () {
		auto v = d->as.audio_sync; d->engine.setAudioSync(v);
	});
	d->connectStepActions(audio("amp"), "audio_amp", &AppState::audioAmpChanged, [this] () {
		auto v = d->as.audio_amplifier; d->engine.setAmp(v*1e-2);
	});
	d->connectEnumMenu<ChannelLayout>(audio, "audio_channel_layout", &AppState::audioChannelLayoutChanged, [this] () {
		d->engine.setChannelLayout(d->as.audio_channel_layout);
	});
	d->connectPropertyCheckable(audio["normalizer"], "audio_volume_normalizer", &AppState::audioVolumeNormalizerChanged, [this] () {
		d->engine.setVolumeNormalizerActivated(d->as.audio_volume_normalizer);
	});
	d->connectPropertyCheckable(audio["tempo-scaler"], "audio_tempo_scaler", &AppState::audioTempoScalerChanged, [this] () {
		d->engine.setTempoScalerActivated(d->as.audio_tempo_scaler);
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
		showMessage(tr("Select All Subtitles"), tr("%1 Subtitle(s)").arg(d->subtitle.componentsCount()));
	});
	connect(sub("track")["hide"], &QAction::triggered, [this] (bool hide) {
		if (hide != d->subtitle.isHidden()) {
			d->push(hide, d->subtitle.isHidden(), [this] (bool hide) {
				d->subtitle.setHidden(hide);
				d->engine.setSubtitleStreamsVisible(!hide);
				if (hide)
					showMessage(tr("Hide Subtitles"));
				else
					showMessage(tr("Show Subtitles"));
				d->menu("subtitle")("track")["hide"]->setChecked(hide);
			});
		}
	});
	connect(sub("track")["open"], &QAction::triggered, [this] () {
		const QString filter = tr("Subtitle Files") % ' ' % Info::subtitleExt().toFilter();
		const auto dir = d->engine.mrl().isLocalFile() ? QFileInfo(d->engine.mrl().toLocalFile()).absolutePath() : _L("");
		QString enc = d->pref().sub_enc;
		const auto files = EncodingFileDialog::getOpenFileNames(this, tr("Open Subtitle"), dir, filter, &enc);
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
	d->connectEnumMenu<SubtitleDisplay>(sub, "sub_display", &AppState::subDisplayChanged, [this] () {
		d->renderer.setOverlayOnLetterbox(d->as.sub_display == SubtitleDisplay::OnLetterbox);
	});
	d->connectEnumActions<VerticalAlignment>(sub("align"), "sub_alignment", &AppState::subAlignmentChanged, [this] () {
		d->subtitle.setTopAligned(d->as.sub_alignment == VerticalAlignment::Top);
	});
	d->connectStepActions(sub("position"), "sub_position", &AppState::subPositionChanged, [this] () {
		d->subtitle.setPos(d->as.sub_position*1e-2);
	});
	d->connectStepActions(sub("sync"), "sub_sync", &AppState::subSyncChanged, [this] () {
		d->subtitle.setDelay(d->as.sub_sync);
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

	auto &history = tool("history");
	connect(history["toggle"], &QAction::triggered, [toggleTool] () {toggleTool("history", AppState::get().history_visible);});
	connect(history["clear"], &QAction::triggered, [this] () { d->history.clear(); });

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
	connect(tool["auto-exit"], &QAction::triggered, [this] (bool on) {
		if (on != AppState::get().auto_exit)
			d->push(on, AppState::get().auto_exit, [this] (bool on) {
				AppState::get().auto_exit = on;
				showMessage(on ? tr("Exit CMPlayer when the playlist has finished.") : tr("Auto-exit is canceled."));
				d->menu("tool")["auto-exit"]->setChecked(on);
			});
	});
	connect(tool["auto-shutdown"], &QAction::toggled, [this] (bool on) {
		if (on) {
			if (QMessageBox::warning(nullptr, tr("Auto-shutdown")
					, tr("The system will shut down when the play list has finished.")
					, QMessageBox::Ok | QMessageBox::Cancel) == QMessageBox::Cancel) {
				d->menu("tool")["auto-shutdown"]->setChecked(false);
			} else
				showMessage(tr("The system will shut down when the play list has finished."));
		} else
			showMessage(tr("Auto-shutdown is canceled."));
	});

	Menu &win = d->menu("window");		Menu &help = d->menu("help");
	d->connectEnumMenu<StaysOnTop>(win, "window_stays_on_top", &AppState::windowStaysOnTopChanged, [this] () { updateStaysOnTop(); });
	connect(win.g("size"), &ActionGroup::triggered, [this] (QAction *a) {setVideoSize(a->data().toDouble());});
	connect(win["minimize"], &QAction::triggered, this, &MainWindow::showMinimized);
	connect(win["maximize"], &QAction::triggered, this, &MainWindow::showMaximized);
	connect(win["close"], &QAction::triggered, [this] () { d->menu.hide(); close(); });

	connect(help["about"], &QAction::triggered, [this] () {AboutDialog dlg(this); dlg.exec();});
	connect(d->menu["exit"], &QAction::triggered, this, &MainWindow::exit);

	d->connectCurrentStreamActions(&d->menu("play")("title"), &PlayEngine::currentDvdTitle);
	d->connectCurrentStreamActions(&d->menu("play")("chapter"), &PlayEngine::currentChapter);
	d->connectCurrentStreamActions(&d->menu("audio")("track"), &PlayEngine::currentAudioStream);
	d->connectCurrentStreamActions(&d->menu("video")("track"), &PlayEngine::currentVideoStream);
	d->connectCurrentStreamActions(&d->menu("subtitle")("track"), &PlayEngine::currentSubtitleStream);
}

Playlist MainWindow::generatePlaylist(const Mrl &mrl) const {
	if (!mrl.isLocalFile() || !d->pref().enable_generate_playist)
		return Playlist(mrl);
	const auto mode = d->pref().generate_playlist;
	const QFileInfo file(mrl.toLocalFile());
	const QDir dir = file.dir();
	if (mode == GeneratePlaylist::Folder)
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
		Q_ASSERT(sprt->isSeparator());
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
		act->setData(list[i].location());
		act->setText(list[i].displayName());
		act->setVisible(!list[i].isEmpty());
//		qDebug() << act->isVisible();
	}
	recent.syncActions();
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
	if (!d->dontShowMsg)
		d->showOSD(message);
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
	const auto full = isFullScreen();
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
			static Qt::WindowFlags flags = windowFlags();
			static QRect geometry;
			if (full) {
				auto desktop = cApp.desktop();
				const int screen = desktop->screenNumber(this);
				if (screen >= 0) {
					flags = windowFlags();
					geometry = this->geometry();
					setWindowFlags(flags | Qt::FramelessWindowHint);
					SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
					show();
					setGeometry(QRect(QPoint(0, 0), desktop->screenGeometry(this).size()));
				}
			} else {
				setWindowFlags(flags);
				setGeometry(geometry);
				SetSystemUIMode(kUIModeNormal, 0);
			}
			checkWindowState();
			updateTitle();
			updateStaysOnTop();
		} else
#endif
			setWindowState(full ? Qt::WindowFullScreen : (d->prevWinState & ~(Qt::WindowMinimized | Qt::WindowFullScreen)));
	}
	d->dontPause = false;
}

bool MainWindow::isFullScreen() const {return d->fullScreen || QWidget::isFullScreen();}

void MainWindow::setVideoSize(double rate) {
	if (rate < 0) {
		setFullScreen(!isFullScreen());
	} else {
		if (isFullScreen())
			setFullScreen(false);
		if (isMaximized())
			showNormal();
		// patched by Handrake
		const QSizeF video = d->renderer.sizeHint();
		const QSizeF desktop = window()->windowHandle()->screen()->availableVirtualSize();
		const double target = 0.15;
		if (rate == 0.0)
			rate = desktop.width()*desktop.height()*target/(video.width()*video.height());
		d->setVideoSize((video*qSqrt(rate)).toSize());
		return;
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
		const auto &info = d->pref().wheel_scroll_map[event->modifiers()];
		const bool up = event->delta() > 0;
		if (auto action = d->menu.wheelScrollAction(info, d->pref().invert_wheel ? !up : up)) {
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
	d->player = nullptr;
	d->view->engine()->clearComponentCache();
	d->view->rootContext()->setContextProperty("engine", &d->engine);
	d->view->rootContext()->setContextProperty("history", &d->history);
	d->view->rootContext()->setContextProperty("playlist", &d->playlist);
	Skin::apply(d->view, d->pref().skin_name);
	if (d->view->status() == QQuickView::Error) {
//		auto errors = d->view->errors();
//		for (auto error : errors)
//			qDebug() << error.toString();
		d->view->setSource(QUrl("qrc:/emptyskin.qml"));
	}
	auto root = d->view->rootObject();
	if (!root)
		return;
	if (root->objectName() == "player")
		d->player = qobject_cast<QQuickItem*>(root);
	if (!d->player)
		d->player = root->findChild<QQuickItem*>("player");
	if (d->player) {
		if (auto item = d->findItem("playlist"))
			item->setProperty("show", AppState::get().playlist_visible);
		if (auto item = d->findItem("history"))
			item->setProperty("show", AppState::get().history_visible);
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
	case PlayEngine::Playing:
	case PlayEngine::Loading:
	case PlayEngine::Paused:
		time = d->engine.time();
		break;
	default:
		break;
	}
	auto &p = d->pref();
	Translator::load(p.locale);
	d->history.setRememberImage(p.remember_image);
	d->engine.setHwAccCodecs(p.enable_hwaccel ? p.hwaccel_codecs : QList<int>());
	d->engine.setVolumeNormalizerOption(p.normalizer_length, p.normalizer_target, p.normalizer_silence, p.normalizer_min, p.normalizer_max);
	d->engine.setImageDuration(p.image_duration);
	d->engine.setChannelLayoutMap(p.channel_manipulation);

	auto conv = [&p] (const DeintCaps &caps) {
		DeintOption option;
		option.method = caps.method();
		option.doubler = caps.doubler();
		if (caps.hwdec()) {
			if (!caps.supports(DeintDevice::GPU) && !caps.supports(DeintDevice::OpenGL))
				return DeintOption();
			if (caps.supports(DeintDevice::GPU) && p.hwdeints.contains(caps.method()))
				option.device = DeintDevice::GPU;
			else
				option.device = DeintDevice::OpenGL;
		} else
			option.device = caps.supports(DeintDevice::OpenGL) ? DeintDevice::OpenGL : DeintDevice::CPU;
		return option;
	};
	const auto deint_swdec = conv(p.deint_swdec);
	const auto deint_hwdec = conv(p.deint_hwdec);
	d->engine.setDeintOptions(deint_swdec, deint_hwdec);
	d->engine.setAudioDriver(p.audio_driver);
	d->engine.setClippingMethod(p.clipping_method);
	d->engine.setMinimumCache(p.cache_min_playback, p.cache_min_seeking);
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

	if (d->tray)
		d->tray->setVisible(p.enable_system_tray);
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
		if (auto action = RootMenu::instance().action(QKeySequence(event->key() + (event->modifiers() & modMask))))
			action->trigger();
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
	if (d->tray && d->pref().enable_system_tray && d->pref().hide_rather_close) {
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
	const auto id = d->as.window_stays_on_top;
	bool onTop = false;
	if (!isFullScreen()) {
		if (id == StaysOnTop::Always)
			onTop = true;
		else if (id == StaysOnTop::None)
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
	cApp.setWindowTitle(this, fileName);
}

void MainWindow::updateMrl(const Mrl &mrl) {
	if (mrl.isLocalFile()) {
		const auto &p = d->pref();
		auto autoselection = [this, &mrl, &p] (const QList<SubComp> &loaded) {
			QList<int> selected;
			if (loaded.isEmpty() || !mrl.isLocalFile() || !p.sub_enable_autoselect)
				return selected;

			QSet<QString> langSet;
			const QString base = QFileInfo(mrl.toLocalFile()).completeBaseName();
			for (int i=0; i<loaded.size(); ++i) {
				bool select = false;
				if (p.sub_autoselect == SubtitleAutoselect::Matched) {
					select = QFileInfo(loaded[i].fileName()).completeBaseName() == base;
				} else if (p.sub_autoselect == SubtitleAutoselect::All) {
					select = true;
				} else if (p.sub_autoselect == SubtitleAutoselect::EachLanguage) {
		//			const QString lang = loaded[i].m_comp.language().id();
					const QString lang = loaded[i].language();
					if ((select = (!langSet.contains(lang))))
						langSet.insert(lang);
				}
				if (select)
					selected.append(i);
			}
			if (p.sub_autoselect == SubtitleAutoselect::Matched
					&& !selected.isEmpty() && !p.sub_ext.isEmpty()) {
				for (int i=0; i<selected.size(); ++i) {
					const QString fileName = loaded[selected[i]].fileName();
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
			QList<SubComp> loaded;
			if (!p.sub_enable_autoload)
				return loaded;
			const QStringList filter = Info::subtitleNameFilter();
			const QFileInfo fileInfo(mrl.toLocalFile());
			const QFileInfoList all = fileInfo.dir().entryInfoList(filter, QDir::Files, QDir::Name);
			const QString base = fileInfo.completeBaseName();
			for (int i=0; i<all.size(); ++i) {
				if (p.sub_autoload != SubtitleAutoload::Folder) {
					if (p.sub_autoload == SubtitleAutoload::Matched) {
						if (base != all[i].completeBaseName())
							continue;
					} else if (!all[i].fileName().contains(base))
						continue;
				}
				Subtitle sub;
				if (load(sub, all[i].absoluteFilePath(), p.sub_enc)) {
					for (int i=0; i<sub.size(); ++i)
						loaded.push_back(sub[i]);
				}
			}
			if (autoselect) {
				const QList<int> selected = autoselection(loaded);
				for (int i=0; i<selected.size(); ++i)
					loaded[selected[i]].selection() = true;
			}
			return loaded;
		};
		d->subtitle.setComponents(autoload(true));
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
	if (sub.load(fileName, encoding, d->pref().sub_enc_autodetection ? d->pref().sub_enc_accuracy*0.01 : -1.0))
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
		dlg.setCheckBoxText(tr("Don't ask again"));
		dlg.setWindowTitle(tr("Resume Playback"));
		d->startFromStopped = dlg.exec() == QDialogButtonBox::Yes;
		if (_Change(d->preferences.ask_record_found, !dlg.isChecked()))
			d->preferences.save();
	}
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

