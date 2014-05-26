#include "rootmenu.hpp"
#include "video/videocolor.hpp"
#include "misc/log.hpp"
#include "misc/stepactionpair.hpp"
#include "enum/deintmode.hpp"
#include "enum/dithering.hpp"
#include "enum/movetoward.hpp"
#include "enum/staysontop.hpp"
#include "enum/videoratio.hpp"
#include "enum/colorrange.hpp"
#include "enum/videoeffect.hpp"
#include "enum/channellayout.hpp"
#include "enum/subtitledisplay.hpp"
#include "enum/interpolatortype.hpp"
#include "enum/verticalalignment.hpp"
#include "enum/horizontalalignment.hpp"

DECLARE_LOG_CONTEXT(Menu)

auto root_menu_execute(const QString &longId, const QString &argument) -> bool
{
    return RootMenu::execute(longId, argument);
}

template<class T>
auto addEnumAction(Menu &menu, T t, const char *key, bool checkable = false,
                   const char *group = "") -> EnumAction<T>*
{
    static_assert(tmp::is_enum_class<T>(), "wrong type");
    auto action = _NewEnumAction<T>(t);
    action->setCheckable(checkable);
    menu.addActionToGroup(action, key, group);
    return action;
}

template<class T>
auto addEnumActions(Menu &menu, std::initializer_list<T> list,
                    bool checkable = false) -> void
{
    static_assert(!tmp::is_same<T, ChangeValue>(), "oops!");
    const QString g = _L(EnumInfo<T>::typeKey());
    for (const auto &item : list) {
        const auto key = EnumInfo<T>::key(item);
        auto action = menu.addActionToGroup(_NewEnumAction(item), key, g);
        action->setCheckable(checkable);
        if (key == _L("reset"))
            menu.addSeparator();
    }
}

template<class T>
auto addEnumActions(Menu &menu, bool checkable = false) -> void
{
    static_assert(!tmp::is_same<T, ChangeValue>(), "oops!");
    const QString g = _L(EnumInfo<T>::typeKey());
    for (auto &item : EnumInfo<T>::items()) {
        auto act = _NewEnumAction(item.value);
        auto action = menu.addActionToGroup(act, item.key, g);
        action->setCheckable(checkable);
        if (item.key == _L("reset"))
            menu.addSeparator();
    }
}

template<class T>
auto addEnumActionsCheckable(Menu &menu, std::initializer_list<T> list,
                             bool cycle, bool exclusive = true) -> void
{
    if (cycle) {
        const int size = list.size();
        menu.addAction(size > 2 ? "next" : "toggle");
        menu.addSeparator();
    }
    const QString g = _L(EnumInfo<T>::typeKey());
    menu.addGroup(g)->setExclusive(exclusive);
    for (const T &t : list) {
        auto key = EnumInfo<T>::key(t);
        auto action = menu.addActionToGroup(_NewEnumAction(t), key, g);
        action->setCheckable(true);
    }
}

template<class T>
auto addEnumActionsCheckable(Menu &menu, bool cycle,
                             bool exclusive = true) -> void
{
    if (cycle) {
        const int size = EnumInfo<T>::size();
        menu.addAction(size > 2 ? "next" : "toggle");
        menu.addSeparator();
    }
    const QString g = _L(EnumInfo<T>::typeKey());
    menu.addGroup(g)->setExclusive(exclusive);
    for (auto &item : EnumInfo<T>::items()) {
        auto act = _NewEnumAction(item.value);
        auto action = menu.addActionToGroup(act, item.key, g);
        action->setCheckable(true);
    }
}

template<class T>
auto addEnumMenuCheckable(Menu &parent, bool cycle,
                          bool exclusive = true) -> void
{
    auto menu = parent.addMenu(EnumInfo<T>::typeKey());
    addEnumActionsCheckable<T>(*menu, cycle, exclusive);
}

template<class T>
auto updateEnumActions(Menu &menu) -> void
{
    auto actions = menu.g(_L(EnumInfo<T>::typeKey()))->actions();
    if (actions.size() > 2) {
        auto next = _C(menu).a("next");
        if (next)
            next->setText(qApp->translate("RootMenu", "Select Next"));
    } else {
        auto toggle = _C(menu).a("toggle");
        if (toggle)
            toggle->setText(qApp->translate("RootMenu", "Toggle"));
    }
    for (auto a : actions) {
        auto action = static_cast<EnumAction<T>*>(a);
        action->setText(EnumInfo<T>::description(action->enum_()));
    }
}

