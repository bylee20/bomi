#include "rootmenu.hpp"
#include "videorenderer.hpp"
#include "enums.hpp"
#include "pref.hpp"
#include "colorproperty.hpp"
#include <QtCore/QDebug>
#include "record.hpp"

RootMenu *RootMenu::obj = 0;

RootMenu::RootMenu(): Menu(_LS("menu"), 0) {
	Q_ASSERT(obj == 0);
	obj = this;

	Menu *open = obj->addMenu(_LS("open"));

	QAction *file = open->addAction(_LS("file"));
	file->setShortcut(Qt::CTRL + Qt::Key_F);
	file->setData(int('f'));
	QAction *url = open->addAction(_LS("url"));
	url->setData(int('u'));
	QAction *dvd = open->addAction(_LS("dvd"));
	dvd->setData(QUrl("dvd://"));
	url->setDisabled(true);
	url->setVisible(false);

	open->addSeparator();

	Menu *recent = open->addMenu(_LS("recent"));

	recent->addSeparator();
	recent->addAction(_LS("clear"));

	Menu *play = obj->addMenu(_LS("play"));

	QAction *pause = play->addAction(_LS("pause"));
	pause->setShortcut(Qt::Key_Space);
	play->addAction(_LS("stop"));

	play->addSeparator();

	QAction *prev = play->addAction(_LS("prev"));
	QAction *next = play->addAction(_LS("next"));
	prev->setShortcut(Qt::CTRL + Qt::Key_Left);
	next->setShortcut(Qt::CTRL + Qt::Key_Right);

	play->addSeparator();

	Menu *speed = play->addMenu(_LS("speed"));
	speed->addActionToGroup(_LS("slower"), false)->setShortcut(Qt::Key_Minus);
	QAction *reset = speed->addActionToGroup(_LS("reset"), false);
	reset->setShortcut(Qt::Key_Backspace);
	reset->setData(0);
	QAction *faster = speed->addActionToGroup(_LS("faster"), false);
	faster->setShortcuts(QList<QKeySequence>() << Qt::Key_Plus << Qt::Key_Equal);

	play->addSeparator();

	Menu *repeat = play->addMenu(_LS("repeat"));
	QAction *range = repeat->addActionToGroup(_LS("range"), false);
	QAction *srange = repeat->addActionToGroup(_LS("subtitle"), false);
	QAction *quitRepeat = repeat->addActionToGroup(_LS("quit"), false);
	range->setShortcut(Qt::Key_R);
	range->setData(int('r'));
	srange->setShortcut(Qt::Key_E);
	srange->setData(int('s'));
	quitRepeat->setShortcut(Qt::Key_Escape);
	quitRepeat->setData(int('q'));

	play->addSeparator();

	Menu *seek = play->addMenu(_LS("seek"));
	QAction *forward1 = seek->addActionToGroup(_LS("forward1"), false, _LS("relative"));
	QAction *forward2 = seek->addActionToGroup(_LS("forward2"), false, _LS("relative"));
	QAction *forward3 = seek->addActionToGroup(_LS("forward3"), false, _LS("relative"));
	QAction *backward1 = seek->addActionToGroup(_LS("backward1"), false, _LS("relative"));
	QAction *backward2 = seek->addActionToGroup(_LS("backward2"), false, _LS("relative"));
	QAction *backward3 = seek->addActionToGroup(_LS("backward3"), false, _LS("relative"));
	forward1->setShortcut(Qt::Key_Right);
	forward2->setShortcut(Qt::Key_PageDown);
	forward3->setShortcut(Qt::Key_End);
	backward1->setShortcut(Qt::Key_Left);
	backward2->setShortcut(Qt::Key_PageUp);
	backward3->setShortcut(Qt::Key_Home);

	seek->addSeparator();
	QAction *prevSub = seek->addActionToGroup(_LS("prev-subtitle"), false, _LS("subtitle"));
	QAction *curSub = seek->addActionToGroup(_LS("current-subtitle"), false, _LS("subtitle"));
	QAction *nextSub = seek->addActionToGroup(_LS("next-subtitle"), false, _LS("subtitle"));
	prevSub->setData(-1);
	prevSub->setShortcut(Qt::Key_Comma);
	curSub->setData(0);
	curSub->setShortcut(Qt::Key_Period);
	nextSub->setData(1);
	nextSub->setShortcut(Qt::Key_Slash);

	play->addMenu(_LS("title"))->setEnabled(false);
	play->addMenu(_LS("chapter"))->setEnabled(false);

	Menu *subtitle = obj->addMenu(_LS("subtitle"));
	subtitle->addMenu(_LS("spu"))->setEnabled(false);

	Menu *sList = subtitle->addMenu(_LS("list"));
	sList->g()->setExclusive(false);
	sList->addAction(_LS("open"));
	sList->addAction(_LS("clear"));
	sList->addAction(_LS("hide"))->setCheckable(true);

	subtitle->addSeparator();
	subtitle->addActionToGroup(_LS("in-video"), true, _LS("display"))->setData(0);
	subtitle->addActionToGroup(_LS("on-letterbox"), true, _LS("display"))->setData(1);
	subtitle->addSeparator();
	subtitle->addActionToGroup(_LS("align-top"), true, _LS("align"))->setData(1);
	subtitle->addActionToGroup(_LS("align-bottom"), true, _LS("align"))->setData(0);
	subtitle->addSeparator();
//	subtitle->addAction();
	subtitle->addActionToGroup(_LS("pos-up"), false, _LS("pos"))->setShortcut(Qt::Key_W);
	subtitle->addActionToGroup(_LS("pos-down"), false, _LS("pos"))->setShortcut(Qt::Key_S);

	subtitle->addSeparator();

	subtitle->addActionToGroup(_LS("sync-add"), false, _LS("sync"))->setShortcut(Qt::Key_D);
	QAction *syncReset = subtitle->addActionToGroup(_LS("sync-reset"), false, _LS("sync"));
	syncReset->setShortcut(Qt::Key_Q);
	syncReset->setData(0);
	subtitle->addActionToGroup(_LS("sync-sub"), false, _LS("sync"))->setShortcut(Qt::Key_A);

	Menu *video = obj->addMenu(_LS("video"));
	video->addMenu(_LS("track"))->setEnabled(false);
	video->addSeparator();
	video->addAction(_LS("snapshot"))->setShortcut(Qt::CTRL + Qt::Key_S);
	video->addSeparator();

	Menu *aspect = video->addMenu(_LS("aspect"));
	aspect->addActionToGroup(_LS("auto"), true)->setData(-1.0);
	aspect->addActionToGroup(_LS("window"), true)->setData(0.0);
	aspect->addActionToGroup(_LS("4:3"), true)->setData(4.0/3.0);
	aspect->addActionToGroup(_LS("16:9"), true)->setData(16.0/9.0);
	aspect->addActionToGroup(_LS("1.85:1"), true)->setData(1.85);
	aspect->addActionToGroup(_LS("2.35:1"), true)->setData(2.35);

	Menu *crop = video->addMenu(_LS("crop"));
	crop->addActionToGroup(_LS("off"), true)->setData(-1.0);
	crop->addActionToGroup(_LS("window"), true)->setData(0.0);
	crop->addActionToGroup(_LS("4:3"), true)->setData(4.0/3.0);
	crop->addActionToGroup(_LS("16:9"), true)->setData(16.0/9.0);
	crop->addActionToGroup(_LS("1.85:1"), true)->setData(1.85);
	crop->addActionToGroup(_LS("2.35:1"), true)->setData(2.35);

	video->addSeparator();

	Menu *effect = video->addMenu(_LS("filter"));
	effect->g()->setExclusive(false);
	effect->addActionToGroup(_LS("flip-v"), true)->setData((int)VideoRenderer::FlipVertically);
	effect->addActionToGroup(_LS("flip-h"), true)->setData((int)VideoRenderer::FlipHorizontally);
	effect->addSeparator();
	effect->addActionToGroup(_LS("blur"), true)->setData((int)VideoRenderer::Blur);
	effect->addActionToGroup(_LS("sharpen"), true)->setData((int)VideoRenderer::Sharpen);
	effect->addSeparator();
	effect->addActionToGroup(_LS("remap"), true)->setData((int)VideoRenderer::RemapLuma);
	effect->addActionToGroup(_LS("auto-contrast"), true)->setData((int)VideoRenderer::AutoContrast);
	effect->addSeparator();
	effect->addActionToGroup(_LS("gray"), true)->setData((int)VideoRenderer::Grayscale);
	effect->addActionToGroup(_LS("invert"), true)->setData((int)VideoRenderer::InvertColor);
	effect->addSeparator();
	effect->addActionToGroup(_LS("ignore"), true)->setData((int)VideoRenderer::IgnoreEffect);
	video->addSeparator();

	QAction *creset = video->addActionToGroup(_LS("reset"), false, _LS("color"));
	creset->setShortcut(Qt::Key_O);
	creset->setData(QList<QVariant>() << -1 << 0);
	video->addActionToGroup(_LS("brightness+"), false, _LS("color"))->setShortcut(Qt::Key_T);
	video->addActionToGroup(_LS("brightness-"), false, _LS("color"))->setShortcut(Qt::Key_G);
	video->addActionToGroup(_LS("contrast+"), false, _LS("color"))->setShortcut(Qt::Key_Y);
	video->addActionToGroup(_LS("contrast-"), false, _LS("color"))->setShortcut(Qt::Key_H);
	video->addActionToGroup(_LS("saturation+"), false, _LS("color"))->setShortcut(Qt::Key_U);
	video->addActionToGroup(_LS("saturation-"), false, _LS("color"))->setShortcut(Qt::Key_J);
	video->addActionToGroup(_LS("hue+"), false, _LS("color"))->setShortcut(Qt::Key_I);
	video->addActionToGroup(_LS("hue-"), false, _LS("color"))->setShortcut(Qt::Key_K);

	video->addSeparator();
	Menu *overlay = video->addMenu(_LS("overlay"));
	overlay->addActionToGroup(_LS("auto"), true)->setData(Enum::Overlay::Auto.id());
	overlay->addActionToGroup(_LS("fbo"), true)->setData(Enum::Overlay::FramebufferObject.id());
	overlay->addActionToGroup(_LS("pixmap"), true)->setData(Enum::Overlay::Pixmap.id());

	Menu *audio = obj->addMenu(_LS("audio"));
	audio->addMenu(_LS("track"))->setEnabled(false);
	audio->addSeparator();

	QAction *volUp = audio->addActionToGroup(_LS("volume-up"), false, _LS("volume"));
	volUp->setShortcut(Qt::Key_Up);
	QAction *volDown = audio->addActionToGroup(_LS("volume-down"), false, _LS("volume"));
	volDown->setShortcut(Qt::Key_Down);
	QAction *mute = audio->addAction(_LS("mute"), true);
	mute->setShortcut(Qt::Key_M);
	QAction *volnorm = audio->addAction(_LS("volnorm"), true);
	volnorm->setShortcut(Qt::Key_N);

	audio->addSeparator();

	QAction *ampUp = audio->addActionToGroup(_LS("amp-up"), false, _LS("amp"));
	QAction *ampDown = audio->addActionToGroup(_LS("amp-down"), false, _LS("amp"));
	ampUp->setShortcut(Qt::CTRL + Qt::Key_Up);
	ampDown->setShortcut(Qt::CTRL + Qt::Key_Down);

	Menu *tool = obj->addMenu(_LS("tool"));
	tool->addAction(_LS("playlist"))->setShortcut(Qt::Key_L);
	tool->addAction(_LS("favorites"))->setVisible(false);
	tool->addAction(_LS("history"))->setShortcut(Qt::Key_C);
	tool->addAction(_LS("subtitle"))->setShortcut(Qt::Key_V);
	tool->addSeparator();
	QAction *pref = tool->addAction(_LS("pref"));
	pref->setShortcut(Qt::Key_P);
	pref->setMenuRole(QAction::PreferencesRole);
	tool->addSeparator();
	QAction *playInfo = tool->addAction(_LS("playinfo"));
	playInfo->setCheckable(true);
	playInfo->setShortcut(Qt::Key_Tab);

	Menu *window = obj->addMenu(_LS("window"));
	// sot == Stay On Top
	window->addActionToGroup(_LS("sot-always"), true, _LS("sot"))->setData(Enum::StaysOnTop::Always.id());
	window->addActionToGroup(_LS("sot-playing"), true, _LS("sot"))->setData(Enum::StaysOnTop::Playing.id());
	window->addActionToGroup(_LS("sot-never"), true, _LS("sot"))->setData(Enum::StaysOnTop::Never.id());
	window->addSeparator();
	QAction *proper = window->addActionToGroup(_LS("proper"), false, _LS("size"));
	QAction *to100 = window->addActionToGroup(_LS("100%"), false, _LS("size"));
	QAction *to200 = window->addActionToGroup(_LS("200%"), false, _LS("size"));
	QAction *to300 = window->addActionToGroup(_LS("300%"), false, _LS("size"));
	QAction *to400 = window->addActionToGroup(_LS("400%"), false, _LS("size"));
	QAction *toFull = window->addActionToGroup(_LS("full"), false, _LS("size"));
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
	window->addAction(_LS("minimize"));
	window->addAction(_LS("maximize"));
	window->addAction(_LS("close"));

	Menu *help = obj->addMenu(_LS("help"));
	QAction *about = help->addAction(_LS("about"));
	about->setMenuRole(QAction::AboutQtRole);

	QAction *exit = obj->addAction(_LS("exit"));
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
}

