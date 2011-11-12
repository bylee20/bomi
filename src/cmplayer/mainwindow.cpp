#include "mainwindow_p.hpp"
#include <QtGui/QDesktopWidget>

#ifdef Q_WS_MAC
void qt_mac_set_dock_menu(QMenu *menu);
#endif

MainWindow *MainWindow::obj = 0;

MainWindow::MainWindow() {
	obj = this;
	d = new Data();
	LibVLC::initialize();

	d->dontPause = false;
	d->pausedByHiding = d->dontShowMsg = false;
	d->changingSub = d->moving = false;
	d->engine = LibVLC::engine();
	d->audio = LibVLC::audio();
	d->video = LibVLC::video();
	d->subtitle = new SubtitleRenderer;
	d->timeLine = new TimeLineOsdRenderer;
	d->playInfo = new PlayInfoView;
	d->message = new TextOsdRenderer(Qt::AlignTop | Qt::AlignLeft);
	d->ab = new ABRepeater(d->engine, d->subtitle);
	d->control = d->create_control_widget();
	d->center = d->create_central_widget(d->video, d->control);
	d->hider = new QTimer(this);
	d->playlist = new PlaylistView(d->engine, this);
	d->history = new HistoryView(d->engine, this);
#ifndef Q_WS_MAC
	d->tray = new QSystemTrayIcon(app()->defaultIcon(), this);
#endif

	d->hider->setSingleShot(true);
	d->playInfo->setVideo(d->video);
	d->playInfo->setAudio(d->audio);

	setMouseTracking(true);
	setCentralWidget(d->center);
	setWindowTitle(QString("CMPlayer %1").arg(Info::version()));
	setAcceptDrops(true);
	d->video->setAcceptDrops(false);
	d->center->setAcceptDrops(false);
	d->control->setAcceptDrops(false);

	Menu &open = d->menu("open");		Menu &play = d->menu("play");
	Menu &video = d->menu("video");		Menu &audio = d->menu("audio");
	Menu &sub = d->menu("subtitle");	Menu &tool = d->menu("tool");
	Menu &win = d->menu("window");		Menu &help = d->menu("help");

	CONNECT(open["file"], triggered(), this, openFile());
	CONNECT(open["url"], triggered(), this, openUrl());
	CONNECT(open["dvd"], triggered(), this, openDvd());
	CONNECT(open("recent").g(), triggered(QString), this, openLocation(QString));
	CONNECT(open("recent")["clear"], triggered(), &d->recent, clear());

	CONNECT(play["stop"], triggered(), d->engine, stop());
	CONNECT(play("speed").g(), triggered(int), this, setSpeed(int));
	CONNECT(play["pause"], triggered(), this, togglePlayPause());
	CONNECT(play("repeat").g(), triggered(int), this, doRepeat(int));
	CONNECT(play["prev"], triggered(), d->playlist, playPrevious());
	CONNECT(play["next"], triggered(), d->playlist, playNext());
	CONNECT(play("seek").g("relative"), triggered(int), this, seek(int));
	CONNECT(play("seek").g("subtitle"), triggered(int), this, seekToSubtitle(int));
	CONNECT(play("title").g(), triggered(QAction*), this, setTitle(QAction*));
	CONNECT(play("chapter").g(), triggered(QAction*), this, setChapter(QAction*));

	CONNECT(video("track").g(), triggered(QAction*), this, setVideoTrack(QAction*));
	CONNECT(video("aspect").g(), triggered(double), d->video, setAspectRatio(double));
	CONNECT(video("crop").g(), triggered(double), d->video, setCropRatio(double));
	CONNECT(video["snapshot"], triggered(), this, takeSnapshot());
	CONNECT(video.g("color"), triggered(QAction*), this, setColorProperty(QAction*));
	CONNECT(&video("filter"), triggered(QAction*), this, setEffect(QAction*));
	CONNECT(video("overlay").g(), triggered(int), d->video, setOverlayType(int));
	CONNECT(audio("track").g(), triggered(QAction*), this, setAudioTrack(QAction*));
	CONNECT(audio.g("volume"), triggered(int), this, setVolume(int));
	CONNECT(audio["mute"], toggled(bool), this, setMuted(bool));
	CONNECT(audio.g("amp"), triggered(int), this, setAmp(int));
	CONNECT(audio["volnorm"], toggled(bool), this, setVolumeNormalized(bool));

	CONNECT(sub("list")["hide"], toggled(bool), d->subtitle, setHidden(bool));
	CONNECT(sub("list")["open"], triggered(), this, openSubFile());
	CONNECT(sub("list")["clear"], triggered(), this, clearSubtitles());
	CONNECT(sub("list").g(), triggered(QAction*), this, updateSubtitle(QAction*));
	CONNECT(sub("spu").g(), triggered(QAction*), this, setSPU(QAction*));
	CONNECT(sub.g("display"), triggered(int), this, setSubtitleDisplay(int));
	CONNECT(sub.g("align"), triggered(int), this, setSubtitleAlign(int));
	CONNECT(sub.g("pos"), triggered(int), this, moveSubtitle(int));
	CONNECT(sub.g("sync"), triggered(int), this, setSyncDelay(int));

	CONNECT(tool["playlist"], triggered(), d->playlist, toggle());
	CONNECT(tool["history"], triggered(), d->history, toggle());
	CONNECT(tool["subtitle"], triggered(), d->subtitle->view(), toggle());
	CONNECT(tool["pref"], triggered(), this, setPref());
	CONNECT(tool["playinfo"], toggled(bool), d->playInfo, setVisible(bool));

	CONNECT(win.g("sot"), triggered(int), this, updateStaysOnTop());
	CONNECT(win.g("size"), triggered(double), this, setVideoSize(double));
	CONNECT(win["minimize"], triggered(), this, showMinimized());
	CONNECT(win["maximize"], triggered(), this, maximize());
	CONNECT(win["close"], triggered(), this, close());

	CONNECT(help["about"], triggered(), this, about());
	CONNECT(d->menu["exit"], triggered(), qApp, quit());

	CONNECT(&play, aboutToShow(), this, checkPlayMenu());
	CONNECT(&play("title"), aboutToShow(), this, checkTitleMenu());
	CONNECT(&play("chapter"), aboutToShow(), this, checkChapterMenu());
	CONNECT(&video, aboutToShow(), this, checkVideoMenu());
	CONNECT(&video("track"), aboutToShow(), this, checkVideoTrackMenu());
	CONNECT(&audio, aboutToShow(), this, checkAudioMenu());
	CONNECT(&audio("track"), aboutToShow(), this, checkAudioTrackMenu());
	CONNECT(&sub, aboutToShow(), this, checkSubtitleMenu());
	CONNECT(&sub("spu"), aboutToShow(), this, checkSPUMenu());

	CONNECT(d->engine, mrlChanged(Mrl), this, updateMrl(Mrl));
	CONNECT(d->engine, stateChanged(MediaState,MediaState), this, updateState(MediaState,MediaState));
	CONNECT(d->engine, tick(int), d->subtitle, render(int));
	CONNECT(d->video, customContextMenuRequested(const QPoint&), this, showContextMenu(const QPoint&));
	CONNECT(d->video, formatChanged(VideoFormat), this, updateVideoFormat(VideoFormat));
	CONNECT(d->video, screenSizeChanged(QSize), this, onScreenSizeChanged(QSize));
	CONNECT(d->audio, mutedChanged(bool), audio["mute"], setChecked(bool));
	CONNECT(d->audio, volumeNormalizedChanged(bool), audio["volnorm"], setChecked(bool));

	CONNECT(&d->recent, openListChanged(QList<Mrl>), this, updateRecentActions(QList<Mrl>));
	CONNECT(d->hider, timeout(), this, hideCursor());
	CONNECT(d->history, playRequested(Mrl), this, openMrl(Mrl));
#ifndef Q_WS_MAC
	CONNECT(d->tray, activated(QSystemTrayIcon::ActivationReason)
		, this, handleTray(QSystemTrayIcon::ActivationReason));
#endif

	addActions(d->menu.actions());
	d->context = new QMenu(this);
	d->context->addMenu(&open);
	d->context->addSeparator();
	d->context->addMenu(&play);
	d->context->addMenu(&video);
	d->context->addMenu(&audio);
	d->context->addMenu(&sub);
	d->context->addSeparator();
	d->context->addMenu(&tool);
	d->context->addMenu(&win);
	d->context->addSeparator();
	d->context->addAction(d->menu("help")["about"]);
	d->context->addAction(d->menu["exit"]);

	d->load_state();
	d->menu.load();
	d->apply_pref();


	d->playlist->setPlaylist(d->recent.lastPlaylist());
	d->engine->setMrl(d->recent.lastMrl());
	updateRecentActions(d->recent.openList());

#ifdef Q_WS_MAC
//	qt_mac_set_dock_menu(&d->menu);
	QMenuBar *mb = app()->globalMenuBar();
	mb->addMenu(&open);
	mb->addMenu(&play);
	mb->addMenu(&video);
	mb->addMenu(&audio);
	mb->addMenu(&sub);
	mb->addMenu(&tool);
	mb->addMenu(&win);
	mb->addMenu(&help);
#endif
	d->video->addOsd(d->playInfo->osd());
	d->video->addOsd(d->subtitle->osd());
	d->video->addOsd(d->timeLine);
	d->video->addOsd(d->message);
}

