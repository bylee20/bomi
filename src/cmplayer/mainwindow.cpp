#include "mainwindow_p.hpp"
#include "overlay.hpp"
#include "prefdialog.hpp"
#include <QtCore/QBuffer>
#include <QtGui/QDesktopWidget>

#ifdef Q_WS_MAC
void qt_mac_set_dock_menu(QMenu *menu);
#endif

MainWindow::MainWindow(): d(new Data) {
	d->playlist = new PlaylistView(&d->engine, this);
	d->history = new HistoryView(&d->engine, this);
#ifndef Q_WS_MAC
	d->tray = new QSystemTrayIcon(app()->defaultIcon(), this);
#endif
	d->hider.setSingleShot(true);
	d->skin.connectTo(&d->engine, &d->audio, &d->video);

	setMouseTracking(true);
	setAcceptDrops(true);

	plug(&d->engine, &d->audio);

	Menu &open = d->menu("open");		Menu &play = d->menu("play");
	Menu &video = d->menu("video");		Menu &audio = d->menu("audio");
	Menu &sub = d->menu("subtitle");	Menu &tool = d->menu("tool");
	Menu &win = d->menu("window");		Menu &help = d->menu("help");

	CONNECT(open["file"], triggered(), this, openFile());
	CONNECT(open["url"], triggered(), this, openUrl());
	CONNECT(open["dvd"], triggered(), this, openDvd());
	CONNECT(open("recent").g(), triggered(QString), this, openLocation(QString));
	CONNECT(open("recent")["clear"], triggered(), &d->recent, clear());

	CONNECT(play["stop"], triggered(), &d->engine, stop());
	CONNECT(play("speed").g(), triggered(int), this, setSpeed(int));
	CONNECT(play["pause"], toggled(bool), this, pause(bool));
	CONNECT(play("repeat").g(), triggered(int), this, doRepeat(int));
	CONNECT(play["prev"], triggered(), d->playlist, playPrevious());
	CONNECT(play["next"], triggered(), d->playlist, playNext());
	CONNECT(play("seek").g("relative"), triggered(int), this, seek(int));
	CONNECT(play("seek").g("subtitle"), triggered(int), this, seekToSubtitle(int));
	CONNECT(play("title").g(), triggered(QAction*), this, setTitle(QAction*));
	CONNECT(play("chapter").g(), triggered(QAction*), this, setChapter(QAction*));

	CONNECT(video("track").g(), triggered(QAction*), this, setVideoTrack(QAction*));
	CONNECT(video("aspect").g(), triggered(double), &d->video, setAspectRatio(double));
	CONNECT(video("crop").g(), triggered(double), &d->video, setCropRatio(double));
	CONNECT(video["snapshot"], triggered(), this, takeSnapshot());
	CONNECT(video["drop-frame"], toggled(bool), this, setFrameDroppingEnabled(bool));
	CONNECT(&video("align"), triggered(QAction*), this, alignScreen(QAction*));
	CONNECT(&video("move"), triggered(QAction*), this, moveScreen(QAction*));
	CONNECT(&video("filter"), triggered(QAction*), this, setEffect(QAction*));
	CONNECT(video("color").g(), triggered(QAction*), this, setColorProperty(QAction*));

	CONNECT(audio("track").g(), triggered(QAction*), this, setAudioTrack(QAction*));
	CONNECT(audio.g("volume"), triggered(int), this, setVolume(int));
	CONNECT(audio["mute"], toggled(bool), this, setMuted(bool));
	CONNECT(audio.g("amp"), triggered(int), this, setAmp(int));
	CONNECT(audio["volnorm"], toggled(bool), this, setVolumeNormalized(bool));

	CONNECT(sub("list")["hide"], toggled(bool), &d->subtitle, setHidden(bool));
	CONNECT(sub("list")["open"], triggered(), this, openSubFile());
	CONNECT(sub("list")["clear"], triggered(), this, clearSubtitles());
	CONNECT(sub("list").g(), triggered(QAction*), this, updateSubtitle(QAction*));
	CONNECT(sub("spu").g(), triggered(QAction*), this, setSpu(QAction*));
	CONNECT(sub.g("display"), triggered(int), this, setSubtitleDisplay(int));
	CONNECT(sub.g("align"), triggered(int), this, setSubtitleAlign(int));
	CONNECT(sub.g("pos"), triggered(int), this, moveSubtitle(int));
	CONNECT(sub.g("sync"), triggered(int), this, setSyncDelay(int));

	CONNECT(tool["playlist"], triggered(), d->playlist, toggle());
	CONNECT(tool["history"], triggered(), d->history, toggle());
	CONNECT(tool["subtitle"], triggered(), d->subtitle.view(), toggle());
	CONNECT(tool["pref"], triggered(), this, setPref());
	CONNECT(tool["playinfo"], toggled(bool), &d->playInfo, setVisible(bool));

	CONNECT(win.g("sot"), triggered(int), this, updateStaysOnTop());
	CONNECT(win.g("size"), triggered(double), this, setVideoSize(double));
	CONNECT(win["minimize"], triggered(), this, showMinimized());
	CONNECT(win["maximize"], triggered(), this, maximize());
	CONNECT(win["close"], triggered(), this, close());

	CONNECT(help["about"], triggered(), this, about());
	CONNECT(d->menu["exit"], triggered(), this, exit());

	CONNECT(&play, aboutToShow(), this, checkPlayMenu());
	CONNECT(&play("title"), aboutToShow(), this, checkTitleMenu());
	CONNECT(&play("chapter"), aboutToShow(), this, checkChapterMenu());
	CONNECT(&video, aboutToShow(), this, checkVideoMenu());
	CONNECT(&video("track"), aboutToShow(), this, checkVideoTrackMenu());
	CONNECT(&audio, aboutToShow(), this, checkAudioMenu());
	CONNECT(&audio("track"), aboutToShow(), this, checkAudioTrackMenu());
	CONNECT(&sub, aboutToShow(), this, checkSubtitleMenu());
	CONNECT(&sub("spu"), aboutToShow(), this, checkSpuMenu());

	CONNECT(&d->engine, mrlChanged(Mrl), this, updateMrl(Mrl));
	CONNECT(&d->engine, stateChanged(State,State), this, updateState(State,State));
	CONNECT(&d->engine, tick(int), &d->subtitle, render(int));
	CONNECT(&d->screen, customContextMenuRequested(const QPoint&), this, showContextMenu(const QPoint&));
	CONNECT(&d->video, formatChanged(VideoFormat), this, updateVideoFormat(VideoFormat));
//	CONNECT(d->video, screenSizeChanged(QSize), this, onScreenSizeChanged(QSize));
	CONNECT(&d->audio, mutedChanged(bool), audio["mute"], setChecked(bool));
	CONNECT(&d->audio, volumeNormalizedChanged(bool), audio["volnorm"], setChecked(bool));

	CONNECT(&d->recent, openListChanged(QList<Mrl>), this, updateRecentActions(QList<Mrl>));
	CONNECT(&d->hider, timeout(), this, hideCursor());
	CONNECT(d->history, playRequested(Mrl), this, openMrl(Mrl));
#ifndef Q_WS_MAC
	CONNECT(d->tray, activated(QSystemTrayIcon::ActivationReason), this, handleTray(QSystemTrayIcon::ActivationReason));
#endif

	CONNECT(&d->skin, windowTitleChanged(QString), this, setWindowTitle(QString));
	CONNECT(&d->skin, windowFilePathChanged(QString), this, onWindowFilePathChanged(QString));

	addActions(d->menu.actions());
	auto makeContextMenu = [this, &open, &play, &video, &audio, &sub, &tool, &win] () {
		auto menu = new QMenu(this);
		menu->addMenu(&open);
		menu->addSeparator();
		menu->addMenu(&play);
		menu->addMenu(&video);
		menu->addMenu(&audio);
		menu->addMenu(&sub);
		menu->addSeparator();
		menu->addMenu(&tool);
		menu->addMenu(&win);
		menu->addSeparator();
		menu->addAction(d->menu("help")["about"]);
		menu->addAction(d->menu["exit"]);
		return menu;
	};
	d->context = makeContextMenu();

	d->load_state();
	d->apply_pref();

	d->playlist->setPlaylist(d->recent.lastPlaylist());
	d->engine.setMrl(d->recent.lastMrl(), false);
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
	d->screen.overlay()->add(&d->playInfo.osd());
	d->screen.overlay()->add(&d->subtitle.osd());
	d->screen.overlay()->add(&d->timeLine);
	d->screen.overlay()->add(&d->message);

	if (d->skin.load(d->p.skin_name)) {
		auto screen = d->skin.screen();
		d->screen.setParent(screen);
		d->screen.move(0, 0);
		d->screen.resize(screen->size());
		d->skin.initializePlaceholders();
		setCentralWidget(d->skin.widget());
		resize(minimumSizeHint());
	} else {
		setCentralWidget(&d->screen);
		adjustSize();
	}

	d->screen.installEventFilter(this);
}