RootMenu::~RootMenu() {
	Q_ASSERT(obj == this);
	obj = 0;
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

	Menu &effect = video("filter");
	effect.setTitle(tr("Filter"));
	effect["flip-v"]->setText(tr("Flip Vertically"));
	effect["flip-h"]->setText(tr("Flip Horizontally"));
	effect["blur"]->setText(tr("Blur"));
	effect["sharpen"]->setText(tr("Sharpen"));
	effect["gray"]->setText(tr("Grayscale"));
	effect["invert"]->setText(tr("Invert Color"));
	effect["remap"]->setText(tr("Adjust Constrast"));
	effect["auto-contrast"]->setText(tr("Auto Contrast"));
	effect["ignore"]->setText(tr("Ignore All Filters"));

	video["reset"]->setText(tr("Reset"));
	setVideoPropStep(video, "brightness", ColorProperty::Brightness
			, tr("Brightness %1%"), p.brightness_step);
	setVideoPropStep(video, "saturation", ColorProperty::Saturation
			, tr("Saturation %1%"), p.brightness_step);
	setVideoPropStep(video, "contrast", ColorProperty::Contrast
			, tr("Contrast %1%"), p.brightness_step);
	setVideoPropStep(video, "hue", ColorProperty::Hue
			, tr("Hue %1%"), p.brightness_step);
	video["snapshot"]->setText(tr("Take Snapshot"));

	Menu &overlay = video("overlay");
	overlay.setTitle(tr("Overlay"));
	overlay["auto"]->setText(tr("Auto"));
	overlay["fbo"]->setText(tr("Framebuffer Object"));
	overlay["pixmap"]->setText(tr("Pixmap"));

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
	for (int i=1; i<key.size()-1 && it; ++i)
		it = it->m(key[i]);
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

