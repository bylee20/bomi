#include "rootmenu.hpp"
#include "videorendereritem.hpp"
#include "enums.hpp"
#include "pref.hpp"
#include "colorproperty.hpp"
#include "record.hpp"

RootMenu *RootMenu::obj = nullptr;

RootMenu::RootMenu(): Menu(_L("menu"), 0) {
	Q_ASSERT(obj == nullptr);
	obj = this;

	setTitle("Root Menu");

	auto &open = *addMenu(_L("open"));
	open.addAction(_L("file"));
	open.addAction(_L("folder"));
	open.addAction(_L("url"));
	open.addAction(_L("dvd"));
	open.addSeparator();
	auto &recent = *open.addMenu(_L("recent"));
		recent.addSeparator();
		recent.addAction(_L("clear"));

	auto &play = *addMenu(_L("play"));
	play.addAction(_L("pause"));
	play.addAction(_L("stop"));
	play.addSeparator();
	auto prev = play.addAction(_L("prev"));
	auto next = play.addAction(_L("next"));
	play.addSeparator();
	auto &speed = *play.addMenu(_L("speed"));
		speed.addActionToGroup(_L("slower"), false);
		speed.addActionToGroup(_L("reset"), false)->setData(0);
		speed.addActionToGroup(_L("faster"), false);
	auto &repeat = *play.addMenu(_L("repeat"));
		repeat.addActionToGroup(_L("range"), false)->setData(int('r'));
		repeat.addActionToGroup(_L("subtitle"), false)->setData(int('s'));
		repeat.addActionToGroup(_L("quit"), false)->setData(int('q'));
	play.addSeparator();
	auto &seek = *play.addMenu(_L("seek"));
		auto forward1 = seek.addActionToGroup(_L("forward1"), false, _L("relative"));
		auto forward2 = seek.addActionToGroup(_L("forward2"), false, _L("relative"));
		auto forward3 = seek.addActionToGroup(_L("forward3"), false, _L("relative"));
		auto backward1 = seek.addActionToGroup(_L("backward1"), false, _L("relative"));
		auto backward2 = seek.addActionToGroup(_L("backward2"), false, _L("relative"));
		auto backward3 = seek.addActionToGroup(_L("backward3"), false, _L("relative"));
		seek.addSeparator();
		seek.addActionToGroup(_L("prev-subtitle"), false, _L("subtitle"))->setData(-1);
		seek.addActionToGroup(_L("current-subtitle"), false, _L("subtitle"))->setData(0);
		seek.addActionToGroup(_L("next-subtitle"), false, _L("subtitle"))->setData(1);
	play.addMenu(_L("title"))->setEnabled(false);
	play.addMenu(_L("chapter"))->setEnabled(false);

	auto &subtitle = *addMenu(_L("subtitle"));
	auto &spu = *subtitle.addMenu(_L("track"));
		spu.addGroup("internal")->setExclusive(true);
		spu.addGroup("external")->setExclusive(false);
		spu.addAction(_L("open"));
		spu.addAction(_L("clear"));
		spu.addSeparator();
		spu.addAction(_L("next"));
		spu.addAction(_L("all"));
		spu.addAction(_L("hide"), true);
		spu.addSeparator();
	subtitle.addSeparator();
	subtitle.addActionToGroup(_L("in-video"), true, _L("display"))->setData(0);
	subtitle.addActionToGroup(_L("on-letterbox"), true, _L("display"))->setData(1);
	subtitle.addSeparator();
	subtitle.addActionToGroup(_L("align-top"), true, _L("align"))->setData(1);
	subtitle.addActionToGroup(_L("align-bottom"), true, _L("align"))->setData(0);
	subtitle.addSeparator();
	subtitle.addActionToGroup(_L("pos-up"), false, _L("pos"))->setData(1);
	subtitle.addActionToGroup(_L("pos-down"), false, _L("pos"))->setData(-1);
	subtitle.addSeparator();
	subtitle.addActionToGroup(_L("sync-reset"), false, _L("sync"))->setData(0);
	subtitle.addActionToGroup(_L("sync-add"), false, _L("sync"))->setData(1);
	subtitle.addActionToGroup(_L("sync-sub"), false, _L("sync"))->setData(-1);

	auto &video = *addMenu(_L("video"));
	video.addMenu(_L("track"))->setEnabled(false);
	video.addSeparator();
	video.addAction(_L("snapshot"));
	video.addSeparator();
	auto &aspect = *video.addMenu(_L("aspect"));
		aspect.addActionToGroup(_L("auto"), true)->setData(-1.0);
		aspect.addActionToGroup(_L("window"), true)->setData(0.0);
		aspect.addActionToGroup(_L("4:3"), true)->setData(4.0/3.0);
		aspect.addActionToGroup(_L("16:10"), true)->setData(16.0/10.0);
		aspect.addActionToGroup(_L("16:9"), true)->setData(16.0/9.0);
		aspect.addActionToGroup(_L("1.85:1"), true)->setData(1.85);
		aspect.addActionToGroup(_L("2.35:1"), true)->setData(2.35);
	auto &crop = *video.addMenu(_L("crop"));
		crop.addActionToGroup(_L("off"), true)->setData(-1.0);
		crop.addActionToGroup(_L("window"), true)->setData(0.0);
		crop.addActionToGroup(_L("4:3"), true)->setData(4.0/3.0);
		crop.addActionToGroup(_L("16:10"), true)->setData(16.0/10.0);
		crop.addActionToGroup(_L("16:9"), true)->setData(16.0/9.0);
		crop.addActionToGroup(_L("1.85:1"), true)->setData(1.85);
		crop.addActionToGroup(_L("2.35:1"), true)->setData(2.35);
	auto &align = *video.addMenu(_L("align"));
		align.addActionToGroup(_L("top"), true, _L("vertical"))->setData((int)Qt::AlignTop);
		align.addActionToGroup(_L("v-center"), true, _L("vertical"))->setData((int)Qt::AlignVCenter);
		align.addActionToGroup(_L("bottom"), true, _L("vertical"))->setData((int)Qt::AlignBottom);
		align.addSeparator();
		align.addActionToGroup(_L("left"), true, _L("horizontal"))->setData((int)Qt::AlignLeft);
		align.addActionToGroup(_L("h-center"), true, _L("horizontal"))->setData((int)Qt::AlignHCenter);
		align.addActionToGroup(_L("right"), true, _L("horizontal"))->setData((int)Qt::AlignRight);
	auto &move = *video.addMenu(_L("move"));
		move.addActionToGroup(_L("reset"))->setData((int)Qt::NoArrow);
		move.addSeparator();
		move.addAction(_L("up"))->setData((int)Qt::UpArrow);
		move.addAction(_L("down"))->setData((int)Qt::DownArrow);
		move.addAction(_L("left"))->setData((int)Qt::LeftArrow);
		move.addAction(_L("right"))->setData((int)Qt::RightArrow);
	video.addSeparator();

	auto &interpolator = *video.addMenu(_L("interpolator"));
	interpolator.addAction("next");
	interpolator.addSeparator();
	interpolator.g()->setExclusive(true);
	interpolator.addActionToGroup(_L("bilinear"), true)->setData((int)InterpolatorType::Bilinear);
	interpolator.addActionToGroup(_L("catmull"), true)->setData((int)InterpolatorType::BicubicCR);
	interpolator.addActionToGroup(_L("mitchell"), true)->setData((int)InterpolatorType::BicubicMN);
	interpolator.addActionToGroup(_L("b-spline"), true)->setData((int)InterpolatorType::BicubicBS);
	interpolator.addActionToGroup(_L("lanczos2"), true)->setData((int)InterpolatorType::Lanczos2);
	interpolator.addActionToGroup(_L("lanczos3-approx"), true)->setData((int)InterpolatorType::Lanczos3Approx);

	auto &deint = *video.addMenu(_L("deint"));
	deint.addAction(_L("toggle"));
	deint.addSeparator();
	deint.g()->setExclusive(true);
	deint.addActionToGroup(_L("off"), true)->setData((int)DeintMode::Off);
	deint.addActionToGroup(_L("auto"), true)->setData((int)DeintMode::Auto);

	auto &effect = *video.addMenu(_L("filter"));
	effect.g()->setExclusive(false);
	effect.addActionToGroup(_L("flip-v"), true)->setData((int)VideoRendererItem::FlipVertically);
	effect.addActionToGroup(_L("flip-h"), true)->setData((int)VideoRendererItem::FlipHorizontally);
	effect.addSeparator();
	effect.addActionToGroup(_L("blur"), true)->setData((int)VideoRendererItem::Blur);
	effect.addActionToGroup(_L("sharpen"), true)->setData((int)VideoRendererItem::Sharpen);
	effect.addSeparator();
	effect.addActionToGroup(_L("gray"), true)->setData((int)VideoRendererItem::Grayscale);
	effect.addActionToGroup(_L("invert"), true)->setData((int)VideoRendererItem::InvertColor);
	effect.addSeparator();
	effect.addActionToGroup(_L("disable"), true)->setData((int)VideoRendererItem::Disable);

	auto &color = *video.addMenu(_L("color"));
	color.addActionToGroup(_L("reset"), false)->setData(QList<QVariant>() << (int)ColorProperty::PropMax << 0);
	color.addSeparator();
	color.addActionToGroup(_L("brightness+"))->setShortcut(Qt::Key_T);
	color.addActionToGroup(_L("brightness-"))->setShortcut(Qt::Key_G);
	color.addActionToGroup(_L("contrast+"))->setShortcut(Qt::Key_Y);
	color.addActionToGroup(_L("contrast-"))->setShortcut(Qt::Key_H);
	color.addActionToGroup(_L("saturation+"))->setShortcut(Qt::Key_U);
	color.addActionToGroup(_L("saturation-"))->setShortcut(Qt::Key_J);
	color.addActionToGroup(_L("hue+"))->setShortcut(Qt::Key_I);
	color.addActionToGroup(_L("hue-"))->setShortcut(Qt::Key_K);

	auto &audio = *addMenu(_L("audio"));
	auto &track = *audio.addMenu(_L("track"));
		track.setEnabled(false);
		track.g()->setExclusive(true);
		track.addAction(_L("next"));
		track.addSeparator();
	audio.addSeparator();
	auto volUp = audio.addActionToGroup(_L("volume-up"), false, _L("volume"));
	auto volDown = audio.addActionToGroup(_L("volume-down"), false, _L("volume"));
	auto mute = audio.addAction(_L("mute"), true);
	audio.addSeparator();
	audio.addActionToGroup(_L("sync-reset"), false, _L("sync"))->setData(0);
	audio.addActionToGroup(_L("sync-add"), false, _L("sync"))->setData(1);
	audio.addActionToGroup(_L("sync-sub"), false, _L("sync"))->setData(-1);
	audio.addSeparator();
	audio.addAction(_L("normalizer"), true)->setShortcut(Qt::Key_N);
	audio.addAction(_L("tempo-scaler"), true)->setShortcut(Qt::Key_Z);
	audio.addSeparator();
	auto ampUp = audio.addActionToGroup(_L("amp-up"), false, _L("amp"));
	auto ampDown = audio.addActionToGroup(_L("amp-down"), false, _L("amp"));

	auto &tool = *addMenu(_L("tool"));
	tool.addAction(_L("undo"))->setShortcut(Qt::CTRL + Qt::Key_Z);
	tool.addAction(_L("redo"))->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Z);
	tool.addSeparator();
		auto &playlist = *tool.addMenu(_L("playlist"));
		playlist.addAction(_L("toggle"))->setShortcut(Qt::Key_L);
		playlist.addSeparator();
		playlist.addAction(_L("open"));
		playlist.addAction(_L("save"));
		playlist.addAction(_L("clear"));
		playlist.addSeparator();
		playlist.addAction(_L("append-file"));
		playlist.addAction(_L("append-url"));
		playlist.addAction(_L("remove"));
		playlist.addSeparator();
		playlist.addAction(_L("move-up"));
		playlist.addAction(_L("move-down"));

	tool.addAction(_L("favorites"))->setVisible(false);
		auto &history = *tool.addMenu(_L("history"));
		history.addAction(_L("toggle"))->setShortcut(Qt::Key_C);
		history.addAction(_L("clear"));

	tool.addAction(_L("subtitle"))->setShortcut(Qt::Key_V);
	tool.addAction(_L("playinfo"));
	tool.addSeparator();
	tool.addAction(_L("pref"))->setMenuRole(QAction::PreferencesRole);
	tool.addAction(_L("reload-skin"));
	tool.addSeparator();
	tool.addAction(_L("auto-exit"), true);
	tool.addAction(_L("auto-shutdown"), true);

	auto &window = *addMenu(_L("window"));
	// sot == Stays On Top
	window.addActionToGroup(_L("sot-always"), true, _L("stays-on-top"))->setData((int)StaysOnTop::Always);
	window.addActionToGroup(_L("sot-playing"), true, _L("stays-on-top"))->setData((int)StaysOnTop::Playing);
	window.addActionToGroup(_L("sot-never"), true, _L("stays-on-top"))->setData((int)StaysOnTop::Never);
	window.addSeparator();
	window.addActionToGroup(_L("proper"), false, _L("size"))->setData(0.0);
	window.addActionToGroup(_L("100%"), false, _L("size"))->setData(1.0);
	window.addActionToGroup(_L("200%"), false, _L("size"))->setData(2.0);
	window.addActionToGroup(_L("300%"), false, _L("size"))->setData(3.0);
	window.addActionToGroup(_L("400%"), false, _L("size"))->setData(4.0);
	window.addActionToGroup(_L("full"), false, _L("size"))->setData(-1.0);
	window.addSeparator();
	window.addAction(_L("minimize"));
	window.addAction(_L("maximize"));
	window.addAction(_L("close"));

	auto &help = *addMenu(_L("help"));
	help.addAction(_L("about"))->setMenuRole(QAction::AboutRole);

	addAction(_L("exit"))->setMenuRole(QAction::QuitRole);

	m_click[ClickAction::OpenFile] = open["file"];
	m_click[ClickAction::Fullscreen] = window["full"];
	m_click[ClickAction::Pause] = play["pause"];
	m_click[ClickAction::Mute] = mute;
	m_wheel[WheelAction::Seek1] = WheelActionPair(forward1, backward1);
	m_wheel[WheelAction::Seek2] = WheelActionPair(forward2, backward2);
	m_wheel[WheelAction::Seek3] = WheelActionPair(forward3, backward3);
	m_wheel[WheelAction::PrevNext] = WheelActionPair(prev, next);
	m_wheel[WheelAction::Volume] = WheelActionPair(volUp, volDown);
	m_wheel[WheelAction::Amp] = WheelActionPair(ampUp, ampDown);

	play("title").setEnabled(false);
	play("chapter").setEnabled(false);
	video("track").setEnabled(false);
	audio("track").setEnabled(false);

	fillId(this, "");
}