template<class T>
auto updateEnumMenu(Menu &parent) -> void
{
    const auto desc = EnumInfo<T>::typeDescription();
    updateEnumActions<T>(parent(EnumInfo<T>::typeKey(), desc));
}

RootMenu *RootMenu::obj = nullptr;

static auto addStepPair(Menu &menu, const QString &inc, const QString &dec,
                        const QString &pair, int min, int def, int max,
                        const QString &g = _L("")) -> StepActionPair*
{
    auto p = menu.addStepActionPair(inc, dec, pair, g);
    p->setRange(min, def, max); return p;
}

static auto addStepPair(Menu &menu, const QString &inc,
                        const QString &dec, const QString &pair,
                        const QString &g = _L("")) -> StepActionPair*
{ return addStepPair(menu, inc, dec, pair, _Min<int>(), 0, _Max<int>(), g); }

static auto addStepPair(Menu &menu, int min, int def, int max,
                        const QString &g = _L("")) -> StepActionPair*
{ return addStepPair(menu, "increase", "decrease", "", min, def, max, g); }

static auto addStepReset(Menu &menu, int min, int def, int max,
                         const QString &g = _L("")) -> StepActionPair*
{
    auto reset = new StepAction(ChangeValue::Reset);
    menu.addActionToGroup(reset, _L("reset"), g);
    menu.addSeparator();
    return addStepPair(menu, min, def, max, g);
}

static auto addStepReset(Menu &menu, const QString &g = _L("")) -> StepActionPair*
{ return addStepReset(menu, _Min<int>(), 0, _Max<int>(), g); }


