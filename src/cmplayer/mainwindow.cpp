#include "overlay.hpp"
#include "stdafx.hpp"
#include "app.hpp"
#include "avmisc.hpp"
#include "colorproperty.hpp"
#include "playlistview.hpp"
#include "historyview.hpp"
#include "playlistmodel.hpp"
#include "subtitlerenderer.hpp"
#include "charsetdetector.hpp"
#include "snapshotdialog.hpp"
#include "subtitle_parser.hpp"
#include "audiocontroller.hpp"
#include "recentinfo.hpp"
#include "abrepeater.hpp"
#include "mainwindow.hpp"
#include "playengine.hpp"
#include "translator.hpp"
#include "videorendereritem.hpp"
#include "playeritem.hpp"

#include "appstate.hpp"
#include "pref.hpp"
#include "playlist.hpp"
#include "dialogs.hpp"
#include "toolbox.hpp"
#include "rootmenu.hpp"
#include "subtitlerendereritem.hpp"
#include "info.hpp"
#include "skin.hpp"
#include "prefdialog.hpp"
#include "playinfoitem.hpp"

struct MainWindow::Data {
	PlayerItem *player = nullptr;
	RootMenu &menu = RootMenu::get();
	RecentInfo &recent = RecentInfo::get();
	const Pref &p = Pref::get();

	PlayEngine &engine = PlayEngine::get();
	AudioController *audio = nullptr;
	VideoRendererItem *video = nullptr;
	SubtitleRenderer subtitle;
	QPoint prevPos;		QTimer hider;
	Qt::WindowState winState = Qt::WindowNoState, prevWinState = Qt::WindowNoState;
	bool moving = false, changingSub = false;
	bool pausedByHiding = false, dontShowMsg = false, dontPause = false;
	bool stateChanging = false;
	ABRepeater ab = {&engine, &subtitle};
	QMenu *contextMenu = nullptr;
	PlaylistView *playlist;
	HistoryView *history;
	QRect screenRect;
//	FavoritesView *favorite;
#ifndef Q_OS_MAC
	QSystemTrayIcon *tray;
#endif
	QString filePath;
// methods
	bool checkAndPlay(const Mrl &mrl) {
		if (mrl != engine.mrl())
			return false;
		if (!engine.isPlaying())
			engine.play();
		return true;
	}

	void openWith(const Pref::OpenMedia &mode, const Mrl &mrl) {
		auto d = this;
		if (d->checkAndPlay(mrl))
			return;
		Playlist playlist;
		if (mode.playlist_behavior == Enum::PlaylistBehaviorWhenOpenMedia::AppendToPlaylist) {
			playlist.append(mrl);
		} else if (mode.playlist_behavior == Enum::PlaylistBehaviorWhenOpenMedia::ClearAndAppendToPlaylist) {
			d->playlist->clear();
			playlist.append(mrl);
		} else if (mode.playlist_behavior == Enum::PlaylistBehaviorWhenOpenMedia::ClearAndGenerateNewPlaylist) {
			d->playlist->clear();
			playlist = PlaylistView::generatePlaylist(mrl);
		} else
			return;
		d->playlist->merge(playlist);
		d->engine.setMrl(mrl, RecentInfo::get().askStartTime(mrl), mode.start_playback);
		if (!mrl.isDvd())
			RecentInfo::get().stack(mrl);
	}

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

		engine.setSpeed(as.play_speed);

		menu("video")("aspect").g()->trigger(as.video_aspect_ratio);
		menu("video")("crop").g()->trigger(as.video_crop_ratio);
		menu("video")("align").g("horizontal")->trigger(as.video_alignment.id() & 0x0f);
		menu("video")("align").g("vertical")->trigger(as.video_alignment.id() & 0xf0);
		video->setOffset(as.video_offset);
		video->setEffects((VideoRendererItem::Effects)as.video_effects);
		for (int i=0; i<16; ++i) {
			if ((as.video_effects >> i) & 1)
				menu("video")("filter").g()->setChecked(1 << i, true);
		}
		video->setColor(as.video_color);

		if (audio) {
			audio->setVolume(as.audio_volume);
			audio->setMuted(as.audio_muted);
			audio->setPreAmp(as.audio_amp);
			audio->setVolumeNormalized(as.audio_volume_normalized);
		}
		menu("subtitle").g("display")->trigger((int)as.sub_letterbox);
		menu("subtitle").g("align")->trigger((int)as.sub_align_top);
		subtitle.setPos(as.sub_pos);
		subtitle.setDelay(as.sub_sync_delay);

		menu("tool")["auto-exit"]->setChecked(as.auto_exit);

		menu("window").g("sot")->trigger(as.screen_stays_on_top.id());