MainWindow::~MainWindow() {
	obj = 0;
	d->video->setParent(0);
	d->engine->stop();
	d->recent.setLastPlaylist(d->playlist->playlist());
	d->recent.setLastMrl(d->engine->mrl());
	d->save_state();
	delete d->subtitle;
	delete d->playInfo;
	delete d;
	LibVLC::finalize();
}

void MainWindow::onScreenSizeChanged(const QSize &size) {
	showMessage(toString(size));
}

void MainWindow::updateVideoFormat(const VideoFormat &format) {
	d->subtitle->setFrameRate(format.fps);
}

void MainWindow::checkPlayMenu() {
	Menu &menu = d->menu("play");
	menu("title").setEnabled(d->engine->titleCount() > 0);
	menu("chapter").setEnabled(d->engine->chapterCount() > 0);
}

void MainWindow::checkSubtitleMenu() {
	d->menu("subtitle")("spu").setEnabled(d->engine->spuCount() > 0);
}

void MainWindow::checkVideoMenu() {
	d->menu("video")("track").setEnabled(d->engine->videoTrackCount() > 0);
}

void MainWindow::checkAudioMenu() {
	d->menu("audio")("track").setEnabled(d->engine->audioTrackCount() > 0);
}

#define DEF_TRACK_MENU_FUNC(prop, cap, mm, msg) \
void MainWindow::check##cap##Menu() {\
	const PlayEngine::TrackList tracks = d->engine->prop##s();\
	Menu &menu = d->menu mm;\
	menu.g()->clear();\
	if (tracks.isEmpty())\
		return;\
	for (int i=0; i<tracks.size(); ++i)\
		menu.addActionToGroupWithoutKey(tracks[i].name, true)->setData(i);\
	QAction *act = menu.g()->actions().value(d->engine->current##cap##Id());\
	if (act)\
		act->setChecked(true);\
}\
\
void MainWindow::set##cap(QAction *act) {\
	d->engine->setCurrent##cap(act->data().toInt());\
	showMessage(tr(msg), act->text());\
}