MainWindow::~MainWindow() {
	d->screen.setParent(nullptr);
	delete d;
}

void MainWindow::onWindowFilePathChanged(const QString &path) {
	d->filePath = path;
	if (isVisible())
		setWindowFilePath(path);
}

void MainWindow::unplug() {
	::unplug(&d->engine, &d->audio);
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

void MainWindow::onScreenSizeChanged(const QSize &size) {
	showMessage(toString(size));
}

void MainWindow::updateVideoFormat(const VideoFormat &/*format*/) {
//	d->subtitle.setFrameRate(format.fps);
}

void MainWindow::checkPlayMenu() {
	Menu &menu = d->menu("play");
	menu("title").setEnabled(d->engine.titles().size() > 0);
	menu("chapter").setEnabled(d->engine.chapters().size() > 0);
}

void MainWindow::checkSubtitleMenu() {
	d->menu("subtitle")("spu").setEnabled(d->engine.spus().size() > 0);
}

void MainWindow::checkVideoMenu() {
	d->menu("video")("track").setEnabled(d->video.streams().size() > 0);
}

void MainWindow::checkAudioMenu() {
	d->menu("audio")("track").setEnabled(d->audio.streams().size() > 0);
}

static void checkStreamMenu(Menu &menu, const StreamList &streams, int current, const QString &text) {
	menu.g()->clear();
	if (streams.isEmpty())
		return;
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

void MainWindow::checkAudioTrackMenu() {
	checkStreamMenu(d->menu("audio")("track"), d->audio.streams(), d->audio.currentStreamId(), tr("Audio %1"));
}

void MainWindow::setAudioTrack(QAction *act) {
	act->setChecked(true);
	d->audio.setCurrentStream(act->data().toInt());
	showMessage(tr("Current Audio Track"), act->text());
}

void MainWindow::checkSpuMenu() {
	checkStreamMenu(d->menu("subtitle")("spu"), d->engine.spus(), d->engine.currentSpuId(), tr("Subtitle %1"));
}

void MainWindow::setSpu(QAction *act) {
	act->setChecked(true);
	d->engine.setCurrentSpu(act->data().toInt());
	showMessage(tr("Current Subtitle Track"), act->text());
}

void MainWindow::checkVideoTrackMenu() {
	checkStreamMenu(d->menu("video")("track"), d->video.streams(), d->video.currentStreamId(), tr("Video %1"));
}

void MainWindow::setVideoTrack(QAction *act) {
	act->setChecked(true);
	d->video.setCurrentStream(act->data().toInt());
	showMessage(tr("Current Video Track"), act->text());
}

void MainWindow::checkTitleMenu() {
	const auto titles = d->engine.titles();
	Menu &menu = d->menu("play")("title");
	menu.g()->clear();
	if (titles.isEmpty())
		return;
	const int current = d->engine.currentTitleId();
	for (auto it = titles.begin(); it != titles.end(); ++it) {
		auto act = menu.addActionToGroupWithoutKey(it->name, true);
		act->setData(it.key());
		if (current == it.key())
			act->setChecked(true);
	}
}

void MainWindow::setTitle(QAction *act) {
	act->setChecked(true);
	d->engine.setCurrentTitle(act->data().toInt());
	showMessage(tr("Current DVD Title"), act->text());
}

void MainWindow::checkChapterMenu() {
	const auto chapters = d->engine.chapters();
	Menu &menu = d->menu("play")("chapter");
	menu.g()->clear();
	if (chapters.isEmpty())
		return;
	const int current = d->engine.currentChapterId();
	for (int i=0; i<chapters.size(); ++i) {
		auto act = menu.addActionToGroupWithoutKey(chapters[i], true);
		act->setData(i);
		if (i == current)
			act->setChecked(true);
	}
}

void MainWindow::setChapter(QAction *act) {
	act->setChecked(true);
	d->engine.setCurrentChapter(act->data().toInt());
	showMessage(tr("Current Chapter"), act->text());
}

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

void MainWindow::openFile() {
	AppState &as = AppState::get();
	const QString filter = Info::mediaExtFilter();
	const QString dir = QFileInfo(as.open_last_file).absolutePath();
	const QString file = getOpenFileName(this, tr("Open File"), dir, filter);
	if (!file.isEmpty()) {
		openMrl(Mrl(file));
		as.open_last_file = file;
	}
}

void MainWindow::openDvd() {
	OpenDvdDialog dlg(this);
	dlg.setDevices(app()->devices());
	if (dlg.exec()) {
//		d->engine.setDvdDevice(dlg.device());
		QString url = "dvdnav://";
		if (!dlg.device().isEmpty())
			url += "/" + dlg.device();
		const Mrl mrl(url);
		openMrl(mrl);
	}
}

void MainWindow::openUrl() {
	GetUrlDialog dlg(this);
	if (dlg.exec()) {
			openMrl(dlg.url().toString(), dlg.encoding());
	}
}

bool MainWindow::eventFilter(QObject *o, QEvent *e) {
	if (o == &d->screen && e->type() == QEvent::Resize) {
		static QSize prev;
		QSize size = d->screen.size();
		if (d->skin.screen())
			size = isFullScreen() ? d->skin.widget()->size() : d->skin.screen()->size();
		if (isFullScreen()) {
			d->video.setFixedRenderSize(size);
		} else {
			d->video.setFixedRenderSize(QSize());
		}
		if (prev != size) {
			showMessage(QString::fromUtf8("%1x%2").arg(size.width()).arg(size.height()), 1000);
			prev = size;
		}
	}
	return QMainWindow::eventFilter(o, e);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
	QMainWindow::resizeEvent(event);
	d->skin.setVisible(!isFullScreen());
}

void MainWindow::pause(bool pause) {
	if (d->stateChanging)
		return;
	if (pause)
		d->engine.pause();
	else
		d->engine.play();
}

void MainWindow::showContextMenu(const QPoint &pos) {
	d->context->exec(mapToGlobal(pos));
}

void MainWindow::updateMrl(const Mrl &mrl) {
	if (mrl.isLocalFile())
		d->subtitle.autoload(mrl, true);
	else
		clearSubtitles();
	d->sync_subtitle_file_menu();
	const int number = d->playlist->model()->currentRow() + 1;
	if (number > 0) {
		d->skin.setMediaNumber(number);
		d->skin.setTotalMediaCount(d->playlist->model()->rowCount());
	}
}

void MainWindow::clearSubtitles() {
	d->subtitle.unload();
	QList<QAction*> acts = d->menu("subtitle")("list").g()->actions();
	for (int i=0; i<acts.size(); ++i)
		delete acts[i];
}

void MainWindow::openSubFile() {
	const QString filter = tr("Subtitle Files") +' '+ Info::subtitleExt().toFilter();
	QString enc = d->p.sub_enc;
	QString dir;
	if (d->engine.mrl().isLocalFile())
		dir = QFileInfo(d->engine.mrl().toLocalFile()).absolutePath();
	const QStringList files = EncodingFileDialog::getOpenFileNames(this
			, tr("Open Subtitle"), dir, filter, &enc);
	if (!files.isEmpty())
		appendSubFiles(files, true, enc);
}

void MainWindow::appendSubFiles(const QStringList &files, bool checked, const QString &enc) {
	if (files.isEmpty())
		return;
	for (int i=0; i<files.size(); ++i) {
		d->subtitle.load(files[i], enc, checked);
	}
	d->sync_subtitle_file_menu();
}

void MainWindow::updateSubtitle(QAction *action) {
	if (!d->changingSub) {
		const int idx = action->data().toInt();
		d->subtitle.select(idx, action->isChecked());
	}
}

void MainWindow::maximize() {
	setGeometry(QDesktopWidget().availableGeometry(this));
}

void MainWindow::seek(int diff) {
	qDebug() << d->engine.isSeekable();
	if (!diff || d->engine.state() == State::Stopped || !d->engine.isSeekable())
		return;
	const int target = qBound(0, d->engine.position() + diff, d->engine.duration());
	if (d->engine.isSeekable()) {
		d->engine.relativeSeek(diff);
		d->timeLine.show(target, d->engine.duration());
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
	if (!d->dontShowMsg)
		d->message.show("<p>" % message % "</p>", last);
}

void MainWindow::setVolume(int diff) {
	if (!diff)
		return;
	const int volume = qBound(0, d->audio.volume() + diff, 100);
	d->audio.setVolume(volume);
	showMessage(tr("Volume"), volume, "%");
}

void MainWindow::setMuted(bool muted) {
	d->audio.setMuted(muted);
	showMessage(tr("Mute"), muted);
}

void MainWindow::setFullScreen(bool full) {
	if (full == isFullScreen())
		return;
	QTime t;
	t.start();
	d->dontPause = true;
	d->moving = false;
	d->prevPos = QPoint();
	if (full) {
		app()->setAlwaysOnTop(this, false);
		setWindowState(windowState() | Qt::WindowFullScreen);
		if (d->p.hide_cursor)
			d->hider.start(d->p.hide_cursor_delay);
	} else {
		setWindowState(windowState() & ~Qt::WindowFullScreen);
		d->hider.stop();
		if (cursor().shape() == Qt::BlankCursor)
			unsetCursor();
		updateStaysOnTop();
	}
	d->dontPause = false;
	emit fullscreenChanged(full);
	setWindowFilePath(full ? QString() : d->filePath);
}

void MainWindow::setVideoSize(double rate) {
	if (rate < 0) {
		const bool wasFull = isFullScreen();
		setFullScreen(!wasFull);
	} else {
		// patched by Handrake
		const QSizeF video = d->video.sizeHint();
		const QSizeF desktop = QDesktopWidget().availableGeometry(this).size();
		const double target = 0.15;
		if (isFullScreen())
			setFullScreen(false);
		if (rate == 0.0)
			rate = desktop.width()*desktop.height()*target/(video.width()*video.height());
		const QSize size = this->size() - d->screen.size() + d->video.sizeHint()*qSqrt(rate);
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
				QPoint p = pos();
				p.rx() += dx;
				if (p.x() < 0)
					p.rx() = 0;
				move(p);
			}
		}
	}
}

void MainWindow::updateState(State state, State old) {
	if (old == state)
		return;
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
		break;
	}
	app()->setScreensaverDisabled(d->p.disable_screensaver && state == State::Playing);
	d->video.setLogoMode(state == State::Stopped);
	updateStaysOnTop();
	d->stateChanging = false;
}