		dontShowMsg = false;
	}

	void save_state() const {
		AppState &as = AppState::get();
		if (video) {
			as.video_aspect_ratio = video->aspectRatio();
			as.video_crop_ratio = video->cropRatio();
			as.video_alignment = video->alignment();
			as.video_offset = video->offset();
			as.video_effects = video->effects();
			as.video_color = video->color();
		}
		if (audio) {
			as.audio_volume = audio->volume();
			as.audio_volume_normalized = audio->isVolumeNormalized();
			as.audio_muted = audio->isMuted();
			as.audio_amp = audio->preAmp();
		}
		as.play_speed = engine.speed();
		as.sub_pos = subtitle.pos();
		as.sub_sync_delay = subtitle.delay();
		as.screen_stays_on_top = stay_on_top_mode();
//		as.sub_letterbox = subtitle.osd().letterboxHint();
		as.sub_align_top = subtitle.isTopAligned();
		as.save();
	}

	Enum::StaysOnTop stay_on_top_mode() const {
		const int id = menu("window").g("sot")->checkedAction()->data().toInt();
		return Enum::StaysOnTop::from(id, Enum::StaysOnTop::Playing);
	}
};

#ifdef Q_OS_MAC
void qt_mac_set_dock_menu(QMenu *menu);
#endif