DEF_TRACK_MENU_FUNC(videoTrack, VideoTrack, ("video")("track"), "Current Video Track")
DEF_TRACK_MENU_FUNC(audioTrack, AudioTrack, ("audio")("track"), "Current Audio Track")
DEF_TRACK_MENU_FUNC(spu, SPU, ("subtitle")("spu"), "Current Subtitle Track")
DEF_TRACK_MENU_FUNC(title, Title, ("play")("title"), "Current Title")
DEF_TRACK_MENU_FUNC(chapter, Chapter, ("play")("chapter"), "Current Chapter")

void MainWindow::openLocation(const QString &loc) {
	openMrl(Mrl(loc));
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
	if (mrl == d->engine->mrl()) {
		if (!d->engine->isPlaying())
			d->engine->play();
	} else {
		d->playlist->load(mrl, enc);
		if (!mrl.isPlaylist()) {
			d->playlist->play(mrl);
			if (!mrl.isDVD())
				RecentInfo::get().stack(mrl);
		}
	}
}

void MainWindow::openFile() {
	AppState &as = AppState::get();
	const QString filter = Info::mediaExtFilter();
	const QString dir = QFileInfo(as.last_open_file).absolutePath();
	const QString file = getOpenFileName(this, tr("Open File"), dir, filter);
	if (!file.isEmpty()) {
		openMrl(Mrl(file));
		as.last_open_file = file;
	}
}