void MainWindow::setSpeed(int diff) {
	int newSpeed = 100;
	if (diff)
		newSpeed = d->engine.speed()*100.0 + diff + 0.5;
	d->engine.setSpeed(qBound(0.1, newSpeed*0.01, 10.0));
	showMessage(tr("Speed"), QString::fromUtf8("\303\227%1").arg(d->engine.speed()));
}

void MainWindow::setAmp(int amp) {
	const int newAmp = qBound(0, qRound(d->audio.preAmp()*100 + amp), 1000);
	d->audio.setPreAmp(newAmp*0.01);
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
		if (d->engine.state() == State::Stopped)
			return;
		if (!d->ab.hasA()) {
			const int at = d->ab.setAToCurrentTime();
			QString a = msecToString(at, "h:mm:ss.zzz");
			a.chop(2);
			ex = tr("Set A to %1").arg(a);
		} else if (!d->ab.hasB()) {
			const int at = d->ab.setBToCurrentTime();
			if ((at - d->ab.a()) < 100) {
				ex = tr("Range is too short!");
				d->ab.setB(-1);
			} else {
				QString b = msecToString(at, "h:mm:ss.zzz");
				b.chop(2);
				ex = tr("Set B to %1. Start to repeat!").arg(b);
				d->ab.start();
			}
		}
	} else if (key == 'q') {
		d->ab.stop();
		d->ab.setA(-1);
		d->ab.setB(-1);
		ex = tr("Quit repeating");
	} else if (key == 's') {
		d->ab.setAToSubtitleTime();
		d->ab.setBToSubtitleTime();
		ex = tr("Repeat current subtitle");
		d->ab.start();
	}
	showMessage(tr("A-B Repeat"), ex);
}