RootMenu::RootMenu(): Menu(_L("menu"), 0) {
    Q_ASSERT(obj == nullptr);
    obj = this;

    setTitle("Root Menu");

    auto &open = *addMenu(_L("open"));
    open.addAction(_L("file"));
    open.addAction(_L("folder"));
    open.addAction(_L("url"));
    open.addAction(_L("dvd"));
    open.addAction(_L("bluray"));
    open.addSeparator();
    auto &recent = *open.addMenu(_L("recent"));
    recent.addSeparator();
    recent.addAction(_L("clear"));

    auto &play = *addMenu(_L("play"));
    play.addAction(_L("pause"));
    play.addAction(_L("stop"));
    play.addSeparator();
    play.addAction(_L("prev"));
    play.addAction(_L("next"));
    play.addSeparator();
    ::addStepReset(*play.addMenu("speed"), 10, 100, 1000);
    auto &repeat = *play.addMenu(_L("repeat"));
    repeat.addActionToGroup(_L("range"), false)->setData(int('r'));
    repeat.addActionToGroup(_L("subtitle"), false)->setData(int('s'));
    repeat.addActionToGroup(_L("quit"), false)->setData(int('q'));
    play.addSeparator();
    play.addAction(_L("disc-menu"));
    auto &seek = *play.addMenu(_L("seek"));
    const QString forward("forward%1"), backward("backward%1");
    const QString seekStep("seek%1");
    for (int i = 1; i <= 3; ++i) {
        auto pair = addStepPair(seek, forward.arg(i), backward.arg(i),
                                seekStep.arg(i), _L("relative"));
        pair->increase()->setTextRate(0.001);
        pair->decrease()->setTextRate(-0.001);
    }
    seek.addSeparator();
    seek.addActionToGroup(_L("prev-frame"), false, _L("frame"))->setData(-1);
    seek.addActionToGroup(_L("next-frame"), false, _L("frame"))->setData(1);
    seek.addSeparator();
    seek.addActionToGroup(_L("prev-subtitle"), false, _L("subtitle"))->setData(-1);
    seek.addActionToGroup(_L("current-subtitle"), false, _L("subtitle"))->setData(0);
    seek.addActionToGroup(_L("next-subtitle"), false, _L("subtitle"))->setData(1);
    play.addMenu(_L("title"))->setEnabled(false);
    auto &chapter = *play.addMenu(_L("chapter"));
    chapter.setEnabled(false);
    chapter.g()->setExclusive(true);
    chapter.addAction(_L("prev"));
    chapter.addAction(_L("next"));
    chapter.addSeparator();

    auto &subtitle = *addMenu(_L("subtitle"));
    auto &spu = *subtitle.addMenu(_L("track"));
    spu.addGroup("internal")->setExclusive(false);
    spu.addGroup("external")->setExclusive(false);
    spu.addAction(_L("open"));
    spu.addAction(_L("auto-load"));
    spu.addAction(_L("reload"));
    spu.addAction(_L("clear"));
    spu.addSeparator();
    spu.addAction(_L("next"));
    spu.addAction(_L("all"));
    spu.addAction(_L("hide"), true);
    spu.addSeparator();
    subtitle.addSeparator();
    addEnumMenuCheckable<SubtitleDisplay>(subtitle, true);
    addEnumActionsCheckable(*subtitle.addMenu("align"),
        {VerticalAlignment::Top, VerticalAlignment::Bottom}, true);
    subtitle.addSeparator();
    addStepReset(*subtitle.addMenu("position"), 0, 100, 100);
    addStepReset(*subtitle.addMenu("sync"))->setTextRate(1e-3);

    auto &video = *addMenu(_L("video"));
    video.addMenu(_L("track"))->setEnabled(false);
    video.addSeparator();
    video.addAction(_L("snapshot"));
    video.addSeparator();
    addEnumActionsCheckable<VideoRatio>(*video.addMenu(_L("aspect")), true);
    addEnumActionsCheckable<VideoRatio>(*video.addMenu(_L("crop")), true);
    auto &align = *video.addMenu(_L("align"));
    addEnumActionsCheckable<VerticalAlignment>(align, false);
    align.addSeparator();
    addEnumActionsCheckable<HorizontalAlignment>(align, false);
    auto &move = *video.addMenu(_L("move"));
    addEnumActions<MoveToward>(move);
    video.addSeparator();

    addEnumMenuCheckable<ColorRange>(video, true);
    addEnumActionsCheckable<InterpolatorType>(*video.addMenu("chroma-upscaler"), true);
    addEnumActionsCheckable<InterpolatorType>(*video.addMenu("interpolator"), true);
    addEnumMenuCheckable<Dithering>(video, true);
    addEnumMenuCheckable<DeintMode>(video, true);

    auto &effect = *video.addMenu(_L("filter"));
    effect.g()->setExclusive(false);
    addEnumAction(effect, VideoEffect::FlipV, "flip-v", true);
    addEnumAction(effect, VideoEffect::FlipH, "flip-h", true);
    effect.addSeparator();
    addEnumAction(effect, VideoEffect::Blur, "blur", true);
    addEnumAction(effect, VideoEffect::Sharpen, "sharpen", true);
    effect.addSeparator();
    addEnumAction(effect, VideoEffect::Gray, "gray", true);
    addEnumAction(effect, VideoEffect::Invert, "invert", true);
    effect.addSeparator();
    addEnumAction(effect, VideoEffect::Disable, "disable", true);

    auto &color = *video.addMenu("color");
    color.addActionToGroup(_L("reset"));
    color.addSeparator();
    VideoColor::for_type([&] (VideoColor::Type type) {
        const auto str = VideoColor::name(type);
        addStepPair(color, str % _L("+"), str % _L("-"), str, -100, 0, 100, str);
    });

    auto &audio = *addMenu(_L("audio"));
    auto &track = *audio.addMenu(_L("track"));
    track.setEnabled(false);
    track.g()->setExclusive(true);
    track.addAction(_L("next"));
    track.addSeparator();
    addStepReset(*audio.addMenu("sync"))->setTextRate(1e-3);
    audio.addSeparator();
    auto &volume = *audio.addMenu("volume");
    volume.addAction(_L("mute"), true);
    volume.addSeparator();
    ::addStepPair(volume, 0, 100, 100);
    addStepReset(*audio.addMenu("amp"), 10, 100, 1000);
    addEnumMenuCheckable<ChannelLayout>(audio, true);
    audio.addSeparator();

    audio.addSeparator();
    audio.addAction(_L("normalizer"), true)->setShortcut(Qt::Key_N);
    audio.addAction(_L("tempo-scaler"), true)->setShortcut(Qt::Key_Z);


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
    tool.addAction(_L("find-subtitle"));
    tool.addAction(_L("playinfo"));
    tool.addSeparator();
    tool.addAction(_L("pref"))->setMenuRole(QAction::PreferencesRole);
    tool.addAction(_L("reload-skin"));
    tool.addSeparator();
    tool.addAction(_L("auto-exit"), true);
    tool.addAction(_L("auto-shutdown"), true);

    auto &window = *addMenu(_L("window"));
    addEnumMenuCheckable<StaysOnTop>(window, true);
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

    play("title").setEnabled(false);
    play("chapter").setEnabled(false);
    video("track").setEnabled(false);
    audio("track").setEnabled(false);

    fillId(this, "");
}