void MainWindow::openDvd() {
	OpenDVDDialog dlg(this);
	dlg.setDevices(app()->devices());
	if (dlg.exec()) {
		const Mrl mrl(QLatin1String("dvd://") + dlg.device());
		openMrl(mrl);
	}
}

void MainWindow::openUrl() {
//	GetUrlDialog dlg(this);
//	if (dlg.exec())
//		openMrl(dlg.url(), dlg.encoding());
}

void MainWindow::resizeEvent(QResizeEvent *event) {
	QMainWindow::resizeEvent(event);
//	int width = d->center->width();
//	int height = d->center->height();
//	if (isFullScreen()) {
//		d->video->setFixedRenderSize(QSize(width, height));
//	} else {
//		d->video->setFixedRenderSize(QSize());
//		height -= d->control->height();
//	}
//	showMessage(QString("%1x%2").arg(width).arg(height), 1000);
}

void MainWindow::togglePlayPause() {
	if (d->engine->state() == PlayingState)
		d->engine->pause();
	else
		d->engine->play();
}

void MainWindow::showContextMenu(const QPoint &pos) {
	d->context->exec(mapToGlobal(pos));
}

void MainWindow::updateMrl(const Mrl &mrl) {
	if (mrl.isLocalFile())
		d->subtitle->autoload(mrl, true);
	else
		clearSubtitles();
	d->sync_subtitle_file_menu();
	const int row = d->playlist->model()->currentRow() + 1;
	if (row > 0)
		d->control->setTrackNumber(row, d->playlist->model()->rowCount());
}

void MainWindow::clearSubtitles() {
	d->subtitle->unload();
	QList<QAction*> acts = d->menu("subtitle")("list").g()->actions();
	for (int i=0; i<acts.size(); ++i)
		delete acts[i];
}

void MainWindow::openSubFile() {
	const QString filter = tr("Subtitle Files") +' '+ Info::subtitleExt().toFilter();
	QString enc = d->pref.sub_enc;
	QString dir;
	if (d->engine->mrl().isLocalFile())
		dir = QFileInfo(d->engine->mrl().toLocalFile()).absolutePath();
	const QStringList files = EncodingFileDialog::getOpenFileNames(this
			, tr("Open Subtitle"), dir, filter, &enc);
	if (!files.isEmpty())
		appendSubFiles(files, true, enc);
}

void MainWindow::appendSubFiles(const QStringList &files, bool checked, const QString &enc) {
	if (files.isEmpty())
		return;
	for (int i=0; i<files.size(); ++i) {
		d->subtitle->load(files[i], enc, checked);
	}
	d->sync_subtitle_file_menu();
}

void MainWindow::updateSubtitle(QAction *action) {
	if (!d->changingSub) {
		const int idx = action->data().toInt();
		d->subtitle->select(idx, action->isChecked());
	}
}

void MainWindow::maximize() {
	setGeometry(QDesktopWidget().availableGeometry(this));
//	move(0, 0);
//	resize()
}

void MainWindow::seek(int diff) {
	if (!diff || d->engine->state() == StoppedState)
		return;
	const int target = qBound(0, d->engine->position() + diff, d->engine->duration());
	if (d->engine->seek(target)) {
		d->timeLine->show(target, d->engine->duration());
		showMessage(tr("Seeking"), diff/1000, tr("sec"), true);
	}
}

void MainWindow::showMessage(const QString &cmd, int value, const QString &unit, bool sign, int last) {
	showMessage(cmd, toString(value, sign) + unit, last);
}

void MainWindow::showMessage(const QString &cmd, double value, const QString &unit, bool sign, int last) {
	showMessage(cmd, toString(value, sign) + unit, last);
}

void MainWindow::showMessage(const QString &cmd, const QString &description, int last) {
	showMessage(cmd + ": " + description, last);
}