void MainWindow::moveSubtitle(int dy) {
	int newPos = qBound(0, qRound(d->subtitle.pos()*100.0 + dy), 100);
	d->subtitle.setPos(newPos*0.01);
	showMessage(tr("Subtitle Position"), newPos, "%");
}

#define IS_IN_CENTER (d->screen.geometry().contains(d->screen.mapFrom(this, event->pos())))
#define IS_BUTTON(b) (event->buttons() & (b))

void MainWindow::mousePressEvent(QMouseEvent *event) {
	QMainWindow::mousePressEvent(event);
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
	d->hider.stop();
	if (cursor().shape() == Qt::BlankCursor)
		unsetCursor();
	QMainWindow::mouseMoveEvent(event);
	const bool full = isFullScreen();
	if (full) {
		if (d->moving) {
			d->moving = false;
			d->prevPos = QPoint();
		}
		d->skin.setVisible(d->skin.contains(event->globalPos()));
		if (d->p.hide_cursor)
			d->hider.start(d->p.hide_cursor_delay);
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
		appendSubFiles(subList, true, d->p.sub_enc);
}

void MainWindow::setSyncDelay(int diff) {
	int delay = diff ? d->subtitle.delay() + diff : 0;
	d->subtitle.setDelay(delay);
	showMessage("Subtitle Sync", delay*0.001, "sec", true);

}

void MainWindow::setPref() {
	static PrefDialog *dlg = nullptr;
	if (!dlg) {
		dlg = new PrefDialog(this);
		connect(dlg, SIGNAL(applicationRequested()), this, SLOT(applyPref()));
	}
	dlg->show();
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
	d->apply_pref();
	if (d->skin.name() != d->p.skin_name) {
		d->screen.setParent(0);
		if (d->skin.load(d->p.skin_name)) {
			auto screen = d->skin.screen();
			d->screen.setParent(screen);
			d->screen.move(0, 0);
			d->screen.resize(screen->size());
			d->skin.initializePlaceholders();
			setCentralWidget(d->skin.widget());
		} else
			setCentralWidget(&d->screen);
	}
	if (time >= 0)
		d->engine.play(time);
}

void MainWindow::showEvent(QShowEvent *event) {
	QMainWindow::showEvent(event);
	if (d->pausedByHiding && d->engine.isPaused()) {
		d->engine.play();
		d->pausedByHiding = false;
	}
	setWindowFilePath(d->filePath);
}

void MainWindow::hideEvent(QHideEvent *event) {
	QMainWindow::hideEvent(event);
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
		d->context->exec(QCursor::pos());
}



void MainWindow::closeEvent(QCloseEvent *event) {
#ifndef Q_WS_MAC
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

void MainWindow::showMessage(const QString &cmd, bool value, int last) {
	showMessage(cmd, value ? tr("On") : tr("Off"), last);
}

void MainWindow::setVolumeNormalized(bool norm) {
	d->audio.setVolumeNormalized(norm);
	showMessage(tr("Normalize Volume"), norm);
}

void MainWindow::setTempoScaled(bool scaled) {
	d->audio.setTempoScaled(scaled);
	showMessage(tr("Autoscale Pitch"), scaled);
}

void MainWindow::setColorProperty(QAction *action) {
	const QList<QVariant> data = action->data().toList();
	const ColorProperty::Value prop = ColorProperty::Value(data[0].toInt());
	if ((int)prop == -1) {
		d->video.setColorProperty(ColorProperty());
		showMessage(tr("Reset brightness, contrast, saturation and hue"));
	} else {
		ColorProperty color = d->video.colorProperty();
		color.setValue(prop, color.value(prop) + data[1].toInt()*0.01);
		d->video.setColorProperty(color);
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
		const double v = d->video.colorProperty()[prop]*100.0;
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
			onTop = d->engine.isPlaying();
	}
	app()->setAlwaysOnTop(this, onTop);
}

void MainWindow::takeSnapshot() {
	static SnapshotDialog *dlg = new SnapshotDialog(this);
	dlg->setVideoRenderer(&d->video);
	dlg->setSubtitleRenderer(&d->subtitle);
	dlg->take();
	if (!dlg->isVisible()) {
		dlg->adjustSize();
		dlg->show();
	}
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
	d->video.setEffects(effects);
}

void MainWindow::setSubtitleAlign(int data) {
	d->subtitle.setTopAlignment(data);
}

void MainWindow::setSubtitleDisplay(int data) {
	d->subtitle.osd().setLetterboxHint(data);
}

void MainWindow::seekToSubtitle(int key) {
	int time = -1;
	if (key < 0)
		time = d->subtitle.previous();
	else if (key > 0)
		time = d->subtitle.next();
	else
		time = d->subtitle.current();
	if (time >= 0)
		d->engine.seek(time-100);
}

void MainWindow::moveScreen(QAction *action) {
	QPoint offset = d->video.offset();
	const Qt::ArrowType move = (Qt::ArrowType)action->data().toInt();
	if (move == Qt::UpArrow)
		offset.ry() -= 1;
	else if (move == Qt::DownArrow)
		offset.ry() += 1;
	else if (move == Qt::LeftArrow)
		offset.rx() -= 1;
	else if (move == Qt::RightArrow)
		offset.rx() += 1;
	else
		offset = QPoint(0, 0);
	d->video.setOffset(offset);
}

void MainWindow::alignScreen(QAction */*action*/) {
	int key = 0;
	for (auto action : d->menu("video")("align").actions()) {
		if (action->isChecked())
			key |= action->data().toInt();
	}
	d->video.setAlignment(key);
}

void MainWindow::setFrameDroppingEnabled(bool enabled) {
	d->engine.setFrameDroppingEnabled(enabled);
	showMessage(tr("Drop Frame"), enabled);
}
