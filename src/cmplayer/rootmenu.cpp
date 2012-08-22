#include "rootmenu.hpp"
#include "videorenderer.hpp"
#include "enums.hpp"
#include "pref.hpp"
#include "colorproperty.hpp"
#include <QtCore/QDebug>
#include "record.hpp"

RootMenu *RootMenu::obj = nullptr;

RootMenu::RootMenu(): Menu(_L("menu"), 0) {
	Menu *open = this->addMenu(_L("open"));

	QAction *file = open->addAction(_L("file"));
	file->setShortcut(Qt::CTRL + Qt::Key_F);
	file->setData(int('f'));
	QAction *url = open->addAction(_L("url"));
	url->setData(int('u'));
	QAction *dvd = open->addAction(_L("dvd"));
	dvd->setData(QUrl("dvd://"));

	open->addSeparator();

	Menu *recent = open->addMenu(_L("recent"));

	recent->addSeparator();
	recent->addAction(_L("clear"));

	Menu *play = this->addMenu(_L("play"));

	QAction *pause = play->addAction(_L("pause"));
	pause->setCheckable(true);
	pause->setChecked(true);
	pause->setShortcut(Qt::Key_Space);
	play->addAction(_L("stop"));

	play->addSeparator();

	QAction *prev = play->addAction(_L("prev"));
	QAction *next = play->addAction(_L("next"));
	prev->setShortcut(Qt::CTRL + Qt::Key_Left);
	next->setShortcut(Qt::CTRL + Qt::Key_Right);

	play->addSeparator();

	Menu *speed = play->addMenu(_L("speed"));
	speed->addActionToGroup(_L("slower"), false)->setShortcut(Qt::Key_Minus);
	QAction *reset = speed->addActionToGroup(_L("reset"), false);
	reset->setShortcut(Qt::Key_Backspace);
	reset->setData(0);
	QAction *faster = speed->addActionToGroup(_L("faster"), false);
	faster->setShortcuts(QList<QKeySequence>() << Qt::Key_Plus << Qt::Key_Equal);

	play->addSeparator();

	Menu *repeat = play->addMenu(_L("repeat"));
	QAction *range = repeat->addActionToGroup(_L("range"), false);
	QAction *srange = repeat->addActionToGroup(_L("subtitle"), false);
	QAction *quitRepeat = repeat->addActionToGroup(_L("quit"), false);
	range->setShortcut(Qt::Key_R);
	range->setData(int('r'));
	srange->setShortcut(Qt::Key_E);
	srange->setData(int('s'));
	quitRepeat->setShortcut(Qt::Key_Escape);
	quitRepeat->setData(int('q'));

	play->addSeparator();

	Menu *seek = play->addMenu(_L("seek"));
	QAction *forward1 = seek->addActionToGroup(_L("forward1"), false, _L("relative"));
	QAction *forward2 = seek->addActionToGroup(_L("forward2"), false, _L("relative"));
	QAction *forward3 = seek->addActionToGroup(_L("forward3"), false, _L("relative"));
	QAction *backward1 = seek->addActionToGroup(_L("backward1"), false, _L("relative"));
	QAction *backward2 = seek->addActionToGroup(_L("backward2"), false, _L("relative"));
	QAction *backward3 = seek->addActionToGroup(_L("backward3"), false, _L("relative"));
	forward1->setShortcut(Qt::Key_Right);
	forward2->setShortcut(Qt::Key_PageDown);
	forward3->setShortcut(Qt::Key_End);
	backward1->setShortcut(Qt::Key_Left);
	backward2->setShortcut(Qt::Key_PageUp);
	backward3->setShortcut(Qt::Key_Home);

	seek->addSeparator();
	QAction *prevSub = seek->addActionToGroup(_L("prev-subtitle"), false, _L("subtitle"));
	QAction *curSub = seek->addActionToGroup(_L("current-subtitle"), false, _L("subtitle"));
	QAction *nextSub = seek->addActionToGroup(_L("next-subtitle"), false, _L("subtitle"));
	prevSub->setData(-1);
	prevSub->setShortcut(Qt::Key_Comma);
	curSub->setData(0);
	curSub->setShortcut(Qt::Key_Period);
	nextSub->setData(1);
	nextSub->setShortcut(Qt::Key_Slash);

	play->addMenu(_L("title"))->setEnabled(false);
	play->addMenu(_L("chapter"))->setEnabled(false);

	Menu *subtitle = this->addMenu(_L("subtitle"));
	subtitle->addMenu(_L("spu"))->setEnabled(false);

	Menu *sList = subtitle->addMenu(_L("list"));
	sList->g()->setExclusive(false);
	sList->addAction(_L("open"));
	sList->addAction(_L("clear"));
	sList->addAction(_L("hide"))->setCheckable(true);

	subtitle->addSeparator();
	subtitle->addActionToGroup(_L("in-video"), true, _L("display"))->setData(0);
	subtitle->addActionToGroup(_L("on-letterbox"), true, _L("display"))->setData(1);
	subtitle->addSeparator();
	subtitle->addActionToGroup(_L("align-top"), true, _L("align"))->setData(1);
	subtitle->addActionToGroup(_L("align-bottom"), true, _L("align"))->setData(0);
	subtitle->addSeparator();
	subtitle->addActionToGroup(_L("pos-up"), false, _L("pos"))->setShortcut(Qt::Key_W);
	subtitle->addActionToGroup(_L("pos-down"), false, _L("pos"))->setShortcut(Qt::Key_S);

	subtitle->addSeparator();

	subtitle->addActionToGroup(_L("sync-add"), false, _L("sync"))->setShortcut(Qt::Key_D);
	QAction *syncReset = subtitle->addActionToGroup(_L("sync-reset"), false, _L("sync"));
	syncReset->setShortcut(Qt::Key_Q);
	syncReset->setData(0);
	subtitle->addActionToGroup(_L("sync-sub"), false, _L("sync"))->setShortcut(Qt::Key_A);

	Menu *video = this->addMenu(_L("video"));
	video->addMenu(_L("track"))->setEnabled(false);
	video->addSeparator();
	QAction *snapshot = video->addAction(_L("snapshot"));
	snapshot->setShortcut(Qt::CTRL + Qt::Key_S);
	snapshot->setEnabled(false);
	snapshot->setVisible(false);
	video->addAction(_L("drop-frame"), true)->setShortcut(Qt::CTRL + Qt::Key_D);
	video->addSeparator();

	Menu *aspect = video->addMenu(_L("aspect"));
	aspect->addActionToGroup(_L("auto"), true)->setData(-1.0);
	aspect->addActionToGroup(_L("window"), true)->setData(0.0);
	aspect->addActionToGroup(_L("4:3"), true)->setData(4.0/3.0);
	aspect->addActionToGroup(_L("16:9"), true)->setData(16.0/9.0);
	aspect->addActionToGroup(_L("1.85:1"), true)->setData(1.85);
	aspect->addActionToGroup(_L("2.35:1"), true)->setData(2.35);

	Menu *crop = video->addMenu(_L("crop"));
	crop->addActionToGroup(_L("off"), true)->setData(-1.0);
	crop->addActionToGroup(_L("window"), true)->setData(0.0);
	crop->addActionToGroup(_L("4:3"), true)->setData(4.0/3.0);
	crop->addActionToGroup(_L("16:9"), true)->setData(16.0/9.0);
	crop->addActionToGroup(_L("1.85:1"), true)->setData(1.85);
	crop->addActionToGroup(_L("2.35:1"), true)->setData(2.35);

	Menu *align = video->addMenu(_L("align"));
	align->addActionToGroup(_L("top"), true, _L("vertical"))->setData((int)Qt::AlignTop);
	align->addActionToGroup(_L("v-center"), true, _L("vertical"))->setData((int)Qt::AlignVCenter);
	align->addActionToGroup(_L("bottom"), true, _L("vertical"))->setData((int)Qt::AlignBottom);
	align->addSeparator();
	align->addActionToGroup(_L("left"), true, _L("horizontal"))->setData((int)Qt::AlignLeft);
	align->addActionToGroup(_L("h-center"), true, _L("horizontal"))->setData((int)Qt::AlignHCenter);
	align->addActionToGroup(_L("right"), true, _L("horizontal"))->setData((int)Qt::AlignRight);

	Menu *move = video->addMenu(_L("move"));
	QAction *mact = move->addActionToGroup(_L("reset"));
	mact->setData((int)Qt::NoArrow);
	mact->setShortcut(Qt::SHIFT + Qt::Key_X);
	move->addSeparator();
	mact = move->addAction(_L("up"));
	mact->setData((int)Qt::UpArrow);
	mact->setShortcut(Qt::SHIFT + Qt::Key_W);
	mact = move->addAction(_L("down"));
	mact->setData((int)Qt::DownArrow);
	mact->setShortcut(Qt::SHIFT + Qt::Key_S);
	mact = move->addAction(_L("left"));
	mact->setData((int)Qt::LeftArrow);
	mact->setShortcut(Qt::SHIFT + Qt::Key_A);
	mact = move->addAction(_L("right"));
	mact->setData((int)Qt::RightArrow);
	mact->setShortcut(Qt::SHIFT + Qt::Key_D);

	video->addSeparator();

	Menu *effect = video->addMenu(_L("filter"));
	effect->g()->setExclusive(false);
	effect->addActionToGroup(_L("flip-v"), true)->setData((int)VideoRenderer::FlipVertically);
	effect->addActionToGroup(_L("flip-h"), true)->setData((int)VideoRenderer::FlipHorizontally);
	effect->addSeparator();
	effect->addActionToGroup(_L("blur"), true)->setData((int)VideoRenderer::Blur);
	effect->addActionToGroup(_L("sharpen"), true)->setData((int)VideoRenderer::Sharpen);
	effect->addSeparator();
	effect->addActionToGroup(_L("remap"), true)->setData((int)VideoRenderer::RemapLuma);
	effect->addSeparator();
	effect->addActionToGroup(_L("gray"), true)->setData((int)VideoRenderer::Grayscale);
	effect->addActionToGroup(_L("invert"), true)->setData((int)VideoRenderer::InvertColor);
	effect->addSeparator();
	effect->addActionToGroup(_L("ignore"), true)->setData((int)VideoRenderer::IgnoreEffect);

	Menu *color = video->addMenu(_L("color"));
	QAction *creset = color->addActionToGroup(_L("reset"), false);
	creset->setShortcut(Qt::Key_O);
	creset->setData(QList<QVariant>() << -1 << 0);
	color->addSeparator();
	color->addActionToGroup(_L("brightness+"))->setShortcut(Qt::Key_T);
	color->addActionToGroup(_L("brightness-"))->setShortcut(Qt::Key_G);
	color->addActionToGroup(_L("contrast+"))->setShortcut(Qt::Key_Y);
	color->addActionToGroup(_L("contrast-"))->setShortcut(Qt::Key_H);
	color->addActionToGroup(_L("saturation+"))->setShortcut(Qt::Key_U);
	color->addActionToGroup(_L("saturation-"))->setShortcut(Qt::Key_J);
	color->addActionToGroup(_L("hue+"))->setShortcut(Qt::Key_I);
	color->addActionToGroup(_L("hue-"))->setShortcut(Qt::Key_K);

	Menu *audio = this->addMenu(_L("audio"));
	audio->addMenu(_L("track"))->setEnabled(false);
	audio->addSeparator();

	QAction *volUp = audio->addActionToGroup(_L("volume-up"), false, _L("volume"));
	volUp->setShortcut(Qt::Key_Up);
	QAction *volDown = audio->addActionToGroup(_L("volume-down"), false, _L("volume"));
	volDown->setShortcut(Qt::Key_Down);
	QAction *mute = audio->addAction(_L("mute"), true);
	mute->setShortcut(Qt::Key_M);
	QAction *volnorm = audio->addAction(_L("volnorm"), true);
	volnorm->setShortcut(Qt::Key_N);

	audio->addSeparator();

	QAction *ampUp = audio->addActionToGroup(_L("amp-up"), false, _L("amp"));
	QAction *ampDown = audio->addActionToGroup(_L("amp-down"), false, _L("amp"));
	ampUp->setShortcut(Qt::CTRL + Qt::Key_Up);
	ampDown->setShortcut(Qt::CTRL + Qt::Key_Down);

	Menu *tool = this->addMenu(_L("tool"));
	tool->addAction(_L("playlist"))->setShortcut(Qt::Key_L);
	tool->addAction(_L("favorites"))->setVisible(false);
	tool->addAction(_L("history"))->setShortcut(Qt::Key_C);
	tool->addAction(_L("subtitle"))->setShortcut(Qt::Key_V);
	tool->addSeparator();
	QAction *pref = tool->addAction(_L("pref"));
	pref->setShortcut(Qt::Key_P);
	pref->setMenuRole(QAction::PreferencesRole);
	tool->addSeparator();
	QAction *playInfo = tool->addAction(_L("playinfo"));
	playInfo->setCheckable(true);
	playInfo->setShortcut(Qt::Key_Tab);

	Menu *window = this->addMenu(_L("window"));
	// sot == Stay On Top
	window->addActionToGroup(_L("sot-always"), true, _L("sot"))->setData(Enum::StaysOnTop::Always.id());
	window->addActionToGroup(_L("sot-playing"), true, _L("sot"))->setData(Enum::StaysOnTop::Playing.id());
	window->addActionToGroup(_L("sot-never"), true, _L("sot"))->setData(Enum::StaysOnTop::Never.id());
	window->addSeparator();
	QAction *proper = window->addActionToGroup(_L("proper"), false, _L("size"));
	QAction *to100 = window->addActionToGroup(_L("100%"), false, _L("size"));
	QAction *to200 = window->addActionToGroup(_L("200%"), false, _L("size"));
	QAction *to300 = window->addActionToGroup(_L("300%"), false, _L("size"));
	QAction *to400 = window->addActionToGroup(_L("400%"), false, _L("size"));
	QAction *toFull = window->addActionToGroup(_L("full"), false, _L("size"));
	proper->setData(0.0);
	to100->setData(1.0);
	to200->setData(2.0);
	to300->setData(3.0);
	to400->setData(4.0);
	toFull->setData(-1.0);
	proper->setShortcut(Qt::Key_QuoteLeft);
	to100->setShortcut(Qt::Key_1);
	to200->setShortcut(Qt::Key_2);
	to300->setShortcut(Qt::Key_3);
	to400->setShortcut(Qt::Key_4);
	toFull->setShortcuts(QList<QKeySequence>()
			<< Qt::Key_Enter << Qt::Key_Return << Qt::Key_F);
	window->addSeparator();
	window->addAction(_L("minimize"));
	window->addAction(_L("maximize"));
	window->addAction(_L("close"));

	Menu *help = this->addMenu(_L("help"));
	QAction *about = help->addAction(_L("about"));
	about->setMenuRole(QAction::AboutQtRole);

	QAction *exit = this->addAction(_L("exit"));
#ifdef Q_WS_MAC
	exit->setShortcut(Qt::ALT + Qt::Key_F4);
#else
	exit->setShortcut(Qt::CTRL + Qt::Key_Q);
#endif

	m_click[Enum::ClickAction::OpenFile] = file;
	m_click[Enum::ClickAction::Fullscreen] = toFull;
	m_click[Enum::ClickAction::Pause] = pause;
	m_click[Enum::ClickAction::Mute] = mute;
	m_wheel[Enum::WheelAction::Seek1] = WheelActionPair(forward1, backward2);
	m_wheel[Enum::WheelAction::Seek2] = WheelActionPair(forward2, backward2);
	m_wheel[Enum::WheelAction::Seek3] = WheelActionPair(forward3, backward2);
	m_wheel[Enum::WheelAction::PrevNext] = WheelActionPair(prev, next);
	m_wheel[Enum::WheelAction::Volume] = WheelActionPair(volUp, volDown);
	m_wheel[Enum::WheelAction::Amp] = WheelActionPair(ampUp, ampDown);

	load();
}

