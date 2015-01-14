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
#include "enum/colorspace.hpp"
#include "enum/videoeffect.hpp"
#include "enum/channellayout.hpp"
#include "enum/subtitledisplay.hpp"
#include "enum/interpolator.hpp"
#include "enum/verticalalignment.hpp"
#include "enum/horizontalalignment.hpp"

DECLARE_LOG_CONTEXT(Menu)

auto root_menu_execute(const QString &longId, const QString &argument) -> bool
{
    return RootMenu::execute(longId, argument);
}

template<class T>
auto addEnumAction(Menu &menu, T t, const QString &key, bool checkable = false,
                   const QString &group = QString()) -> EnumAction<T>*
{
    auto action = _NewEnumAction<T>(t);
    action->setCheckable(checkable);
    menu.addActionToGroup(action, key, group);
    return action;
}

template<class T>
auto addEnumActions(Menu &menu, std::initializer_list<T> list,
                    bool checkable = false) -> void
{
    static_assert(!std::is_same<T, ChangeValue>::value, "oops!");
    const QString g = _L(EnumInfo<T>::typeKey());
    for (const auto &item : list) {
        const auto key = EnumInfo<T>::key(item);
        auto action = menu.addActionToGroup(_NewEnumAction(item), key, g);
        action->setCheckable(checkable);
        if (key == u"reset"_q)
            menu.addSeparator();
    }
}

template<class T>
auto addEnumActions(Menu &menu, bool checkable = false) -> void
{
    static_assert(!std::is_same<T, ChangeValue>::value, "oops!");
    const QString g = _L(EnumInfo<T>::typeKey());
    for (auto &item : EnumInfo<T>::items()) {
        auto act = _NewEnumAction(item.value);
        auto action = menu.addActionToGroup(act, item.key, g);
        action->setCheckable(checkable);
        if (item.key == u"reset"_q)
            menu.addSeparator();
    }
}