auto RootMenu::execute(const QString &longId, const QString &argument) -> bool
{
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
        _Warn("Cannot execute '%%'", longId);
        return false;
    }
}

auto RootMenu::setShortcuts(const Shortcuts &shortcuts) -> void
{
    for (auto it = shortcuts.cbegin(); it != shortcuts.cend(); ++it) {
        auto id = it.key();
        if (id.startsWith("menu/"))
            id = id.mid(5);
        auto found = m_actions.constFind(id);
        if (found != m_actions.cend())
            found.value().action->setShortcuts(it.value());
        else
            _Warn("Cannot set shortcuts for '%%'", id);
    }
#ifdef Q_OS_MAC
    a(_L("exit"))->setShortcut(QKeySequence());
#endif
}

auto RootMenu::fillId(Menu *menu, const QString &id) -> void
{
    const auto ids = menu->ids();
    for (auto it = ids.cbegin(); it != ids.cend(); ++it) {
        const QString key = id % it.key();
        m_ids[m_actions[key].action = it.value()] = key;
        if (auto menu = qobject_cast<Menu*>(it.value()->menu()))
            fillId(menu, key % "/");
    }
}

auto RootMenu::shortcuts() const -> Shortcuts
{
    Shortcuts keys;
    for (auto it = m_actions.cbegin(); it != m_actions.cend(); ++it) {
        auto shortcuts = it.value().action->shortcuts();
        if (!shortcuts.isEmpty())
            keys[it.key()] = shortcuts;
    }
    return keys;
}