bool RootMenu::execute(const QString &longId, const QString &argument) {
	ArgAction aa = RootMenu::instance().m_actions.value(longId);
	if (aa.action) {
		if (aa.action->menu())
			aa.action->menu()->exec(QCursor::pos());
		else {
			aa.argument = argument;
			aa.action->trigger();
			aa.argument.clear();
		}
		return true;
	} else {
		qDebug() << "Cannot find action:" << longId;
		return false;
	}
}

void RootMenu::setShortcuts(const Shortcuts &shortcuts) {
	for (auto it = shortcuts.cbegin(); it != shortcuts.cend(); ++it) {
		auto id = it.key();
		if (id.startsWith("menu/"))
			id = id.mid(5);
		auto found = m_actions.constFind(id);
		if (found != m_actions.cend())
			found.value().action->setShortcuts(it.value());
		else
			qDebug() << "Cannot find action:" << id;
	}
#ifdef Q_OS_MAC
	a(_L("exit"))->setShortcut(QKeySequence());
#endif
}

void RootMenu::fillId(Menu *menu, const QString &id) {
	const auto ids = menu->ids();
	for (auto it = ids.cbegin(); it != ids.cend(); ++it) {
		const QString key = id % it.key();
		m_ids[m_actions[key].action = it.value()] = key;
		if (auto menu = qobject_cast<Menu*>(it.value()->menu()))
			fillId(menu, key % "/");
	}
}