MainWindow::MainWindow(): d(new Data) {
	setFlags(flags() | Qt::WindowFullscreenButtonHint);
	setResizeMode(QQuickView::SizeRootObjectToView);

	engine()->addImportPath("/Users/xylosper/dev/cmplayer/src/cmplayer/skins/simple");
	setSource(QUrl::fromLocalFile("/Users/xylosper/dev/cmplayer/src/cmplayer/skins/simple/main.qml"));

	d->player = rootObject()->findChild<PlayerItem*>();
	d->video = d->player->video();
	d->audio = d->player->audio();
	d->subtitle.setItem(d->video->subtitle());
	setVideoSize(1.0);
	d->playlist = new PlaylistView(&d->engine, nullptr);
	d->history = new HistoryView(&d->engine, nullptr);
#ifndef Q_OS_MAC
	d->tray = new QSystemTrayIcon(app()->defaultIcon(), this);
#endif
	d->hider.setSingleShot(true);
	d->player->plug(&d->engine);

	Menu &open = d->menu("open");		Menu &play = d->menu("play");
	Menu &video = d->menu("video");		Menu &audio = d->menu("audio");
	Menu &sub = d->menu("subtitle");	Menu &tool = d->menu("tool");
	Menu &win = d->menu("window");		Menu &help = d->menu("help");

	connect(open["file"], &QAction::triggered, [this] () {
		AppState &as = AppState::get();
		const QString filter = Info::mediaExtFilter();
		const QString dir = QFileInfo(as.open_last_file).absolutePath();
		const QString file = getOpenFileName(nullptr, tr("Open File"), dir, filter);
		if (!file.isEmpty()) {openMrl(Mrl(file)); as.open_last_file = file;}
	});
	connect(open["url"], &QAction::triggered, [this] () {
		GetUrlDialog dlg; if (dlg.exec()) {openMrl(dlg.url().toString(), dlg.encoding());}
	});
	connect(open["dvd"], &QAction::triggered, [this] () {
		OpenDvdDialog dlg; dlg.setDevices(app()->devices());
		if (dlg.exec()) {openMrl(Mrl(_L("dvdnav://") % (dlg.device().isEmpty() ? QString("") : ("/" % dlg.device()))));}
	});
	connect(open("recent").g(), &ActionGroup::triggered, [this] (QAction *a) {openMrl(Mrl(a->data().toString()));});
	connect(open("recent")["clear"], &QAction::triggered, &d->recent, &RecentInfo::clear);

	connect(play["stop"], &QAction::triggered, [this] () {d->engine.stop();});
	connect(play("speed").g(), &ActionGroup::triggered, [this] (QAction *a) {
		const int diff = a->data().toInt();
		d->engine.setSpeed(qBound(0.1, diff ? (d->engine.speed() + diff/100.0) : 1, 10.0));
		showMessage(tr("Speed"), QString::fromUtf8("\303\227%1").arg(d->engine.speed()));
	});
	connect(play["pause"], &QAction::toggled, [this] (bool pause) {
		if (!d->stateChanging) {if (pause) d->engine.pause(); else d->engine.play();}
	});
	connect(play("repeat").g(), &ActionGroup::triggered, [this] (QAction *a) {
		const int key = a->data().toInt();
		auto msg = [this] (const QString &ex) {showMessage(tr("A-B Repeat"), ex);};
		if (key == 'r') {
			if (d->engine.state() == State::Stopped) return;
			if (!d->ab.hasA()) msg(tr("Set A to %1").arg(chopped(msecToString(d->ab.setAToCurrentTime(), "h:mm:ss.zzz"), 2)));
			else if (!d->ab.hasB()) {
				const int at = d->ab.setBToCurrentTime();
				if ((at - d->ab.a()) < 100) {d->ab.setB(-1); msg(tr("Range is too short!"));}
				else {d->ab.start(); msg(tr("Set B to %1. Start to repeat!").arg(chopped(msecToString(at, "h:mm:ss.zzz"), 2)));}
			}
		} else if (key == 'q') {d->ab.stop(); d->ab.setA(-1); d->ab.setB(-1); msg(tr("Quit repeating"));}
		else if (key == 's') {d->ab.setAToSubtitleTime(); d->ab.setBToSubtitleTime(); d->ab.start(); msg(tr("Repeat current subtitle"));}
	});
	connect(play["prev"], &QAction::triggered, d->playlist, &PlaylistView::playPrevious);
	connect(play["next"], &QAction::triggered, d->playlist, &PlaylistView::playNext);
	connect(play("seek").g("relative"), &ActionGroup::triggered, [this] (QAction *a) {
		const int diff = a->data().toInt();
		if (diff && d->engine.state() == State::Stopped && d->engine.isSeekable()) {
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
		a->setChecked(true); d->engine.setCurrentTitle(a->data().toInt()); showMessage(tr("Current DVD Title"), a->text());
	});
	connect(play("chapter").g(), &ActionGroup::triggered, [this] (QAction *a) {
		a->setChecked(true); d->engine.setCurrentChapter(a->data().toInt()); showMessage(tr("Current Chapter"), a->text());
	});

	connect(video("track").g(), &ActionGroup::triggered, [this] (QAction *a) {
		a->setChecked(true); d->video->setCurrentStream(a->data().toInt()); showMessage(tr("Current Video Track"), a->text());
	});
	connect(video("aspect").g(), &ActionGroup::triggered, [this] (QAction *a) {d->video->setAspectRatio(a->data().toDouble());});
	connect(video("crop").g(), &ActionGroup::triggered, [this] (QAction *a) {d->video->setCropRatio(a->data().toDouble());});
	connect(video["snapshot"], &QAction::triggered, [this] () {
		//	static SnapshotDialog *dlg = new SnapshotDialog(this);
		//	dlg->setVideoRenderer(d->video); dlg->setSubtitleRenderer(&d->subtitle); dlg->take();
		//	if (!dlg->isVisible()) {dlg->adjustSize(); dlg->show();}
	});
	connect(video["drop-frame"], &QAction::toggled, [this] (bool enabled) {
		d->engine.setFrameDroppingEnabled(enabled); showMessage(tr("Drop Frame"), enabled);
	});
	connect(&video("align"), &Menu::triggered, [this] () {
		int key = 0;
		for (auto a : d->menu("video")("align").actions()) {if (a->isChecked()) key |= a->data().toInt();}
		d->video->setAlignment(key);
	});

	connect(&video("move"), &Menu::triggered, [this] (QAction *action) {
		const int move = action->data().toInt();
		if (move == Qt::NoArrow) {
			d->video->setOffset(QPoint(0, 0));
		} else {
			const double x = move == Qt::LeftArrow ? -1 : (move == Qt::RightArrow ? 1 : 0);
			const double y = move == Qt::UpArrow ? -1 : (move == Qt::DownArrow ? 1 : 0);
			d->video->setOffset(d->video->offset() += QPoint(x, y));
		}
	});
	connect(&video("filter"), &Menu::triggered, [this] () {
		VideoRendererItem::Effects effects = 0;
		for (auto act : d->menu("video")("filter").actions()) {
			if (act->isChecked()) effects |= static_cast<VideoRendererItem::Effect>(act->data().toInt());
		} d->video->setEffects(effects);
	});
	connect(video("color").g(), &ActionGroup::triggered, [this] (QAction *action) {
		const auto data = action->data().toList();
		const auto prop = ColorProperty::Value(data[0].toInt());
		if (prop == ColorProperty::PropMax) {
			d->video->setColor(ColorProperty());
			showMessage(tr("Reset brightness, contrast, saturation and hue"));
		} else {
			QString cmd;
			switch(prop) {
			case ColorProperty::Brightness: cmd = tr("Brightness"); break;
			case ColorProperty::Saturation: cmd = tr("Saturation"); break;
			case ColorProperty::Hue: cmd = tr("Hue"); break;
			case ColorProperty::Contrast: cmd = tr("Contrast"); break;
			default: return;}
			ColorProperty color = d->video->color();
			color.setValue(prop, color.value(prop) + data[1].toInt()*0.01);
			d->video->setColor(color);
			showMessage(cmd, qRound(d->video->color()[prop]*100.0), "%", true);
		}
	});

	connect(audio("track").g(), &ActionGroup::triggered, [this] (QAction *a) {
		a->setChecked(true); d->audio->setCurrentStream(a->data().toInt()); showMessage(tr("Current Audio Track"), a->text());
	});
	connect(audio.g("volume"), &ActionGroup::triggered, [this] (QAction *a) {
		const int diff = a->data().toInt();
		if (diff) {
			const int volume = qBound(0, d->audio->volume() + diff, 100);
			d->audio->setVolume(volume); showMessage(tr("Volume"), volume, "%");
		}
	});
	connect(audio["mute"], &QAction::toggled, [this] (bool on) {d->audio->setMuted(on); showMessage(tr("Mute"), on);});
	connect(audio.g("amp"), &ActionGroup::triggered, [this] (QAction *a) {
		const int amp = qBound(0, qRound(d->audio->preAmp()*100 + a->data().toInt()), 1000);
		d->audio->setPreAmp(amp*0.01); showMessage(tr("Amp"), amp, "%");
	});
	connect(audio["volnorm"], &QAction::toggled, [this] (bool on) {
		d->audio->setVolumeNormalized(on); showMessage(tr("Normalize Volume"), on);
	});
	connect(audio["scale-tempo"], &QAction::toggled, [this] (bool on) {
		d->audio->setTempoScaled(on); showMessage(tr("Auto-scale tempo"), on);
	});

	connect(sub("list")["hide"], &QAction::toggled, &d->subtitle, &SubtitleRenderer::setHidden);
	connect(sub("list")["open"], &QAction::triggered, [this] () {
		const auto filter = tr("Subtitle Files") % ' ' % Info::subtitleExt().toFilter();
		const auto dir = d->engine.mrl().isLocalFile() ? QFileInfo(d->engine.mrl().toLocalFile()).absolutePath() : _L("");
		QString enc = d->p.sub_enc;
		const auto files = EncodingFileDialog::getOpenFileNames(nullptr, tr("Open Subtitle"), dir, filter, &enc);
		if (!files.isEmpty()) appendSubFiles(files, true, enc);
	});
	connect(sub("list")["clear"], &QAction::triggered, [this] () {
		d->subtitle.unload();
		qDeleteAll(d->menu("subtitle")("list").g()->actions());
	});
	connect(sub("list").g(), &ActionGroup::triggered, [this] (QAction *a) {
		if (!d->changingSub) {d->subtitle.select(a->data().toInt(), a->isChecked());}
	});
	connect(sub("spu").g(), &ActionGroup::triggered, [this] (QAction *a) {
		a->setChecked(true); d->engine.setCurrentSpu(a->data().toInt());
		showMessage(tr("Current Subtitle Track"), a->text());
	});
	connect(sub.g("display"), &ActionGroup::triggered, [this] (QAction *a) {a->data().toInt();/*d->subtitle.osd().setLetterboxHint(data);*/});
	connect(sub.g("align"), &ActionGroup::triggered, [this] (QAction *a) {d->subtitle.setTopAlignment(a->data().toInt());});
	connect(sub.g("pos"), &ActionGroup::triggered, [this] (QAction *a) {
		const int pos = qBound(0, qRound(d->subtitle.pos()*100.0 + a->data().toInt()), 100);
		d->subtitle.setPos(pos*0.01); showMessage(tr("Subtitle Position"), pos, "%");
	});
	connect(sub.g("sync"), &ActionGroup::triggered, [this] (QAction *a) {
		const int diff = a->data().toInt(); const int delay = diff ? d->subtitle.delay() + diff : 0;
		d->subtitle.setDelay(delay); showMessage("Subtitle Sync", delay*0.001, "sec", true);
	});

	connect(tool["playlist"], &QAction::triggered, d->playlist, &PlaylistView::toggle);
	connect(tool["history"], &QAction::triggered, d->history, &HistoryView::toggle);
	connect(tool["subtitle"], &QAction::triggered, static_cast<ToggleDialog*>(d->subtitle.view()), &ToggleDialog::toggle);
	connect(tool["pref"], &QAction::triggered, [this] () {static PrefDialog *dlg = nullptr;
		if (!dlg) {dlg = new PrefDialog; connect(dlg, SIGNAL(applicationRequested()), this, SLOT(applyPref()));} dlg->show();
	});
	connect(tool["playinfo"], &QAction::toggled, [this](bool v) {d->player->setInfoVisible(v);});
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



	connect(win.g("sot"), &ActionGroup::triggered, this, &MainWindow::updateStaysOnTop);
	connect(win.g("size"), &ActionGroup::triggered, [this] (QAction *a) {setVideoSize(a->data().toDouble());});
	connect(win["minimize"], &QAction::triggered, this, &MainWindow::showMinimized);
	connect(win["maximize"], &QAction::triggered, this, &MainWindow::showMaximized);
	connect(win["close"], &QAction::triggered, this, &MainWindow::close);

	connect(help["about"], &QAction::triggered, [this] () {AboutDialog dlg; dlg.exec();});
	connect(d->menu["exit"], &QAction::triggered, this, &MainWindow::exit);

	connect(&play, &Menu::aboutToShow, [this] () {
		Menu &menu = d->menu("play");
		menu("title").setEnabled(d->engine.titles().size() > 0);
		menu("chapter").setEnabled(d->engine.chapters().size() > 0);
	});
	connect(&play("title"), &Menu::aboutToShow, [this] () {
		const auto titles = d->engine.titles();
		Menu &menu = d->menu("play")("title"); menu.g()->clear();
		if (!titles.isEmpty()) {
			const int current = d->engine.currentTitleId();
			for (auto it = titles.begin(); it != titles.end(); ++it) {
				auto act = menu.addActionToGroupWithoutKey(it->name, true);
				act->setData(it.key());
				if (current == it.key()) act->setChecked(true);
			}
		}
	});
	connect(&play("chapter"), &Menu::aboutToShow, [this] () {
		const auto chapters = d->engine.chapters();
		Menu &menu = d->menu("play")("chapter"); menu.g()->clear();
		if (!chapters.isEmpty()) {
			const int current = d->engine.currentChapterId();
			for (int i=0; i<chapters.size(); ++i) {
				auto act = menu.addActionToGroupWithoutKey(chapters[i], true);
				act->setData(i);
				if (i == current) act->setChecked(true);
			}
		}
	});

	auto checkStreamMenu = [this] (Menu &menu, const StreamList &streams, int current, const QString &text) {
		menu.g()->clear();
		if (!streams.isEmpty()) {
			int nth = 1;
			for (auto it = streams.begin(); it != streams.end(); ++it, ++nth) {
				auto atext = text.arg(nth);
				const auto name = it->name();
				if (!name.isEmpty())
					atext += " - " % name;
				auto act = menu.addActionToGroupWithoutKey(atext, true);
				const int id = it.key();
				act->setData(id);
				act->setChecked(current == id);
			}
		}
	};
	connect(&video, &Menu::aboutToShow, [this] () {d->menu("video")("track").setEnabled(d->video->streams().size() > 0);});
	connect(&video("track"), &Menu::aboutToShow, [this, checkStreamMenu] () {
		checkStreamMenu(d->menu("video")("track"), d->video->streams(), d->video->currentStreamId(), tr("Video %1"));
	});
	connect(&audio, &Menu::aboutToShow, [this] () {if (d->audio) d->menu("audio")("track").setEnabled(d->audio->streams().size() > 0);});
	connect(&audio("track"), &Menu::aboutToShow, [this, checkStreamMenu] () {
		if (d->audio) checkStreamMenu(d->menu("audio")("track"), d->audio->streams(), d->audio->currentStreamId(), tr("Audio %1"));
	});

	connect(&sub, &Menu::aboutToShow, [this] () {d->menu("subtitle")("spu").setEnabled(d->engine.spus().size() > 0);});
	connect(&sub("spu"), &Menu::aboutToShow, [this, checkStreamMenu] () {
		checkStreamMenu(d->menu("subtitle")("spu"), d->engine.spus(), d->engine.currentSpuId(), tr("Subtitle %1"));
	});

	connect(&d->engine, &PlayEngine::mrlChanged, this, &MainWindow::updateMrl);
	connect(&d->engine, &PlayEngine::stateChanged, [this] (State state) {
		d->stateChanging = true;
		switch (state) {
		case State::Paused:
		case State::Stopped:
		case State::Finished:
			d->menu("play")["pause"]->setChecked(true);
			d->menu("play")["pause"]->setText(tr("Play"));
			break;
		default:
			d->menu("play")["pause"]->setChecked(false);
			d->menu("play")["pause"]->setText(tr("Pause"));
		}
		app()->setScreensaverDisabled(d->p.disable_screensaver && state == State::Playing);
		updateStaysOnTop();
		d->stateChanging = false;
	});
	connect(&d->engine, &PlayEngine::tick, &d->subtitle, &SubtitleRenderer::render);
//	connect(d->video, formatChanged(VideoFormat), [this] (const VideoFormat &format) {//fps to subtitle
//	connectvoid MainWindow::onScreenSizeChanged(const QSize &size) {
//		showMessage(toString(size));
//	}

	connect(&d->recent, &RecentInfo::openListChanged, this, &MainWindow::updateRecentActions);
	connect(&d->hider, &QTimer::timeout, [this] () {if (cursor().shape() != Qt::BlankCursor) setCursor(Qt::BlankCursor);});
	connect(d->history, &HistoryView::playRequested, [this] (const Mrl &mrl) {openMrl(mrl);});
	connect(d->playlist, &PlaylistView::finished, [this] () {
		if (d->menu("tool")["auto-exit"]->isChecked()) exit();
		if (d->menu("tool")["auto-shutdown"]->isChecked()) app()->shutdown();
	});

#ifndef Q_OS_MAC
	connect(d->tray, activated(QSystemTrayIcon::ActivationReason), this, handleTray(QSystemTrayIcon::ActivationReason));
#endif

	d->load_state();
	applyPref();

	d->playlist->setPlaylist(d->recent.lastPlaylist());
	d->engine.setMrl(d->recent.lastMrl(), d->recent.askStartTime(d->recent.lastMrl()), false);
	updateRecentActions(d->recent.openList());
#ifdef Q_OS_MAC
////	qt_mac_set_dock_menu(&d->menu);
//	QMenuBar *mb = app()->globalMenuBar();
//	mb->addMenu(&d->menu);
//	mb->addMenu(&open);
//	mb->addMenu(&play);
//	mb->addMenu(&video);
//	mb->addMenu(&audio);
//	mb->addMenu(&sub);
//	mb->addMenu(&tool);
//	mb->addMenu(&win);
//	mb->addMenu(&help);
#endif
	// hack for bug of XCB in multithreaded-OpenGL
//#ifdef Q_OS_X11
//	auto dlg = getPrefDialog();
//	dlg->show();
//	QTimer::singleShot(1, dlg, SLOT(hide()));
//#endif
	d->winState = d->prevWinState = windowState();
	connect(this, &MainWindow::windowStateChanged, [this] (Qt::WindowState state) {
		d->dontPause = true;
		d->moving = false;
		d->prevPos = QPoint();
		if (state != d->winState) {
			d->prevWinState = d->winState;
			d->winState = state;
		}
		switch (state) {
		case Qt::WindowFullScreen:
			app()->setAlwaysOnTop(this, false);
			setVisible(true);
			if (d->p.hide_cursor)
				d->hider.start(d->p.hide_cursor_delay);
			break;
		default:
			d->hider.stop();
			if (cursor().shape() == Qt::BlankCursor)
				unsetCursor();
			updateStaysOnTop();
			setVisible(true);
		}
		d->dontPause = false;
		setFilePath(d->filePath);
	});
}

MainWindow::~MainWindow() {
	if (d->player)
		d->player->unplug();
	delete d->contextMenu;
	delete d;
}

void MainWindow::openFromFileManager(const Mrl &mrl) {
	d->openWith(d->p.open_media_from_file_manager, mrl);
}



void MainWindow::exit() {
	static bool done = false;
	if (!done) {
		const auto stop = !d->engine.isStopped() && !d->engine.isFinished();
		const auto mrl = d->engine.mrl();
		const auto pos = d->engine.position();
		const auto duration = d->engine.duration();
		d->engine.quitRunning();
		if (stop)
			d->history->setStopped(mrl, pos, duration);
		d->recent.setLastPlaylist(d->playlist->playlist());
		d->recent.setLastMrl(d->engine.mrl());
		d->save_state();
		app()->quit();
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
		d->playlist->load(mrl, enc);
		if (!mrl.isPlaylist()) {
			d->playlist->play(mrl);
			if (!mrl.isDvd())
				RecentInfo::get().stack(mrl);
		}
	}
}

bool MainWindow::eventFilter(QObject *o, QEvent *e) {
//	if (o == &d->screen && e->type() == QEvent::Resize) {
//		static QSize prev;
//		QSize size = d->screen.size();
////		if (d->skin.screen()) {
////			auto screen = d->skin.screen();
////			size = isFullScreen() ? d->skin.widget()->size() : screen->size();
////			if (isFullScreen() && d->screenRect.isEmpty()) {
////				d->screenRect = QRect(screen->mapToGlobal(QPoint(0, 0)), screen->size());
////				d->skin.setVisible(false);
////			} else if (!isFullScreen() && !d->screenRect.isEmpty()) {
////				d->screenRect = QRect();
////				d->skin.setVisible(true);
////			}
////		}
//		if (isFullScreen()) {
//			d->video.setFixedRenderSize(size);
//		} else {
//			d->video.setFixedRenderSize(QSize());
//		}
//		if (prev != size) {
//			showMessage(QString::fromUtf8("%1x%2").arg(size.width()).arg(size.height()), 1000);
//			prev = size;
//		}
//	}
	return QQuickView::eventFilter(o, e);
}

void MainWindow::showMessage(const QString &message) {
	if (!d->dontShowMsg && d->player)
		d->player->requestMessage(message);
}

void MainWindow::setFullScreen(bool full) {
	d->dontPause = true;
	qDebug() << d->winState << d->prevWinState;
	setWindowState(full ? Qt::WindowFullScreen : d->prevWinState);
	d->dontPause = false;
}

void MainWindow::setVideoSize(double rate) {
	if (rate < 0) {
		const bool wasFull = isFullScreen();
		setFullScreen(!wasFull);
	} else {
		// patched by Handrake
		const QSizeF video = d->video->sizeHint();
		const QSizeF desktop = screen()->availableVirtualSize();
		const double target = 0.15;
		if (isFullScreen())
			setFullScreen(false);
		if (rate == 0.0)
			rate = desktop.width()*desktop.height()*target/(video.width()*video.height());
		const QSize size = (this->size() - d->video->size() + d->video->sizeHint()*qSqrt(rate)).toSize();
		if (size != this->size()) {
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
				setX(x);
			}
		}
	}
}