template<class T>
auto addEnumActionsCheckable(Menu &menu, std::initializer_list<T> list,
                             bool cycle, bool exclusive = true) -> void
{
    if (cycle) {
        const int size = list.size();
        menu.addAction(size > 2 ? u"next"_q : u"toggle"_q);
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
        menu.addAction(size > 2 ? u"next"_q : u"toggle"_q);
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
    auto menu = parent.addMenu(_L(EnumInfo<T>::typeKey()));
    addEnumActionsCheckable<T>(*menu, cycle, exclusive);
}

template<class T>
auto updateEnumActions(Menu &menu) -> void
{
    auto actions = menu.g(_L(EnumInfo<T>::typeKey()))->actions();
    if (actions.size() > 2) {
        auto next = _C(menu).a(u"next"_q);
        if (next)
            next->setText(qApp->translate("RootMenu", "Select Next"));
    } else {
        auto toggle = _C(menu).a(u"toggle"_q);
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
    updateEnumActions<T>(parent(_L(EnumInfo<T>::typeKey()), desc));
}

RootMenu *RootMenu::obj = nullptr;

static auto addStepPair(Menu &menu, const QString &inc, const QString &dec,
                        const QString &pair, int min, int def, int max,
                        const QString &g = QString()) -> StepActionPair*
{
    auto p = menu.addStepActionPair(inc, dec, pair, g);
    p->setRange(min, def, max); return p;
}

static auto addStepPair(Menu &menu, const QString &inc,
                        const QString &dec, const QString &pair,
                        const QString &g = QString()) -> StepActionPair*
{ return addStepPair(menu, inc, dec, pair, _Min<int>(), 0, _Max<int>(), g); }

static auto addStepPair(Menu &menu, int min, int def, int max,
                        const QString &g = QString()) -> StepActionPair*
{ return addStepPair(menu, u"increase"_q, u"decrease"_q, QString(), min, def, max, g); }

static auto addStepReset(Menu &menu, int min, int def, int max,
                         const QString &g = u""_q) -> StepActionPair*
{
    auto reset = new StepAction(ChangeValue::Reset);
    reset->setRange(min, def, max);
    menu.addActionToGroup(reset, u"reset"_q, g);
    menu.addSeparator();
    return addStepPair(menu, min, def, max, g);
}

static auto addStepReset(Menu &menu,
                         const QString &g = QString()) -> StepActionPair*
{ return addStepReset(menu, _Min<int>(), 0, _Max<int>(), g); }


RootMenu::RootMenu(): Menu(u"menu"_q, 0) {
    Q_ASSERT(obj == nullptr);
    obj = this;

    setTitle(u"Root Menu"_q);

    auto &open = *addMenu(u"open"_q);
    open.addAction(u"file"_q);
    open.addAction(u"folder"_q);
    open.addAction(u"url"_q);
    open.addAction(u"dvd"_q);
    open.addAction(u"bluray"_q);
    open.addSeparator();
    auto &recent = *open.addMenu(u"recent"_q);
    recent.addSeparator();
    recent.addAction(u"clear"_q);

    auto &play = *addMenu(u"play"_q);
    play.addAction(u"pause"_q);
    play.addAction(u"stop"_q);
    play.addSeparator();
    play.addAction(u"prev"_q);
    play.addAction(u"next"_q);
    play.addSeparator();
    addStepReset(*play.addMenu(u"speed"_q), 10, 100, 1000);
    auto &repeat = *play.addMenu(u"repeat"_q);
    repeat.addActionToGroup(u"range"_q, false)->setData(int('r'));
    repeat.addActionToGroup(u"subtitle"_q, false)->setData(int('s'));
    repeat.addActionToGroup(u"quit"_q, false)->setData(int('q'));
    play.addSeparator();
    play.addAction(u"disc-menu"_q);
    auto &seek = *play.addMenu(u"seek"_q);
    const QString forward(u"forward%1"_q), backward(u"backward%1"_q);
    const QString seekStep(u"seek%1"_q);
    for (int i = 1; i <= 3; ++i) {
        auto pair = addStepPair(seek, forward.arg(i), backward.arg(i),
                                seekStep.arg(i), u"relative"_q);
        pair->increase()->setTextRate(0.001);
        pair->decrease()->setTextRate(-0.001);
    }
    seek.addSeparator();
    seek.addActionToGroup(u"prev-frame"_q, false, u"frame"_q)->setData(-1);
    seek.addActionToGroup(u"next-frame"_q, false, u"frame"_q)->setData(1);
    seek.addSeparator();
    seek.addActionToGroup(u"prev-subtitle"_q, false, u"subtitle"_q)->setData(-1);
    seek.addActionToGroup(u"current-subtitle"_q, false, u"subtitle"_q)->setData(0);
    seek.addActionToGroup(u"next-subtitle"_q, false, u"subtitle"_q)->setData(1);
    play.addMenu(u"title"_q)->setEnabled(false);
    auto &chapter = *play.addMenu(u"chapter"_q);
    chapter.setEnabled(false);
    chapter.g()->setExclusive(true);
    chapter.addAction(u"prev"_q);
    chapter.addAction(u"next"_q);
    chapter.addSeparator();

    auto &subtitle = *addMenu(u"subtitle"_q);
    auto &spu = *subtitle.addMenu(u"track"_q);
    spu.addGroup(u"internal"_q)->setExclusive(false);
    spu.addGroup(u"external"_q)->setExclusive(false);
    spu.addAction(u"open"_q);
    spu.addAction(u"auto-load"_q);
    spu.addAction(u"reload"_q);
    spu.addAction(u"clear"_q);
    spu.addSeparator();
    spu.addAction(u"next"_q);
    spu.addAction(u"all"_q);
    spu.addAction(u"hide"_q, true);
    spu.addSeparator();
    subtitle.addSeparator();
    addEnumMenuCheckable<SubtitleDisplay>(subtitle, true);
    addEnumActionsCheckable(*subtitle.addMenu(u"align"_q),
        {VerticalAlignment::Top, VerticalAlignment::Bottom}, true);
    subtitle.addSeparator();
    addStepReset(*subtitle.addMenu(u"position"_q), 0, 100, 100);
    addStepReset(*subtitle.addMenu(u"sync"_q))->setTextRate(1e-3);

    auto &video = *addMenu(u"video"_q);
    video.addMenu(u"track"_q)->setEnabled(false);
    video.addSeparator();
    auto &snap = *video.addMenu(u"snapshot"_q);
    snap.addAction(u"quick"_q);
    snap.addAction(u"quick-nosub"_q);
    snap.addAction(u"tool"_q);
    video.addSeparator();
    addEnumActionsCheckable<VideoRatio>(*video.addMenu(u"aspect"_q), true);
    addEnumActionsCheckable<VideoRatio>(*video.addMenu(u"crop"_q), true);
    auto &align = *video.addMenu(u"align"_q);
    addEnumActionsCheckable<VerticalAlignment>(align, false);
    align.addSeparator();
    addEnumActionsCheckable<HorizontalAlignment>(align, false);
    auto &move = *video.addMenu(u"move"_q);
    addEnumActions<MoveToward>(move);
    video.addSeparator();

    addEnumMenuCheckable<ColorSpace>(video, true);
    addEnumMenuCheckable<ColorRange>(video, true);
    addEnumActionsCheckable<Interpolator>(*video.addMenu(u"chroma-upscaler"_q), true);
    addEnumActionsCheckable<Interpolator>(*video.addMenu(u"interpolator"_q), true);
    addEnumMenuCheckable<Dithering>(video, true);
    addEnumMenuCheckable<DeintMode>(video, true);

    auto &effect = *video.addMenu(u"filter"_q);
    effect.g()->setExclusive(false);
    addEnumAction(effect, VideoEffect::FlipV, u"flip-v"_q, true);
    addEnumAction(effect, VideoEffect::FlipH, u"flip-h"_q, true);
    effect.addSeparator();
    addEnumAction(effect, VideoEffect::Remap, u"remap"_q, true);
    addEnumAction(effect, VideoEffect::Gray, u"gray"_q, true);
    addEnumAction(effect, VideoEffect::Invert, u"invert"_q, true);
    effect.addSeparator();
    addEnumAction(effect, VideoEffect::Disable, u"disable"_q, true);

    auto &color = *video.addMenu(u"color"_q);
    color.addActionToGroup(u"reset"_q);
    color.addSeparator();
    VideoColor::for_type([&] (VideoColor::Type type) {
        const auto str = VideoColor::name(type);
        addStepPair(color, str % '+'_q, str % '-'_q, str, -100, 0, 100, str);
    });

    auto &audio = *addMenu(u"audio"_q);
    auto &track = *audio.addMenu(u"track"_q);
    track.setEnabled(false);
    track.g()->setExclusive(true);
    track.addAction(u"next"_q);
    track.addSeparator();
    addStepReset(*audio.addMenu(u"sync"_q))->setTextRate(1e-3);
    audio.addSeparator();
    auto &volume = *audio.addMenu(u"volume"_q);
    volume.addAction(u"mute"_q, true);
    volume.addSeparator();
    ::addStepPair(volume, 0, 100, 100);
    addStepReset(*audio.addMenu(u"amp"_q), 10, 100, 1000);
    addEnumMenuCheckable<ChannelLayout>(audio, true);
    audio.addSeparator();

    audio.addSeparator();
    audio.addAction(u"normalizer"_q, true)->setShortcut(Qt::Key_N);
    audio.addAction(u"tempo-scaler"_q, true)->setShortcut(Qt::Key_Z);


    auto &tool = *addMenu(u"tool"_q);
    tool.addAction(u"undo"_q)->setShortcut(Qt::CTRL + Qt::Key_Z);
    tool.addAction(u"redo"_q)->setShortcut(Qt::CTRL + Qt::SHIFT + Qt::Key_Z);
    tool.addSeparator();
    auto &playlist = *tool.addMenu(u"playlist"_q);
    playlist.addAction(u"toggle"_q)->setShortcut(Qt::Key_L);
    playlist.addSeparator();
    playlist.addAction(u"open"_q);
    playlist.addAction(u"save"_q);
    playlist.addAction(u"clear"_q);
    playlist.addSeparator();
    playlist.addAction(u"append-file"_q);
    playlist.addAction(u"append-url"_q);
    playlist.addAction(u"remove"_q);
    playlist.addSeparator();
    playlist.addAction(u"move-up"_q);
    playlist.addAction(u"move-down"_q);
    playlist.addSeparator();
    playlist.addAction(u"shuffle"_q, true);
    playlist.addAction(u"repeat"_q, true);

    tool.addAction(u"favorites"_q)->setVisible(false);
    auto &history = *tool.addMenu(u"history"_q);
    history.addAction(u"toggle"_q)->setShortcut(Qt::Key_C);
    history.addAction(u"clear"_q);

    tool.addAction(u"subtitle"_q)->setShortcut(Qt::Key_V);
    tool.addAction(u"find-subtitle"_q);
    tool.addAction(u"playinfo"_q);
    tool.addSeparator();
    tool.addAction(u"pref"_q)->setMenuRole(QAction::PreferencesRole);
    tool.addAction(u"reload-skin"_q);
    tool.addSeparator();
    tool.addAction(u"auto-exit"_q, true);
    tool.addAction(u"auto-shutdown"_q, true);

    auto &window = *addMenu(u"window"_q);
    addEnumMenuCheckable<StaysOnTop>(window, true);
    window.addSeparator();
    window.addActionToGroup(u"proper"_q, false, u"size"_q)->setData(0.0);
    window.addActionToGroup(u"100%"_q, false, u"size"_q)->setData(1.0);
    window.addActionToGroup(u"200%"_q, false, u"size"_q)->setData(2.0);
    window.addActionToGroup(u"300%"_q, false, u"size"_q)->setData(3.0);
    window.addActionToGroup(u"400%"_q, false, u"size"_q)->setData(4.0);
    window.addActionToGroup(u"full"_q, false, u"size"_q)->setData(-1.0);
    window.addSeparator();
    window.addAction(u"minimize"_q);
    window.addAction(u"maximize"_q);
    window.addAction(u"close"_q);

    auto &help = *addMenu(u"help"_q);
    help.addAction(u"about"_q)->setMenuRole(QAction::AboutRole);

    addAction(u"exit"_q)->setMenuRole(QAction::QuitRole);

    play(u"title"_q).setEnabled(false);
    play(u"chapter"_q).setEnabled(false);
    video(u"track"_q).setEnabled(false);
    audio(u"track"_q).setEnabled(false);

    fillId(this, QString());
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
        if (id.startsWith(u"menu/"_q))
            id = id.mid(5);
        auto found = m_actions.constFind(id);
        if (found != m_actions.cend())
            found.value().action->setShortcuts(it.value());
        else
            _Warn("Cannot set shortcuts for '%%'", id);
    }
#ifdef Q_OS_MAC
    a(u"exit"_q)->setShortcut(QKeySequence());
#endif
}

auto RootMenu::fillId(Menu *menu, const QString &id) -> void
{
    const auto ids = menu->ids();
    for (auto it = ids.cbegin(); it != ids.cend(); ++it) {
        const QString key = id % it.key();
        m_ids[m_actions[key].action = it.value()] = key;
        if (auto menu = qobject_cast<Menu*>(it.value()->menu()))
            fillId(menu, key % '/'_q);
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

    auto &open = root(u"open"_q, tr("Open"));
    open.a(u"file"_q, tr("Open File"));
    open.a(u"folder"_q, tr("Open Folder"));
    open.a(u"url"_q, tr("Load URL"));
    open.a(u"dvd"_q, tr("Open DVD"));
    open.a(u"bluray"_q, tr("Open Blu-ray"));

    auto &recent = open(u"recent"_q);
    recent.setTitle(tr("Recently Opened"));
    recent.a(u"clear"_q, tr("Clear"));

    auto &play = root(u"play"_q, tr("Play"));
    play.a(u"pause"_q, tr("Play"));
    play.a(u"stop"_q, tr("Stop"));
    play.a(u"prev"_q, tr("Play Previous"));
    play.a(u"next"_q, tr("Play Next"));
    play.a(u"disc-menu"_q, tr("Disc Menu"));

    auto translateStepMenu = [] (Menu &menu, const QString &format) -> void
    {
        const auto g = menu.g(_L(EnumInfo<ChangeValue>::typeKey()));
        StepAction::setFormat(g->actions(), format);
    };

    translateStepMenu(play(u"speed"_q, tr("Playback Speed")), u"%1%"_q);

    auto &repeat = play(u"repeat"_q, tr("A-B Repeat"));
    repeat.a(u"range"_q, tr("Set Range to Current Time"));
    repeat.a(u"subtitle"_q, tr("Repeat Current Subtitle"));
    repeat.a(u"quit"_q, tr("Quit"));

    auto &seek = play(u"seek"_q, tr("Seek"));
    auto translateSeekAction = [&] (const QString &prefix, const QString &format) {
        for (int i=1; i<=3; ++i)
            static_cast<StepAction*>(seek[prefix % _N(i)])->setFormat(format);
    };
    translateSeekAction(u"forward"_q,  tr("Forward %1sec"));
    translateSeekAction(u"backward"_q, tr("Backward %1sec"));

    seek.a(u"prev-frame"_q, tr("Previous Frame"));
    seek.a(u"next-frame"_q, tr("Next Frame"));

    seek.a(u"prev-subtitle"_q, tr("To Previous Subtitle"));
    seek.a(u"current-subtitle"_q, tr("To Beginning of Current Subtitle"));
    seek.a(u"next-subtitle"_q, tr("To Next Subtitle"));

    play(u"title"_q, tr("Title"));
    auto &chapter = play(u"chapter"_q, tr("Chapter"));
    chapter.a(u"prev"_q, tr("Previous Chapter"));
    chapter.a(u"next"_q, tr("Next Chapter"));

    auto &sub = root(u"subtitle"_q, tr("Subtitle"));

    auto &spu = sub(u"track"_q, tr("Subtitle Track"));
    spu.a(u"open"_q, tr("Open File(s)"));
    spu.a(u"auto-load"_q, tr("Auto-load File(s)"));
    spu.a(u"reload"_q, tr("Reload File(s)"));
    spu.a(u"clear"_q, tr("Clear File(s)"));
    spu.a(u"next"_q, tr("Select Next"));
    spu.a(u"all"_q, tr("Select All"));
    spu.a(u"hide"_q, tr("Hide"));

    updateEnumMenu<SubtitleDisplay>(sub);
    updateEnumActions<VerticalAlignment>(sub(u"align"_q, tr("Subtitle Alignment")));
    translateStepMenu(sub(u"position"_q, tr("Subtitle Position")), u"%1%"_q);
    translateStepMenu(sub(u"sync"_q, tr("Subtitle Sync")), tr("%1sec"));

    auto &video = root(u"video"_q, tr("Video"));
    video(u"track"_q, tr("Video Track"));
    auto &snap = video(u"snapshot"_q, tr("Take Snapshot"));
    snap.a(u"quick"_q, tr("Quick Snapshot"));
    snap.a(u"quick-nosub"_q, tr("Quick Snapshot(No Subtitles)"));
    snap.a(u"tool"_q, tr("Snapshot Tool"));

    updateEnumActions<VideoRatio>(video(u"aspect"_q, tr("Aspect Ratio")));
    updateEnumActions<VideoRatio>(video(u"crop"_q, tr("Crop")));

    auto &align = video(u"align"_q, tr("Screen Alignment"));
    updateEnumActions<VerticalAlignment>(align);
    updateEnumActions<HorizontalAlignment>(align);
    updateEnumActions<MoveToward>(video(u"move"_q, tr("Screen Position")));

    updateEnumMenu<ColorSpace>(video);
    updateEnumMenu<ColorRange>(video);
    updateEnumActions<Interpolator>(video(u"chroma-upscaler"_q, tr("Chroma Upscaler")));
    updateEnumActions<Interpolator>(video(u"interpolator"_q, tr("Interpolator")));
    updateEnumMenu<Dithering>(video);
    updateEnumMenu<DeintMode>(video);

    auto &effect = video(u"filter"_q, tr("Filter"));
    effect.a(u"flip-v"_q, tr("Flip Vertically"));
    effect.a(u"flip-h"_q, tr("Flip Horizontally"));
    effect.a(u"remap"_q, tr("Remap"));
    effect.a(u"gray"_q, tr("Grayscale"));
    effect.a(u"invert"_q, tr("Invert Color"));
    effect.a(u"disable"_q, tr("Disable Filters"));

    auto &color = video(u"color"_q, tr("Adjust Color"));
    color.a(u"reset"_q, VideoColor::formatText(VideoColor::TypeMax));
    VideoColor::for_type([&] (VideoColor::Type type) {
        const auto prefix = VideoColor::name(type);
        const auto format = VideoColor::formatText(type);
        StepAction::setFormat(color.g(prefix)->actions(), format);
    });

    auto &audio = root(u"audio"_q, tr("Audio"));
    audio(u"track"_q, tr("Audio Track")).a(u"next"_q, tr("Select Next"));
    translateStepMenu(audio(u"sync"_q, tr("Audio Sync")), tr("%1sec"));
    audio(u"volume"_q).a(u"mute"_q, tr("Mute"));
    translateStepMenu(audio(u"volume"_q, tr("Volume")), u"%1%"_q);
    translateStepMenu(audio(u"amp"_q, tr("Amp")), u"%1%"_q);
    updateEnumMenu<ChannelLayout>(audio);
    audio.a(u"normalizer"_q, tr("Volume Normalizer"));
    audio.a(u"tempo-scaler"_q, tr("Tempo Scaler"));

    auto &tool = root(u"tool"_q);
    tool.a(u"undo"_q, tr("Undo"));
    tool.a(u"redo"_q, tr("Redo"));
    tool.setTitle(tr("Tools"));

    auto &playlist = tool(u"playlist"_q);
    playlist.setTitle(tr("Playlist"));
    playlist.a(u"toggle"_q, tr("Show/Hide"));
    playlist.a(u"open"_q, tr("Open"));
    playlist.a(u"save"_q, tr("Save"));
    playlist.a(u"clear"_q, tr("Clear"));
    playlist.a(u"append-file"_q, tr("Append File"));
    playlist.a(u"append-url"_q, tr("Append URL"));
    playlist.a(u"remove"_q, tr("Remove"));
    playlist.a(u"move-up"_q, tr("Move Up"));
    playlist.a(u"move-down"_q, tr("Move Down"));
    playlist.a(u"shuffle"_q, tr("Shuffle"));
    playlist.a(u"repeat"_q, tr("Repeat"));

    tool.a(u"favorites"_q, tr("Favorites"));
    auto &history = tool(u"history"_q);
    history.setTitle(tr("History"));
    history.a(u"toggle"_q, tr("Show/Hide"));
    history.a(u"clear"_q, tr("Clear"));

    tool.a(u"find-subtitle"_q, tr("Find Subtitle"));
    tool.a(u"subtitle"_q, tr("Subtitle View"));
    tool.a(u"pref"_q, tr("Preferences"));
    tool.a(u"reload-skin"_q, tr("Reload Skin"));
    tool.a(u"playinfo"_q, tr("Playback Information"));
    tool.a(u"auto-exit"_q, tr("Auto-exit"));
    tool.a(u"auto-shutdown"_q, tr("Auto-shutdown"));

    auto &window = root(u"window"_q, tr("Window"));
    updateEnumMenu<StaysOnTop>(window);
    window.a(u"proper"_q, tr("Proper Size"));
    window.a(u"full"_q, tr("Fullscreen"));
    window.a(u"minimize"_q, tr("Minimize"));
    window.a(u"maximize"_q, tr("Maximize"));
    window.a(u"close"_q, tr("Close"));

    auto &help = root(u"help"_q);
    help.setTitle(tr("Help"));
    help.a(u"about"_q, tr("About %1").arg(u"bomi"_q));
    root.a(u"exit"_q, tr("Exit"));
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