void MainWindow::showMessage(const QString &message, int last) {
	if (d->dontShowMsg)
		return;
	d->message->showText(message, last);
}

void MainWindow::setVolume(int diff) {
	if (!diff)
		return;
	const int volume = qBound(0, d->audio->volume() + diff, 100);
	d->audio->setVolume(volume);
	showMessage(tr("Volume"), volume, "%");
}

void MainWindow::setMuted(bool muted) {
	d->audio->setMuted(muted);
	showMessage(tr("Mute"), muted);
}

void MainWindow::setFullScreen(bool full) {
	if (full == isFullScreen())
		return;
	d->dontPause = true;
	d->moving = false;
	d->prevPos = QPoint();
	d->control->setHidden(full);
	if (full) {
		app()->setAlwaysOnTop(this, false);
		setWindowState(windowState() | Qt::WindowFullScreen);
		if (d->pref.hide_cursor)
			d->hider->start(d->pref.hide_cursor_delay);
	} else {
		setWindowState(windowState() & ~Qt::WindowFullScreen);
		d->hider->stop();
		if (cursor().shape() == Qt::BlankCursor)
			unsetCursor();
		updateStaysOnTop();
	}
	d->dontPause = false;
	emit fullscreenChanged(full);
}

void MainWindow::setVideoSize(double rate) {
	if (rate < 0) {
		const bool wasFull = isFullScreen();
		setFullScreen(!wasFull);
	} else {
		if (isFullScreen())
			setFullScreen(false);
		if (rate == 0.0) {
			const QSizeF video = d->video->sizeHint();
			const QSizeF desktop = QDesktopWidget().availableGeometry(this).size();
			const double target = 0.15;
			rate = desktop.width()*desktop.height()*target/(video.width()*video.height());
		}
		resize(size() - d->video->size() + d->video->sizeHint()*qSqrt(rate));
	}
}

void MainWindow::updateState(MediaState state, MediaState old) {
	if (old == state)
		return;
	if (state == PlayingState) {
		app()->setScreensaverDisabled(d->pref.disable_screensaver);
		d->menu("play")["pause"]->setText(tr("Pause"));
	} else {
		app()->setScreensaverDisabled(false);
		d->menu("play")["pause"]->setText(tr("Play"));
	}
	d->video->setLogoMode(state == StoppedState);
	updateStaysOnTop();
}

void MainWindow::setSpeed(int diff) {
	int newSpeed = 100;
	if (diff)
		newSpeed = d->engine->speed()*100.0 + diff + 0.5;
	d->engine->setSpeed(qBound(0.1, newSpeed*0.01, 10.0));
	showMessage(tr("Speed"), QString::fromUtf8("\303\227%1").arg(d->engine->speed()));
}

void MainWindow::setAmp(int amp) {
	const int newAmp = qBound(0, qRound(d->audio->preAmp()*100 + amp), 1000);
	d->audio->setPreAmp(newAmp*0.01);
	showMessage(tr("Amp"), newAmp, "%");
}

void MainWindow::doRepeat(int key) {
	QString ex;
/*	if (key == 'a') {
		if (d->repeater->isHidden()) {
			d->repeater->setRepeater(repeater);
			d->repeater->show();
		}
	} else */
	if (key == 'r') {
		if (d->engine->state() == StoppedState)
			return;
		if (!d->ab->hasA()) {
			const int at = d->ab->setAToCurrentTime();
			QString a = msecToString(at, "h:mm:ss.zzz");
			a.chop(2);
			ex = tr("Set A to %1").arg(a);
		} else if (!d->ab->hasB()) {
			const int at = d->ab->setBToCurrentTime();
			if ((at - d->ab->a()) < 100) {
				ex = tr("Range is too short!");
				d->ab->setB(-1);
			} else {
				QString b = msecToString(at, "h:mm:ss.zzz");
				b.chop(2);
				ex = tr("Set B to %1. Start to repeat!").arg(b);
				d->ab->start();
			}
		}
	} else if (key == 'q') {
		d->ab->stop();
		d->ab->setA(-1);
		d->ab->setB(-1);
		ex = tr("Quit repeating");
	} else if (key == 's') {
		d->ab->setAToSubtitleTime();
		d->ab->setBToSubtitleTime();
		ex = tr("Repeat current subtitle");
		d->ab->start();
	}
	showMessage(tr("A-B Repeat"), ex);
}