void MainWindow::mousePressEvent(QMouseEvent *event) {
	QQuickView::mousePressEvent(event);
	if (event->isAccepted())
		return;
	bool showContextMenu = false;
	switch (event->button()) {
	case Qt::LeftButton:
		if (!isFullScreen()) {
			d->moving = true;
			d->prevPos = event->globalPos();
		}
		break;
	case Qt::MiddleButton:
		if (QAction *action = d->menu.middleClickAction(event->modifiers()))
			action->trigger();
		break;
	case Qt::RightButton:
		showContextMenu = true;
		break;
	default:
		break;
	}
	if (showContextMenu)
		d->contextMenu->exec(event->globalPos());
	else
		d->contextMenu->hide();
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
	d->hider.stop();
	if (cursor().shape() == Qt::BlankCursor)
		unsetCursor();
	QQuickView::mouseMoveEvent(event);
	const bool full = isFullScreen();
	if (full) {
		if (d->moving) {
			d->moving = false;
			d->prevPos = QPoint();
		}
//		d->skin.setVisible(!d->screenRect.isEmpty() && !d->screenRect.contains(event->globalPos()));
		if (d->p.hide_cursor)
			d->hider.start(d->p.hide_cursor_delay);
	} else {
		if (d->moving) {
			const QPoint pos = event->globalPos();
			setPosition(this->position() + pos - d->prevPos);
			d->prevPos = pos;
		}
	}
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
	if (d->moving) {
		d->moving = false;
		d->prevPos = QPoint();
	}
	QQuickView::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event) {
	QQuickView::mouseDoubleClickEvent(event);
	if (!event->isAccepted() && event->button() == Qt::LeftButton) {
		if (QAction *action = d->menu.doubleClickAction(event->modifiers()))
			action->trigger();
	}
}