void RootMenu::update() {
	Menu &root = *this;
	const Pref &p = Pref::get();

	Menu &open = root("open");
	open.setTitle(tr("Open"));
	open["file"]->setText(tr("Open File"));
	open["url"]->setText(tr("Load URL"));
	open["dvd"]->setText(tr("Open DVD"));

	Menu &recent = open("recent");
	recent.setTitle(tr("Recent Open"));
	recent["clear"]->setText(tr("Clear"));

	Menu &play = root("play");
	play.setTitle(tr("Play"));
	play["pause"]->setText(tr("Play"));
	play["stop"]->setText(tr("Stop"));
	play["prev"]->setText(tr("Play Previous"));
	play["next"]->setText(tr("Play Next"));

	Menu &speed = play("speed");
	speed.setTitle(tr("Playback Speed"));
	speed["reset"]->setText(tr("Reset"));
	setActionStep(speed["faster"], speed["slower"], "%1%", p.speed_step);

	Menu &repeat = play("repeat");
	repeat.setTitle(tr("A-B Repeat"));
	repeat["range"]->setText(tr("Set Range to Current Time"));
	repeat["subtitle"]->setText(tr("Repeat Current Subtitle"));
	repeat["quit"]->setText(tr("Quit"));

	Menu &seek = play("seek");
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

	Menu &sub = root("subtitle");
	sub.setTitle(tr("Subtitle"));
	Menu &list = sub("list");
	list.setTitle(tr("Subtitle File"));
	list["open"]->setText(tr("Open"));
	list["clear"]->setText(tr("Clear"));
	list["hide"]->setText(tr("Hide"));
	sub("spu").setTitle(tr("Subtitle Track"));

	sub["on-letterbox"]->setText(tr("Display on Letterbox"));
	sub["in-video"]->setText(tr("Display in Video"));
	sub["align-top"]->setText(tr("Top Alignment"));
	sub["align-bottom"]->setText(tr("Bottom Alignment"));

	setActionAttr(sub["pos-up"], -p.sub_pos_step, tr("Up %1%"), p.sub_pos_step, false);
	setActionAttr(sub["pos-down"], p.sub_pos_step, tr("Down %1%"), p.sub_pos_step, false);
	sub["sync-reset"]->setText(tr("Reset Sync"));
	setActionStep(sub["sync-add"], sub["sync-sub"], tr("Sync %1sec"), p.sync_delay_step, 0.001);

	Menu &video = root("video");
	video.setTitle(tr("Video"));
	video("track").setTitle(tr("Video Track"));

	Menu &aspect = video("aspect");
	aspect.setTitle(tr("Aspect Ratio"));
	aspect["auto"]->setText(tr("Auto"));
	aspect["window"]->setText(tr("Same as Window"));
	aspect["4:3"]->setText(tr("4:3 (TV)"));
	aspect["16:9"]->setText(tr("16:9 (HDTV)"));
	aspect["1.85:1"]->setText(tr("1.85:1 (Wide Vision)"));
	aspect["2.35:1"]->setText(tr("2.35:1 (CinemaScope)"));

	Menu &crop = video("crop");
	crop.setTitle(tr("Crop"));
	crop["off"]->setText(tr("Off"));
	crop["window"]->setText(tr("Same as Window"));
	crop["4:3"]->setText(tr("4:3 (TV)"));
	crop["16:9"]->setText(tr("16:9 (HDTV)"));
	crop["1.85:1"]->setText(tr("1.85:1 (Wide Vision)"));
	crop["2.35:1"]->setText(tr("2.35:1 (CinemaScope)"));

	Menu &align = video("align");
	align.setTitle(tr("Screen Alignment"));
//	align["center"]->setText(tr("Center"));
	align["top"]->setText(tr("Top"));
	align["v-center"]->setText(tr("Vertical Center"));
	align["bottom"]->setText(tr("Bottom"));
	align["left"]->setText(tr("Left"));
	align["h-center"]->setText(tr("Horizontal Center"));
	align["right"]->setText(tr("Right"));

	Menu &move = video("move");
	move.setTitle(tr("Screen Position"));
	move["reset"]->setText(tr("Reset"));
	move["up"]->setText(tr("Up"));
	move["down"]->setText(tr("Down"));
	move["left"]->setText(tr("To Left"));
	move["right"]->setText(tr("To Right"));

	Menu &effect = video("filter");
	effect.setTitle(tr("Filter"));
	effect["flip-v"]->setText(tr("Flip Vertically"));
	effect["flip-h"]->setText(tr("Flip Horizontally"));
	effect["blur"]->setText(tr("Blur"));
	effect["sharpen"]->setText(tr("Sharpen"));
	effect["gray"]->setText(tr("Grayscale"));
	effect["invert"]->setText(tr("Invert Color"));
	effect["remap"]->setText(tr("Adjust Constrast"));
	effect["ignore"]->setText(tr("Ignore All Filters"));

	Menu &color = video("color");
	color.setTitle(tr("Color"));
	color["reset"]->setText(tr("Reset"));
	setVideoPropStep(color, "brightness", ColorProperty::Brightness, tr("Brightness %1%"), p.brightness_step);
	setVideoPropStep(color, "saturation", ColorProperty::Saturation, tr("Saturation %1%"), p.brightness_step);
	setVideoPropStep(color, "contrast", ColorProperty::Contrast, tr("Contrast %1%"), p.brightness_step);
	setVideoPropStep(color, "hue", ColorProperty::Hue, tr("Hue %1%"), p.brightness_step);

	video["snapshot"]->setText(tr("Take Snapshot"));
	video["drop-frame"]->setText(tr("Drop Frame"));

	Menu &audio = root("audio");
	audio.setTitle(tr("Audio"));
	audio("track").setTitle(tr("Audio Track"));
	audio["mute"]->setText(tr("Mute"));
	audio["volnorm"]->setText(tr("Normalize Volume"));
	setActionStep(audio["volume-up"], audio["volume-down"]
			, tr("Volume %1%"), p.volume_step);
	setActionStep(audio["amp-up"], audio["amp-down"]
			, tr("Amp %1%"), p.amp_step);

	Menu &tool = root("tool");
	tool.setTitle(tr("Tools"));
	tool["playlist"]->setText(tr("Playlist"));
	tool["favorites"]->setText(tr("Favorites"));
	tool["history"]->setText(tr("Play History"));
	tool["subtitle"]->setText(tr("Subtitle View"));
	tool["pref"]->setText(tr("Preferences"));
	tool["playinfo"]->setText(tr("Play Information"));

	Menu &window = root("window");
	window.setTitle(tr("Window"));
	window["sot-always"]->setText(tr("Always Stay on Top"));
	window["sot-playing"]->setText(tr("Stay on Top Playing"));
	window["sot-never"]->setText(tr("Don't Stay on Top"));
	window["proper"]->setText(tr("Proper Size"));
	window["full"]->setText(tr("Fullscreen"));
	window["minimize"]->setText(tr("Minimize"));
	window["maximize"]->setText(tr("Maximize"));
	window["close"]->setText(tr("Close"));

	Menu &help = root("help");
	help.setTitle(tr("Help"));
	help["about"]->setText(tr("About %1").arg("CMPlayer"));
	root["exit"]->setText(tr("Exit"));
}