void MainWindow::moveSubtitle(int dy) {
	int newPos = qBound(0, qRound(d->subtitle->pos()*100.0 + dy), 100);
	d->subtitle->setPos(newPos*0.01);
	showMessage(tr("Subtitle Position"), newPos, "%");
}

#define IS_IN_CENTER (d->video->geometry().contains(d->video->mapFrom(this, event->pos())))
#define IS_BUTTON(b) (event->buttons() & (b))

void MainWindow::mousePressEvent(QMouseEvent *event) {
	QMainWindow::mouseMoveEvent(event);
	if (!IS_IN_CENTER)
		return;
	if (IS_BUTTON(Qt::LeftButton) && !isFullScreen()) {
		d->moving = true;
		d->prevPos = event->globalPos();
	}
	if (IS_BUTTON(Qt::MidButton)) {
		if (QAction *action = d->menu.middleClickAction(event->modifiers()))
			action->trigger();
	}
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
	d->hider->stop();
	if (cursor().shape() == Qt::BlankCursor)
		unsetCursor();
	QMainWindow::mouseMoveEvent(event);
	const bool full = isFullScreen();
	if (full) {
		if (d->moving) {
			d->moving = false;
			d->prevPos = QPoint();
		}
		static const int h = d->control->height();
		QRect rect = this->rect();
		rect.setTop(rect.height() - h);
		d->control->setVisible(rect.contains(event->pos()));
		if (d->pref.hide_cursor)
			d->hider->start(d->pref.hide_cursor_delay);
	} else {
		if (d->moving) {
			if (event->buttons() != Qt::LeftButton) {
				d->moving = false;
				d->prevPos = QPoint();
			} else {
				const QPoint pos = event->globalPos();
				move(this->pos() + pos - d->prevPos);
				d->prevPos = pos;
			}
		}
	}
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
	if (d->moving) {
		d->moving = false;
		d->prevPos = QPoint();
	}
	QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::mouseDoubleClickEvent(QMouseEvent *event) {
	QMainWindow::mouseDoubleClickEvent(event);
	if (IS_BUTTON(Qt::LeftButton) && IS_IN_CENTER) {
		if (QAction *action = d->menu.doubleClickAction(event->modifiers()))
			action->trigger();
	}
}

void MainWindow::wheelEvent(QWheelEvent *event) {
	if (IS_IN_CENTER && event->delta()) {
		if (QAction *action = d->menu.wheelScrollAction(event->modifiers(), event->delta() > 0)) {
			action->trigger();
			event->accept();
			return;
		}
	}
	QMainWindow::wheelEvent(event);
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
		d->playlist->append(playlist);
		d->playlist->play(playlist.first());
	} else if (!subList.isEmpty())
		appendSubFiles(subList, true, d->pref.sub_enc);
}

void MainWindow::setSyncDelay(int diff) {
	int delay = diff ? d->subtitle->delay() + diff : 0;
	d->subtitle->setDelay(delay);
	showMessage("Subtitle Sync", delay*0.001, "sec", true);

}

void MainWindow::setPref() {
	static Pref::Dialog *dlg = 0;
	if (!dlg) {
		dlg = new Pref::Dialog(this);
		connect(dlg, SIGNAL(needToApplyPref()), this, SLOT(applyPref()));
	}
	dlg->show();
}

void MainWindow::applyPref() {
	d->apply_pref();
}

void MainWindow::showEvent(QShowEvent *event) {
	QMainWindow::showEvent(event);
	if (d->pausedByHiding && d->engine && d->engine->isPaused()) {
		d->engine->play();
		d->pausedByHiding = false;
	}
}

void MainWindow::hideEvent(QHideEvent *event) {
	QMainWindow::hideEvent(event);
	if (!d->pref.pause_minimized || !d->engine || d->dontPause)
		return;
	if (!d->engine->isPlaying() || (d->pref.pause_video_only && !d->engine->hasVideo()))
		return;
	d->pausedByHiding = true;
	d->engine->pause();
}

void MainWindow::hideCursor() {
	if (cursor().shape() != Qt::BlankCursor)
		setCursor(Qt::BlankCursor);
}