void MainWindow::wheelEvent(QWheelEvent *event) {
	QQuickView::wheelEvent(event);
	if (!event->isAccepted() && event->delta()) {
		if (QAction *action = d->menu.wheelScrollAction(event->modifiers(), event->delta() > 0)) {
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
	if (!event->mimeData()->hasUrls())
		return;
	QList<QUrl> urls = event->mimeData()->urls();
	if (urls.isEmpty())
		return;
	qSort(urls);
	Playlist playlist;
	QStringList subList;
	for (int i=0; i<urls.size(); ++i) {
		const QString suffix = QFileInfo(urls[i].path()).suffix().toLower();
		if (Info::playlistExt().contains(suffix)) {
			Playlist list;
			list.load(urls[i].toString());
			playlist += list;
		} else if (Info::subtitleExt().contains(suffix)) {
			subList << urls[i].toLocalFile();
		} else if (Info::videoExt().contains(suffix)
				|| Info::audioExt().contains(suffix)) {
			playlist.append(urls[i].toString());
		}
	}
	if (!playlist.isEmpty()) {
		const Pref::OpenMedia &mode = d->p.open_media_by_drag_and_drop;
		const auto mrl = playlist.first();
		if (mode.playlist_behavior != Enum::PlaylistBehaviorWhenOpenMedia::AppendToPlaylist) {
			d->playlist->clear();
			if (mode.playlist_behavior == Enum::PlaylistBehaviorWhenOpenMedia::ClearAndGenerateNewPlaylist && playlist.size() == 1)
				playlist = d->playlist->generatePlaylist(mrl);
		}
		d->playlist->merge(playlist);
		d->engine.setMrl(mrl, d->recent.askStartTime(mrl), mode.start_playback);
	} else if (!subList.isEmpty())
		appendSubFiles(subList, true, d->p.sub_enc);
}

void MainWindow::applyPref() {
	int time = -1;
	switch (d->engine.state()) {
	case State::Playing:
	case State::Buffering:
	case State::Paused:
		time = d->engine.position();
		break;
	default:
		break;
	}
	SubtitleParser::setMsPerCharactor(d->p.ms_per_char);
	Translator::load(d->p.locale);
//		subtitle.osd().setStyle(p.sub_style);
	d->menu.update();
	d->menu.save();
	delete d->contextMenu;
	d->contextMenu = new QMenu;
	d->contextMenu->addMenu(d->menu("open").duplicated(d->contextMenu));
	d->contextMenu->addSeparator();
	d->contextMenu->addMenu(d->menu("play").duplicated(d->contextMenu));
	d->contextMenu->addMenu(d->menu("video").duplicated(d->contextMenu));
	d->contextMenu->addMenu(d->menu("audio").duplicated(d->contextMenu));
	d->contextMenu->addMenu(d->menu("subtitle").duplicated(d->contextMenu));
	d->contextMenu->addSeparator();
	d->contextMenu->addMenu(d->menu("tool").duplicated(d->contextMenu));
	d->contextMenu->addMenu(d->menu("window").duplicated(d->contextMenu));
	d->contextMenu->addSeparator();
	d->contextMenu->addAction(d->menu("help")["about"]);
	d->contextMenu->addAction(d->menu["exit"]);
#ifndef Q_OS_MAC
	tray->setVisible(p.enable_system_tray);
#endif
//	if (d->skin.name() != d->p.skin_name) {
//		d->screen.setParent(0);
//		if (d->skin.load(d->p.skin_name)) {
//			auto screen = d->skin.screen();
//			d->screen.setParent(screen);
//			d->screen.move(0, 0);
//			d->screen.resize(screen->size());
//			d->skin.initializePlaceholders();
//			setCentralWidget(d->skin.widget());
//		} else
//			setCentralWidget(&d->screen);
//	}
	if (time >= 0)
		d->engine.play(time);
}

void MainWindow::showEvent(QShowEvent *event) {
	QQuickView::showEvent(event);
	if (d->pausedByHiding && d->engine.isPaused()) {
		d->engine.play();
		d->pausedByHiding = false;
	}
	setFilePath(d->filePath);
}

void MainWindow::hideEvent(QHideEvent *event) {
	QQuickView::hideEvent(event);
	if (!d->p.pause_minimized || d->dontPause)
		return;
	if (!d->engine.isPlaying() || (d->p.pause_video_only && !d->engine.hasVideo()))
		return;
	d->pausedByHiding = true;
	d->engine.pause();
}

void MainWindow::hideCursor() {
	if (cursor().shape() != Qt::BlankCursor)
		setCursor(Qt::BlankCursor);
}

void MainWindow::handleTray(QSystemTrayIcon::ActivationReason reason) {
	if (reason == QSystemTrayIcon::Trigger)
		setVisible(!isVisible());
	else if (reason == QSystemTrayIcon::Context)
		d->contextMenu->exec(QCursor::pos());
}

void MainWindow::closeEvent(QCloseEvent *event) {
#ifndef Q_OS_MAC
	if (d->p.enable_system_tray && d->p.hide_rather_close) {
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
//	exit();
#endif
}

void MainWindow::updateStaysOnTop() {
	const int id = d->menu("window").g("sot")->checkedAction()->data().toInt();
	bool onTop = false;
	if (!isFullScreen()) {
		if (id == Enum::StaysOnTop::Always)
			onTop = true;
		else if (id == Enum::StaysOnTop::Never)
			onTop = false;
		else
			onTop = d->engine.isPlaying();
	}
	app()->setAlwaysOnTop(this, onTop);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
	QQuickView::keyPressEvent(event);
	if (event->isAccepted())
		return;
	constexpr int modMask = Qt::SHIFT | Qt::CTRL | Qt::ALT | Qt::META;
	auto action = RootMenu::get().action(QKeySequence(event->key() + (event->modifiers() & modMask)));
	if (action) {
		if (action->isCheckable())
			action->toggle();
		else
			action->trigger();
	}
}

auto MainWindow::updateMrl(const Mrl &mrl) -> void {
	QString title;
	if (mrl.isLocalFile()) {
		d->subtitle.autoload(mrl, true);
		const QFileInfo file(mrl.toLocalFile());
		d->filePath = file.absoluteFilePath();
		title += file.fileName();
		if (isVisible())
			setFilePath(d->filePath);
	} else
		clearSubtitles();
	title += _L(" - ") % Info::name() % " " % Info::version();
	setTitle(title);
	d->sync_subtitle_file_menu();
	const int number = d->playlist->model()->currentRow() + 1;
	if (number > 0) {
//		d->skin.setMediaNumber(number);
//		d->skin.setTotalMediaCount(d->playlist->model()->rowCount());
	}
}

void MainWindow::clearSubtitles() {
	d->subtitle.unload();
	qDeleteAll(d->menu("subtitle")("list").g()->actions());
}

void MainWindow::appendSubFiles(const QStringList &files, bool checked, const QString &enc) {
	if (!files.isEmpty()) {
		for (auto file : files)
			d->subtitle.load(file, enc, checked);
		d->sync_subtitle_file_menu();
	}
}