void RootMenu::save() {
	Record r;
	Menu::save(r);
}

void RootMenu::load() {
	Record r;
	Menu::load(r);
}

QAction *RootMenu::action(const QString &id) const {
	const QStringList key = id.split(QLatin1Char('/'));
	if (key.size() < 2)
		return 0;
	if (key.first() != this->id())
		return 0;
	const Menu *it = this;
	for (int i=1; i<key.size()-1 && it; ++i) {
		if (!(it = it->m(key[i])))
			return nullptr;
	}
	QAction *action = it->a(key.last());
	if (!action) {
		Menu *menu = it->m(key.last());
		if (menu)
			action = menu->menuAction();
	}
	return action;
}

QAction *RootMenu::doubleClickAction(Qt::KeyboardModifiers mod) const {
	const Pref::ClickActionInfo info = Pref::get().double_click_map[mod];
	if (info.enabled)
		return m_click[info.action];
	return 0;
}

QAction *RootMenu::middleClickAction(Qt::KeyboardModifiers mod) const {
	const Pref::ClickActionInfo info = Pref::get().middle_click_map[mod];
	if (info.enabled)
		return m_click[info.action];
	return 0;
}

QAction *RootMenu::wheelScrollAction(Qt::KeyboardModifiers mod, bool up) const {
	const Pref::WheelActionInfo info = Pref::get().wheel_scroll_map[mod];
	if (info.enabled)
		return up ? m_wheel[info.action].up : m_wheel[info.action].down;
	return 0;
}