auto RootMenu::retranslate() -> void
{
    auto &root = *this;

    auto &open = root("open", tr("Open"));
    open.a("file", tr("Open File"));
    open.a("folder", tr("Open Folder"));
    open.a("url", tr("Load URL"));
    open.a("dvd", tr("Open DVD"));
    open.a("bluray", tr("Open Blu-ray"));

    auto &recent = open("recent");
    recent.setTitle(tr("Recently Opened"));
    recent.a("clear", tr("Clear"));

    auto &play = root("play", tr("Play"));
    play.a("pause", tr("Play"));
    play.a("stop", tr("Stop"));
    play.a("prev", tr("Play Previous"));
    play.a("next", tr("Play Next"));
    play.a("disc-menu", tr("Disc Menu"));

    auto translateStepMenu = [] (Menu &menu, const QString &format) -> void
    {
        const auto g = menu.g(_L(EnumInfo<ChangeValue>::typeKey()));
        StepAction::setFormat(g->actions(), format);
    };

    translateStepMenu(play("speed", tr("Playback Speed")), "%1%");

    auto &repeat = play("repeat", tr("A-B Repeat"));
    repeat.a("range", tr("Set Range to Current Time"));
    repeat.a("subtitle", tr("Repeat Current Subtitle"));
    repeat.a("quit", tr("Quit"));

    auto &seek = play("seek", tr("Seek"));
    auto translateSeekAction = [&] (const char *prefix, const QString &format) {
        for (int i=1; i<=3; ++i)
            static_cast<StepAction*>(seek[_L(prefix)%_N(i)])->setFormat(format);
    };
    translateSeekAction("forward",  tr("Forward %1sec"));
    translateSeekAction("backward", tr("Backward %1sec"));

    seek.a("prev-frame", tr("Previous Frame"));
    seek.a("next-frame", tr("Next Frame"));

    seek.a("prev-subtitle", tr("To Previous Subtitle"));
    seek.a("current-subtitle", tr("To Beginning of Current Subtitle"));
    seek.a("next-subtitle", tr("To Next Subtitle"));

    play("title", tr("Title"));
    auto &chapter = play("chapter", tr("Chapter"));
    chapter.a("prev", tr("Previous Chapter"));
    chapter.a("next", tr("Next Chapter"));

    auto &sub = root("subtitle", tr("Subtitle"));

    auto &spu = sub("track", tr("Subtitle Track"));
    spu.a("open", tr("Open File(s)"));
    spu.a("auto-load", tr("Auto-load File(s)"));
    spu.a("reload", tr("Reload File(s)"));
    spu.a("clear", tr("Clear File(s)"));
    spu.a("next", tr("Select Next"));
    spu.a("all", tr("Select All"));
    spu.a("hide", tr("Hide"));

    updateEnumMenu<SubtitleDisplay>(sub);
    updateEnumActions<VerticalAlignment>(sub("align", tr("Subtitle Alignment")));
    translateStepMenu(sub("position", tr("Subtitle Position")), "%1%");
    translateStepMenu(sub("sync", tr("Subtitle Sync")), tr("%1sec"));

    auto &video = root("video", tr("Video"));
    video("track", tr("Video Track"));

    updateEnumActions<VideoRatio>(video("aspect", tr("Aspect Ratio")));
    updateEnumActions<VideoRatio>(video("crop", tr("Crop")));

    auto &align = video("align", tr("Screen Alignment"));
    updateEnumActions<VerticalAlignment>(align);
    updateEnumActions<HorizontalAlignment>(align);
    updateEnumActions<MoveToward>(video("move", tr("Screen Position")));

    updateEnumMenu<ColorRange>(video);
    updateEnumActions<InterpolatorType>(video("chroma-upscaler", tr("Chroma Upscaler")));
    updateEnumActions<InterpolatorType>(video("interpolator", tr("Interpolator")));
    updateEnumMenu<Dithering>(video);
    updateEnumMenu<DeintMode>(video);

    auto &effect = video("filter", tr("Filter"));
    effect.a("flip-v", tr("Flip Vertically"));
    effect.a("flip-h", tr("Flip Horizontally"));
    effect.a("blur", tr("Blur"));
    effect.a("sharpen", tr("Sharpen"));
    effect.a("gray", tr("Grayscale"));
    effect.a("invert", tr("Invert Color"));
    effect.a("disable", tr("Disable Filters"));

    auto &color = video("color", tr("Adjust Color"));
    color.a("reset", VideoColor::formatText(VideoColor::TypeMax));
    VideoColor::for_type([&] (VideoColor::Type type) {
        const auto prefix = VideoColor::name(type);
        const auto format = VideoColor::formatText(type);
        StepAction::setFormat(color.g(prefix)->actions(), format);
    });

    video.a("snapshot", tr("Take Snapshot"));

    auto &audio = root("audio", tr("Audio"));
    audio("track", tr("Audio Track")).a("next", tr("Select Next"));
    translateStepMenu(audio("sync", tr("Audio Sync")), tr("%1sec"));
    audio("volume").a("mute", tr("Mute"));
    translateStepMenu(audio("volume", tr("Volume")), "%1%");
    translateStepMenu(audio("amp", tr("Amp")), "%1%");
    updateEnumMenu<ChannelLayout>(audio);
    audio.a("normalizer", tr("Volume Normalizer"));
    audio.a("tempo-scaler", tr("Tempo Scaler"));

    auto &tool = root("tool");
    tool.a("undo", tr("Undo"));
    tool.a("redo", tr("Redo"));
    tool.setTitle(tr("Tools"));
    auto &playlist = tool("playlist");
    playlist.setTitle(tr("Playlist"));
    playlist.a("toggle", tr("Show/Hide"));
    playlist.a("open", tr("Open"));
    playlist.a("save", tr("Save"));
    playlist.a("clear", tr("Clear"));
    playlist.a("append-file", tr("Append File"));
    playlist.a("append-url", tr("Append URL"));
    playlist.a("remove", tr("Remove"));
    playlist.a("move-up", tr("Move Up"));
    playlist.a("move-down", tr("Move Down"));
    tool.a("favorites", tr("Favorites"));
    auto &history = tool("history");
    history.setTitle(tr("History"));
    history.a("toggle", tr("Show/Hide"));
    history.a("clear", tr("Clear"));

    tool.a("find-subtitle", tr("Find Subtitle"));
    tool.a("subtitle", tr("Subtitle View"));
    tool.a("pref", tr("Preferences"));
    tool.a("reload-skin", tr("Reload Skin"));
    tool.a("playinfo", tr("Playback Information"));
    tool.a("auto-exit", tr("Auto-exit"));
    tool.a("auto-shutdown", tr("Auto-shutdown"));

    auto &window = root("window", tr("Window"));
    updateEnumMenu<StaysOnTop>(window);
    window.a("proper", tr("Proper Size"));
    window.a("full", tr("Fullscreen"));
    window.a("minimize", tr("Minimize"));
    window.a("maximize", tr("Maximize"));
    window.a("close", tr("Close"));

    auto &help = root("help");
    help.setTitle(tr("Help"));
    help.a("about", tr("About %1").arg("CMPlayer"));
    root.a("exit", tr("Exit"));
}

auto RootMenu::fillKeyMap(Menu *menu) -> void
{
    for (auto action : menu->actions()) {
        if (action->menu())
            fillKeyMap(static_cast<Menu*>(action->menu()));
        else {
            for (auto key : action->shortcuts())
                m_keymap[key] = action;
        }
    }
}