QHash<QString, QList<QKeySequence> > RootMenu::shortcuts() const {
	QHash<QString, QList<QKeySequence> > keys;
	for (auto it = m_actions.cbegin(); it != m_actions.cend(); ++it) {
		auto shortcuts = it.value().action->shortcuts();
		if (!shortcuts.isEmpty())
			keys[it.key()] = shortcuts;
	}
	return keys;
}

void RootMenu::update(const Pref &p) {
	auto &root = *this;

	auto &open = root("open");
	open.setTitle(tr("Open"));
	open["file"]->setText(tr("Open File"));
	open["folder"]->setText(tr("Open Folder"));
	open["url"]->setText(tr("Load URL"));
	open["dvd"]->setText(tr("Open DVD"));

	auto &recent = open("recent");
	recent.setTitle(tr("Recently Opened"));
	recent["clear"]->setText(tr("Clear"));

	auto &play = root("play");
	play.setTitle(tr("Play"));
	play["pause"]->setText(tr("Play"));
	play["stop"]->setText(tr("Stop"));
	play["prev"]->setText(tr("Play Previous"));
	play["next"]->setText(tr("Play Next"));

	auto &speed = play("speed");
	speed.setTitle(tr("Playback Speed"));
	speed["reset"]->setText(tr("Reset"));
	setActionStep(speed["faster"], speed["slower"], "%1%", p.speed_step);

	auto &repeat = play("repeat");
	repeat.setTitle(tr("A-B Repeat"));
	repeat["range"]->setText(tr("Set Range to Current Time"));
	repeat["subtitle"]->setText(tr("Repeat Current Subtitle"));
	repeat["quit"]->setText(tr("Quit"));

	auto &seek = play("seek");
	seek.setTitle(tr("Seek"));
	const QString forward = tr("Forward %1sec");
	setActionAttr(seek["forward1"], p.seek_step1, forward, p.seek_step1*0.001, false);
	setActionAttr(seek["forward2"], p.seek_step2, forward, p.seek_step2*0.001, false);
	setActionAttr(seek["forward3"], p.seek_step3, forward, p.seek_step3*0.001, false);
	const QString backward = tr("Backward %1sec");
	setActionAttr(seek["backward1"], -p.seek_step1, backward, p.seek_step1*0.001, false);
	setActionAttr(seek["backward2"], -p.seek_step2, backward, p.seek_step2*0.001, false);
	setActionAttr(seek["backward3"], -p.seek_step3, backward, p.seek_step3*0.001, false);

	seek["prev-subtitle"]->setText(tr("To Previous Subtitle"));
	seek["current-subtitle"]->setText(tr("To Beginning of Current Subtitle"));
	seek["next-subtitle"]->setText(tr("To Next Subtitle"));

	play("title").setTitle(tr("Title"));
	play("chapter").setTitle(tr("Chapter"));

	auto &sub = root("subtitle");
	sub.setTitle(tr("Subtitle"));

	auto &spu = sub("track");
	spu.setTitle(tr("Subtitle Track"));
	spu["open"]->setText(tr("Open File(s)"));
	spu["clear"]->setText(tr("Clear File(s)"));
	spu["next"]->setText(tr("Select Next"));
	spu["all"]->setText(tr("Select All"));
	spu["hide"]->setText(tr("Hide"));

	sub["on-letterbox"]->setText(tr("Display in Letterbox"));
	sub["in-video"]->setText(tr("Display in Video"));
	sub["align-top"]->setText(tr("Top Alignment"));
	sub["align-bottom"]->setText(tr("Bottom Alignment"));

	setActionAttr(sub["pos-up"], -p.sub_pos_step, tr("Up %1%"), p.sub_pos_step, false);
	setActionAttr(sub["pos-down"], p.sub_pos_step, tr("Down %1%"), p.sub_pos_step, false);
	sub["sync-reset"]->setText(tr("Reset Sync"));
	setActionStep(sub["sync-add"], sub["sync-sub"], tr("Sync %1sec"), p.sub_sync_step, 0.001);

	auto &video = root("video");
	video.setTitle(tr("Video"));
	video("track").setTitle(tr("Video Track"));

	auto &aspect = video("aspect");
	aspect.setTitle(tr("Aspect Ratio"));
	aspect["auto"]->setText(tr("Auto"));
	aspect["window"]->setText(tr("Same as Window"));
	aspect["4:3"]->setText(tr("4:3 (TV)"));
	aspect["16:10"]->setText(tr("16:10 (Wide Monitor)"));
	aspect["16:9"]->setText(tr("16:9 (HDTV)"));
	aspect["1.85:1"]->setText(tr("1.85:1 (Wide Vision)"));
	aspect["2.35:1"]->setText(tr("2.35:1 (CinemaScope)"));

	auto &crop = video("crop");
	crop.setTitle(tr("Crop"));
	crop["off"]->setText(tr("Off"));
	crop["window"]->setText(tr("Same as Window"));
	crop["4:3"]->setText(tr("4:3 (TV)"));
	crop["16:10"]->setText(tr("16:10 (Wide Monitor)"));
	crop["16:9"]->setText(tr("16:9 (HDTV)"));
	crop["1.85:1"]->setText(tr("1.85:1 (Wide Vision)"));
	crop["2.35:1"]->setText(tr("2.35:1 (CinemaScope)"));

	auto &align = video("align");
	align.setTitle(tr("Screen Alignment"));
//	align["center"]->setText(tr("Center"));
	align["top"]->setText(tr("Top"));
	align["v-center"]->setText(tr("Vertical Center"));
	align["bottom"]->setText(tr("Bottom"));
	align["left"]->setText(tr("Left"));
	align["h-center"]->setText(tr("Horizontal Center"));
	align["right"]->setText(tr("Right"));

	auto &move = video("move");
	move.setTitle(tr("Screen Position"));
	move["reset"]->setText(tr("Reset"));
	move["up"]->setText(tr("Up"));
	move["down"]->setText(tr("Down"));
	move["left"]->setText(tr("To Left"));
	move["right"]->setText(tr("To Right"));

	auto &deint = video("deint");
	deint.setTitle(tr("Deinterlace"));
	deint["toggle"]->setText(tr("Toggle"));
	deint["off"]->setText(tr("Off"));
	deint["auto"]->setText(tr("Auto"));

	auto &interpolator = video("interpolator");
	interpolator.setTitle(tr("Interpolator"));
	interpolator["next"]->setText(tr("Select Next"));
	for (auto action : interpolator.g()->actions())
		action->setText(InterpolatorTypeInfo::description(action->data().toInt()));

	auto &effect = video("filter");
	effect.setTitle(tr("Filter"));
	effect["flip-v"]->setText(tr("Flip Vertically"));
	effect["flip-h"]->setText(tr("Flip Horizontally"));
	effect["blur"]->setText(tr("Blur"));
	effect["sharpen"]->setText(tr("Sharpen"));
	effect["gray"]->setText(tr("Grayscale"));
	effect["invert"]->setText(tr("Invert Color"));
	effect["disable"]->setText(tr("Disable Filters"));

	auto &color = video("color");
	color.setTitle(tr("Color"));
	color["reset"]->setText(tr("Reset"));
	setVideoPropStep(color, "brightness", ColorProperty::Brightness, tr("Brightness %1%"), p.brightness_step);
	setVideoPropStep(color, "saturation", ColorProperty::Saturation, tr("Saturation %1%"), p.brightness_step);
	setVideoPropStep(color, "contrast", ColorProperty::Contrast, tr("Contrast %1%"), p.brightness_step);
	setVideoPropStep(color, "hue", ColorProperty::Hue, tr("Hue %1%"), p.brightness_step);

	video["snapshot"]->setText(tr("Take Snapshot"));

	auto &audio = root("audio");
	audio.setTitle(tr("Audio"));
	audio("track").setTitle(tr("Audio Track"));
	audio("track")["next"]->setText(tr("Select Next"));
	audio["mute"]->setText(tr("Mute"));
	audio["normalizer"]->setText(tr("Volume Normalizer"));
	audio["tempo-scaler"]->setText(tr("Tempo Scaler"));
	audio["sync-reset"]->setText(tr("Reset Sync"));
	setActionStep(audio["sync-add"], audio["sync-sub"], tr("Sync %1sec"), p.audio_sync_step, 0.001);
	setActionStep(audio["volume-up"], audio["volume-down"], tr("Volume %1%"), p.volume_step);
	setActionStep(audio["amp-up"], audio["amp-down"], tr("Amp %1%"), p.amp_step);

	auto &tool = root("tool");
	tool["undo"]->setText(tr("Undo"));
	tool["redo"]->setText(tr("Redo"));
	tool.setTitle(tr("Tools"));
		auto &playlist = tool("playlist");
		playlist.setTitle(tr("Playlist"));
		playlist["toggle"]->setText(tr("Show/Hide"));
		playlist["open"]->setText(tr("Open"));
		playlist["save"]->setText(tr("Save"));
		playlist["clear"]->setText(tr("Clear"));
		playlist["append-file"]->setText(tr("Append File"));
		playlist["append-url"]->setText(tr("Append URL"));
		playlist["remove"]->setText(tr("Remove"));
		playlist["move-up"]->setText(tr("Move Up"));
		playlist["move-down"]->setText(tr("Move Down"));
	tool["favorites"]->setText(tr("Favorites"));
		auto &history = tool("history");
		history.setTitle(tr("History"));
		history["toggle"]->setText(tr("Show/Hide"));
		history["clear"]->setText(tr("Clear"));

	tool["subtitle"]->setText(tr("Subtitle View"));
	tool["pref"]->setText(tr("Preferences"));
	tool["reload-skin"]->setText(tr("Reload Skin"));
	tool["playinfo"]->setText(tr("Playback Information"));
	tool["auto-exit"]->setText(tr("Auto-exit"));
	tool["auto-shutdown"]->setText(tr("Auto-shutdown"));

	auto &window = root("window");
	window.setTitle(tr("Window"));
	window["sot-always"]->setText(tr("Always Stay on Top"));
	window["sot-playing"]->setText(tr("Stay on Top While Playing"));
	window["sot-never"]->setText(tr("Don't Stay on Top"));
	window["proper"]->setText(tr("Proper Size"));
	window["full"]->setText(tr("Fullscreen"));
	window["minimize"]->setText(tr("Minimize"));
	window["maximize"]->setText(tr("Maximize"));
	window["close"]->setText(tr("Close"));

	auto &help = root("help");
	help.setTitle(tr("Help"));
	help["about"]->setText(tr("About %1").arg("CMPlayer"));
	root["exit"]->setText(tr("Exit"));

	setShortcuts(p.shortcuts);
}

void RootMenu::fillKeyMap(Menu *menu) {
	for (auto action : menu->actions()) {
		if (action->menu())
			fillKeyMap(static_cast<Menu*>(action->menu()));
		else {
			for (auto key : action->shortcuts())
				m_keymap[key] = action;
		}
	}
}

QAction *RootMenu::doubleClickAction(const ClickActionEnumInfo &info) const {
	return info.enabled ? m_click[info.action] : nullptr;
}

QAction *RootMenu::middleClickAction(const ClickActionEnumInfo &info) const {
	return info.enabled ? m_click[info.action] : nullptr;
}

QAction *RootMenu::wheelScrollAction(const WheelActionEnumInfo &info, bool up) const {
	return info.enabled ? (up ? m_wheel[info.action].up : m_wheel[info.action].down) : nullptr;
}