void MainWindow::handleTray(QSystemTrayIcon::ActivationReason reason) {
	if (reason == QSystemTrayIcon::Trigger)
		setVisible(!isVisible());
	else if (reason == QSystemTrayIcon::Context)
		d->context->exec(QCursor::pos());
}



void MainWindow::closeEvent(QCloseEvent *event) {
#ifndef Q_WS_MAC
	if (d->pref.enable_system_tray && d->pref.hide_rather_close) {
		hide();
		AppState as;
		if (as[AppState::TrayFirst].toBool()) {
			CheckDialog dlg(this);
			dlg.setChecked(true);
			dlg.setLabelText(tr("CMPlayer will be running in the system tray "
					"when the window closed.<br>"
					"You can change this behavior in the preferences.<br>"
					"If you want to exit CMPlayer, please use 'Exit' menu."));
			dlg.setCheckBoxText(tr("Do not display this message again"));
			dlg.exec();
			as[AppState::TrayFirst] = !dlg.isChecked();
		}
		event->ignore();
	} else {
		event->accept();
		qApp->quit();
	}
#else
	event->accept();
#endif
}

void MainWindow::showMessage(const QString &cmd, bool value, int last) {
	showMessage(cmd, value ? tr("On") : tr("Off"), last);
}

void MainWindow::setVolumeNormalized(bool norm) {
	d->audio->setVolumeNormalized(norm);
	showMessage(tr("Normalize Volume"), norm);
}

void MainWindow::setTempoScaled(bool scaled) {
	d->audio->setTempoScaled(scaled);
	showMessage(tr("Autoscale Pitch"), scaled);
}

void MainWindow::setColorProperty(QAction *action) {
	const QList<QVariant> data = action->data().toList();
	const ColorProperty::Value prop = ColorProperty::Value(data[0].toInt());
	if ((int)prop == -1) {
		d->video->setColorProperty(ColorProperty());
		showMessage(tr("Reset brightness, contrast, saturation and hue"));
	} else {
		ColorProperty color = d->video->colorProperty();
		color.setValue(prop, color.value(prop) + data[1].toInt()*0.01);
		d->video->setColorProperty(color);
		QString cmd;
		switch(prop) {
		case ColorProperty::Brightness:
			cmd = tr("Brightness");
			break;
		case ColorProperty::Saturation:
			cmd = tr("Saturation");
			break;
		case ColorProperty::Hue:
			cmd = tr("Hue");
			break;
		case ColorProperty::Contrast:
			cmd = tr("Contrast");
			break;
		default:
			return;
		}
		const double v = d->video->colorProperty()[prop]*100.0;
		const int value = v + (v < 0 ? -0.5 : 0.5);
		showMessage(cmd, value, "%", true);
	}
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
			onTop = d->engine->isPlaying();
	}
	app()->setAlwaysOnTop(this, onTop);
}

void MainWindow::takeSnapshot() {
	static SnapshotDialog *dlg = new SnapshotDialog(this);
	dlg->setVideoRenderer(d->video);
	dlg->setSubtitleRenderer(d->subtitle);
	dlg->take();
	dlg->adjustSize();
	dlg->show();
}

void MainWindow::about() {
	AboutDialog dlg(this);
	dlg.exec();
}

void MainWindow::setEffect(QAction *act) {
	if (!act)
		return;
	const QList<QAction*> acts = d->menu("video")("filter").actions();
	VideoRenderer::Effects effects = 0;
	for (int i=0; i<acts.size(); ++i) {
		if (acts[i]->isChecked())
			effects |= static_cast<VideoRenderer::Effect>(acts[i]->data().toInt());
	}
	d->video->setEffects(effects);
}

void MainWindow::setSubtitleAlign(int data) {
	d->subtitle->setTopAlignment(data);
}

void MainWindow::setSubtitleDisplay(int data) {
	d->subtitle->osd()->setLetterboxHint(data);
}

void MainWindow::seekToSubtitle(int key) {
	int time = -1;
	if (key < 0)
		time = d->subtitle->previous();
	else if (key > 0)
		time = d->subtitle->next();
	else
		time = d->subtitle->current();
	if (time >= 0)
		d->engine->seek(time-100);
}
