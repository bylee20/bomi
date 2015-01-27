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
#include <functional>

DECLARE_LOG_CONTEXT(Menu)

auto root_menu_execute(const QString &longId, const QString &argument) -> bool
{
    return RootMenu::execute(longId, argument);
}

RootMenu *RootMenu::obj = nullptr;

using GetText = std::function<QString(void)>;
using Translate = std::function<void(void)>;

struct MenuActionInfo
{
    MenuActionInfo() = default;
    MenuActionInfo(const QString &id): id(id) { }
    QString id; Translate trans; const char *desc = nullptr;
};

struct RootMenu::Data {
    Menu *parent = nullptr;
    QHash<QAction*, MenuActionInfo> infos;
    MenuActionInfo *info = nullptr;

    auto toGetText(const char *trans) const -> GetText
        { return [=] () { return tr(trans); }; }

    auto desc(QAction *act, const char *description) -> void
    {
        Q_ASSERT(act && infos.contains(act));
        infos[act].desc = description;
    }
    auto desc(Menu *m, const char *description) -> void
        { desc(m->menuAction(), description); }

    auto newInfo(QAction *action, const QString &key) -> MenuActionInfo*
    {
        auto &info = infos[action];
        const auto &prefix = infos[parent->menuAction()].id;
        info.id = prefix.isEmpty() ? key : (prefix % '/'_q % key);
        obj->m_actions[info.id].action = action;
        this->info = &info;
        return &info;
    }
    auto newInfo(QMenu *menu, const QString &key) -> MenuActionInfo*
        { return newInfo(menu->menuAction(), key); }
    static auto translate(QAction *action, const QString &text) -> void
        { action->setText(text); }
    static auto translate(QMenu *menu, const QString &title) -> void
        { menu->setTitle(title); }

    template<class T>
    auto reg(T *o, const QString &id, GetText &&gt) -> T*
    { newInfo(o, id)->trans = [=] () { translate(o, gt()); }; return o; }

    template<class T>
    auto reg(T *obj, const QString &id, const char *trans) -> T*
    { return reg(obj, id, toGetText(trans)); }

    template<class T>
    auto reg(T *obj, const QString &id, Translate &&translate) -> T*
    { newInfo(obj, id)->trans = std::move(translate); return obj; }

    template<class Func>
    auto menu(const QString &id, GetText &&gt, Func &&func) -> Menu*
    {
        Q_ASSERT(parent);
        auto menu = parent->addMenu(id);
        reg(menu, id, std::move(gt));
        std::swap(parent, menu);
        func();
        std::swap(parent, menu);
        return menu;
    }

    template<class Func>
    auto menu(const QString &id, const char *tr, Func &&func) -> Menu*
    {
        return menu(id, toGetText(tr), std::move(func));
    }

    auto action(const QString &id, const char *tr, bool checkable = false) -> QAction*
    { return reg(parent->addAction(id, checkable), id, tr); }


    auto stepPair(const QString &inc, const QString &dec, GetText &&trans,
                  const QString &pair, int min, int def, int max,
                  const QString &g = QString()) -> StepActionPair*
    {
        auto p = parent->addStepActionPair(inc, dec, pair, g);
        reg(p->increase(), inc, [=] () { p->increase()->setFormat(trans()); });
        reg(p->decrease(), dec, [=] () { p->decrease()->setFormat(trans()); });
        p->setRange(min, def, max);
        return p;
    }

    auto stepPair(const QString &inc, const QString &dec, const char *trans,
                  const QString &pair, int min, int def, int max,
                  const QString &g = QString()) -> StepActionPair*
    {
        auto p = parent->addStepActionPair(inc, dec, pair, g);
        reg(p->increase(), inc, [=] () { p->increase()->setFormat(tr(trans)); });
        reg(p->decrease(), dec, [=] () { p->decrease()->setFormat(tr(trans)); });
        p->setRange(min, def, max);
        return p;
    }

    auto stepPair(const QString &inc, const QString &dec, const char *trans, const QString &pair,
                  const QString &g = QString()) -> StepActionPair*
    { return stepPair(inc, dec, trans, pair, _Min<int>(), 0, _Max<int>(), g); }

    auto stepPair(const char *f, int min, int def, int max,
                  const QString &g = QString()) -> StepActionPair*
    { return stepPair(u"increase"_q, u"decrease"_q, f, QString(), min, def, max, g); }

    auto stepReset(const char *format, int min, int def, int max,
                   const QString &g = u""_q) -> StepActionPair*
    {
        auto reset = new StepAction(ChangeValue::Reset);
        reset->setRange(min, def, max);
        parent->addActionToGroup(reset, u"reset"_q, g);
        reg(reset, u"reset"_q, GetText([] () { return ChangeValueInfo::description(ChangeValue::Reset); }) );
        separator();
        return stepPair(format, min, def, max, g);
    }

    auto stepReset(const char *trans, const QString &g = QString()) -> StepActionPair*
    { return stepReset(trans, _Min<int>(), 0, _Max<int>(), g); }

    auto menuStepReset(const QString &key, const char *tr,
                       const char *format, int min, int def, int max,
                       const QString &g = u""_q) -> Menu*
    { return menu(key, tr, [=] () { stepReset(format, min, def, max, g); }); }

    auto menuStepReset(const QString &key, const char *tr, const char *format,
                       qreal r = 0.0, const QString &g = QString()) -> Menu*
    { return menu(key, tr, [=] () { stepReset(format, g)->setTextRate(r); }); }

    template<class T>
    using EnumItemVector = QVector<const typename EnumInfo<T>::Item*>;

    template<class T>
    auto toItemList(std::initializer_list<T> list) -> EnumItemVector<T>
    {
        EnumItemVector<T> ret; ret.reserve(list.size());
        for (auto t : list)
            ret.push_back(EnumInfo<T>::item(t));
        return ret;
    }

    template<class T>
    auto toItemList() -> EnumItemVector<T>
    {
        auto &items = EnumInfo<T>::items();
        EnumItemVector<T> ret;
        ret.reserve(items.size());
        for (auto &t : items)
            ret.push_back(&t);
        return ret;
    }

    template<class T>
    auto enumAction(T t, const QString &key, GetText &&gt, bool checkable = false,
                    const QString &group = QString()) -> EnumAction<T>*
    {
        auto action = _NewEnumAction<T>(t);
        action->setCheckable(checkable);
        actionToGroup(action, key, std::move(gt), group);
        return action;
    }

    template<class T>
    auto enumAction(T t, const QString &id, bool ch = false,
                    const QString &g = QString()) -> EnumAction<T>*
    { return enumAction(t, id, [=] () { return EnumInfo<T>::description(t); }, ch, g); }

    template<class T>
    auto enumAction(T t, const QString &key, const char *trans, bool ch = false,
                    const QString &g = QString()) -> EnumAction<T>*
    { return enumAction<T>(t, key, toGetText(trans), ch, g); }

    template<class T>
    auto enumActions(const EnumItemVector<T> &items, bool checkable) -> void
    {
        static_assert(!std::is_same<T, ChangeValue>::value, "oops!");
        const QString g = _L(EnumInfo<T>::typeKey());
        for (auto &item : items) {
            enumAction(item->value, item->key, checkable, g);
            if (item->key == u"reset"_q)
                separator();
        }
    }

    template<class T>
    auto enumActions(std::initializer_list<T> list, bool checkable = false) -> void
    { enumActions<T>(toItemList(list), checkable); }

    template<class T>
    auto enumActions(bool checkable = false) -> void
    { enumActions<T>(toItemList<T>(), checkable); }

    template<class T>
    auto enumActionsCheckable(const EnumItemVector<T> &items,
                              bool cycle, bool exclusive = true) -> void
    {
        if (cycle) {
            if (EnumInfo<T>::size() > 2)
                action(u"next"_q, QT_TRANSLATE_NOOP(RootMenu, "Select Next"));
            else
                action(u"toggle"_q, QT_TRANSLATE_NOOP(RootMenu, "Toggle"));
            separator();
        }
        const QString g = _L(EnumInfo<T>::typeKey());
        group(g)->setExclusive(exclusive);
        for (auto item: items)
            enumAction(item->value, item->key, true, g);
    }

    template<class T>
    auto enumActionsCheckable(std::initializer_list<T> list,
                              bool cycle, bool exclusive = true) -> void
    { enumActionsCheckable<T>(toItemList(list), cycle, exclusive); }

    template<class T>
    auto enumActionsCheckable(bool cycle, bool exclusive = true) -> void
    { enumActionsCheckable<T>(toItemList<T>(), cycle, exclusive); }

    template<class T>
    auto enumMenuCheckable(bool cycle, bool exclusive = true) -> Menu*
    { return menu(_L(EnumInfo<T>::typeKey()),
                  [] () { return EnumInfo<T>::typeDescription(); },
                  [=] () { enumActionsCheckable<T>(cycle, exclusive); }); }

    template<class T>
    auto enumMenuCheckable(const QString &key, const char *trans,
                           bool cycle, bool exclusive = true) -> Menu*
    { return menu(key, trans, [=] () { enumActionsCheckable<T>(cycle, exclusive); }); }

    template<class T>
    auto enumMenuCheckable(const QString &key, const char *trans,
                           std::initializer_list<T> list, bool cycle, bool exclusive = true) -> Menu*
    { return menu(key, trans, [=] () { enumActionsCheckable<T>(list, cycle, exclusive); }); }

    auto actionToGroup(QAction *a, const QString &key, GetText &&getText, const QString &g) -> QAction*
    { return reg(parent->addActionToGroup(a, key, g), key, std::move(getText)); }

    auto actionToGroup(const QString &key, const char *trans,
                       bool ch = false, const QString &g = u""_q) -> QAction*
    { return reg(parent->addActionToGroup(key, ch, g), key, trans); }

    auto actionToGroup(const QString &key, GetText &&getText,
                       bool ch = false, const QString &g = u""_q) -> QAction*
    { return reg(parent->addActionToGroup(key, ch, g), key, std::move(getText)); }

    auto separator() -> QAction* { return parent->addSeparator(); }
    auto group(const QString &g = u""_q) -> ActionGroup* { return parent->addGroup(g); }
};

template<class E>
SIA _EnumKey(E e) -> QString { return EnumInfo<E>::key(e); }

RootMenu::RootMenu()
    : Menu(u"menu"_q, 0), d(new Data) {
    Q_ASSERT(obj == nullptr);
    obj = this;

    setTitle(u"Root Menu"_q);

    d->parent = this;
    d->infos[menuAction()] = { };

    d->menu(u"open"_q, QT_TR_NOOP("Open"), [=] () {
        d->action(u"file"_q, QT_TR_NOOP("Open File"));
        d->action(u"folder"_q, QT_TR_NOOP("Open Folder"));
        d->action(u"url"_q, QT_TR_NOOP("Load URL"));
        d->action(u"dvd"_q, QT_TR_NOOP("Open DVD"));
        d->action(u"bluray"_q, QT_TR_NOOP("Open Blu-ray"));

        d->separator();

        d->menu(u"recent"_q, QT_TR_NOOP("Recently Opened"), [=] () {
            d->separator();
            d->action(u"clear"_q, QT_TR_NOOP("Clear"));
        });
    });

    d->menu(u"play"_q, QT_TR_NOOP("Play"), [=] () {
        d->action(u"pause"_q, QT_TR_NOOP("Play"));
        d->action(u"stop"_q, QT_TR_NOOP("Stop"));
        d->separator();
        d->action(u"prev"_q, QT_TR_NOOP("Play Previous"));
        d->action(u"next"_q, QT_TR_NOOP("Play Next"));

        d->separator();

        d->menuStepReset(u"speed"_q, QT_TR_NOOP("Playback Speed"), QT_TR_NOOP("%1%"), 10, 100, 1000);

        d->menu(u"repeat"_q, QT_TR_NOOP("A-B Repeat"), [=] () {
            d->actionToGroup(u"range"_q, QT_TR_NOOP("Set Range to Current Time"))->setData(int('r'));
            d->actionToGroup(u"subtitle"_q, QT_TR_NOOP("Repeat Current Subtitle"))->setData(int('s'));
            d->actionToGroup(u"quit"_q, QT_TR_NOOP("Quit Repetition"))->setData(int('q'));
        });

        d->separator();

        d->action(u"disc-menu"_q, QT_TR_NOOP("Disc Menu"));
        d->menu(u"seek"_q, QT_TR_NOOP("Seek"), [=] () {
            d->action(u"begin"_q, QT_TR_NOOP("To the Beginning"));

            d->separator();

            const QString forward(u"forward%1"_q), backward(u"backward%1"_q);
            const QString seekStep(u"seek%1"_q);
            for (int i = 1; i <= 3; ++i) {
                auto p = d->stepPair(forward.arg(i), backward.arg(i), QT_TR_NOOP("%1sec"),
                                     seekStep.arg(i), u"relative"_q);
                p->increase()->setTextRate(0.001);
                p->decrease()->setTextRate(0.001);
            }

            d->separator();

            d->actionToGroup(u"prev-frame"_q, QT_TR_NOOP("Previous Frame"), false, u"frame"_q)->setData(-1);
            d->actionToGroup(u"next-frame"_q, QT_TR_NOOP("Next Frame"), false, u"frame"_q)->setData(1);
            d->action(u"black-frame"_q, QT_TR_NOOP("Next Black Frame"));

            d->separator();

            d->actionToGroup(u"prev-subtitle"_q, QT_TR_NOOP("To Previous Subtitle"), false, u"subtitle"_q)->setData(-1);
            d->actionToGroup(u"current-subtitle"_q, QT_TR_NOOP("To Beginning of Current Subtitle"), false, u"subtitle"_q)->setData(0);
            d->actionToGroup(u"next-subtitle"_q, QT_TR_NOOP("To Next Subtitle"), false, u"subtitle"_q)->setData(1);
        });
        d->menu(u"title"_q, QT_TR_NOOP("Title"), [=] () {

        })->setEnabled(false);
        d->menu(u"chapter"_q, QT_TR_NOOP("Chapter"), [=] () {
            d->group()->setExclusive(true);
            d->action(u"prev"_q, QT_TR_NOOP("Previous Chapter"));
            d->action(u"next"_q, QT_TR_NOOP("Next Chapter"));
            d->separator();
        })->setEnabled(false);
    });

    d->menu(u"subtitle"_q, QT_TR_NOOP("Subtitle"), [=] () {
        d->menu(u"track"_q, QT_TR_NOOP("Subtitle Track"), [=] () {
            d->group(u"internal"_q)->setExclusive(false);
            d->group(u"external"_q)->setExclusive(false);

            d->desc(d->action(u"open"_q, QT_TR_NOOP("Open File")),
                    QT_TR_NOOP("Open Subtitle File"));
            d->desc(d->action(u"auto-load"_q, QT_TR_NOOP("Auto-load File")),
                    QT_TR_NOOP("Auto-load Subtitle File"));
            d->desc(d->action(u"reload"_q, QT_TR_NOOP("Reload File")),
                    QT_TR_NOOP("Reload Subtitle File"));
            d->desc(d->action(u"clear"_q, QT_TR_NOOP("Clear File")),
                    QT_TR_NOOP("Clear Subtitle File"));

            d->separator();

            d->desc(d->action(u"next"_q, QT_TR_NOOP("Select Next")),
                    QT_TR_NOOP("Select Next Subtitle"));
            d->desc(d->action(u"all"_q, QT_TR_NOOP("Select All")),
                    QT_TR_NOOP("Select All Subtitles"));
            d->desc(d->action(u"hide"_q, QT_TR_NOOP("Hide"), true),
                    QT_TR_NOOP("Hide Subtitles"));

            d->separator();
        });

        d->separator();

        d->enumMenuCheckable<SubtitleDisplay>(true);
        d->enumMenuCheckable<VerticalAlignment>(u"align"_q, QT_TR_NOOP("Subtitle Alignment"),
                             {VerticalAlignment::Top, VerticalAlignment::Bottom}, true);

        d->separator();
        d->menuStepReset(u"position"_q, QT_TR_NOOP("Subtitle Position"), "%1%", 0, 100, 100);
        d->menuStepReset(u"sync"_q, QT_TR_NOOP("Subtitle Sync"), QT_TR_NOOP("%1sec"), 1e-3);
    });

    d->menu(u"video"_q, QT_TR_NOOP("Video"), [=] () {
        d->menu(u"track"_q, QT_TR_NOOP("Video Track"), [=] () { })->setEnabled(false);

        d->separator();

        d->menu(u"snapshot"_q, QT_TR_NOOP("Take Snapshot"), [=] () {
            d->action(u"quick"_q, QT_TR_NOOP("Quick Snapshot"));
            d->action(u"quick-nosub"_q, QT_TR_NOOP("Quick Snapshot(No Subtitles)"));
            d->action(u"tool"_q, QT_TR_NOOP("Snapshot Tool"));
        });

        d->separator();

        d->enumMenuCheckable<VideoRatio>(u"aspect"_q, QT_TR_NOOP("Aspect Ratio"), true);
        d->enumMenuCheckable<VideoRatio>(u"crop"_q, QT_TR_NOOP("Crop"), true);
        d->menu(u"align"_q, QT_TR_NOOP("Screen Alignment"), [=] () {
            d->enumActionsCheckable<VerticalAlignment>(false);
            d->separator();
            d->enumActionsCheckable<HorizontalAlignment>(false);
        });
        d->menu(u"move"_q, QT_TR_NOOP("Screen Position"), [=] () { d->enumActions<MoveToward>(); });

        d->separator();

        d->enumMenuCheckable<ColorSpace>(true);
        d->enumMenuCheckable<ColorRange>(true);

        d->separator();

        d->enumMenuCheckable<Interpolator>(u"chroma-upscaler"_q, QT_TR_NOOP("Chroma Upscaler"), true);
        d->enumMenuCheckable<Interpolator>(u"interpolator"_q, QT_TR_NOOP("Interpolator"), true);
        d->menu(u"hq-scaling"_q, QT_TR_NOOP("High Quality Scaling"), [=] () {
            d->action(u"up"_q, QT_TR_NOOP("High Quality Upscaling"), true);
            d->action(u"down"_q, QT_TR_NOOP("High Quality Downscaling"), true);
        });

        d->separator();

        d->enumMenuCheckable<DeintMode>(true);
        d->enumMenuCheckable<Dithering>(true);
        d->menu(u"filter"_q, QT_TR_NOOP("Filter"), [=] () {
            d->group()->setExclusive(false);

            d->enumAction(VideoEffect::FlipV, u"flip-v"_q, QT_TR_NOOP("Flip Vertically"), true);
            d->enumAction(VideoEffect::FlipH, u"flip-h"_q, QT_TR_NOOP("Flip Horizontally"), true);

            d->separator();

            d->enumAction(VideoEffect::Remap, u"remap"_q, QT_TR_NOOP("Remap"), true);
            d->enumAction(VideoEffect::Gray, u"gray"_q, QT_TR_NOOP("Grayscale"), true);
            d->enumAction(VideoEffect::Invert, u"invert"_q, QT_TR_NOOP("Invert Color"), true);

            d->separator();

            d->enumAction(VideoEffect::Disable, u"disable"_q, QT_TR_NOOP("Disable Filters"), true);
        });
        d->menu(u"color"_q, QT_TR_NOOP("Adjust Color"), [=] () {
            auto format = [] (VideoColor::Type type) -> GetText
                { return [=] () { return VideoColor::formatText(type); }; };
            d->actionToGroup(u"reset"_q, format(VideoColor::TypeMax));
            d->separator();
            VideoColor::for_type([=] (VideoColor::Type type) {
                const auto str = VideoColor::name(type);
                d->stepPair(str % '+'_q, str % '-'_q, format(type), str, -100, 0, 100, str);
            });
        });
    });

    d->menu(u"audio"_q, QT_TR_NOOP("Audio"), [=] () {
        d->menu(u"track"_q, QT_TR_NOOP("Audio Track"), [=] () {
            d->group()->setExclusive(true);
            d->action(u"next"_q, QT_TR_NOOP("Select Next"));
            d->separator();
        })->setEnabled(false);
        d->menuStepReset(u"sync"_q, QT_TR_NOOP("Audio Sync"), QT_TR_NOOP("%1sec"), 1e-3);

        d->separator();

        d->menu(u"volume"_q, QT_TR_NOOP("Volume"), [=] () {
            d->action(u"mute"_q, QT_TR_NOOP("Mute"), true);
            d->separator();
            d->stepPair("%1%", 0, 100, 100);
        });
        d->menuStepReset(u"amp"_q, QT_TR_NOOP("Amp"), "%1%", 10, 100, 1000);
        d->action(u"equalizer"_q, QT_TR_NOOP("Equalizer"));
        d->enumMenuCheckable<ChannelLayout>(true);

        d->separator();

        d->action(u"normalizer"_q, QT_TR_NOOP("Volume Normalizer"), true);
        d->action(u"tempo-scaler"_q, QT_TR_NOOP("Tempo Scaler"), true);
    });

    d->menu(u"tool"_q, QT_TR_NOOP("Tools"), [=] () {
        d->action(u"undo"_q, QT_TR_NOOP("Undo"));
        d->action(u"redo"_q, QT_TR_NOOP("Redo"));

        d->separator();

        d->menu(u"playlist"_q, QT_TR_NOOP("Playlist"), [=] () {
            d->action(u"toggle"_q, QT_TR_NOOP("Show/Hide"));
            d->separator();
            d->action(u"open"_q, QT_TR_NOOP("Open"));
            d->action(u"save"_q, QT_TR_NOOP("Save"));
            d->action(u"clear"_q, QT_TR_NOOP("Clear"));
            d->separator();
            d->action(u"append-file"_q, QT_TR_NOOP("Append File"));
            d->action(u"append-url"_q, QT_TR_NOOP("Append URL"));
            d->action(u"remove"_q, QT_TR_NOOP("Remove"));
            d->separator();
            d->action(u"move-up"_q, QT_TR_NOOP("Move Up"));
            d->action(u"move-down"_q, QT_TR_NOOP("Move Down"));
            d->separator();
            d->action(u"shuffle"_q, QT_TR_NOOP("Shuffle"), true);
            d->action(u"repeat"_q, QT_TR_NOOP("Repeat"), true);
        });
        d->menu(u"history"_q, QT_TR_NOOP("History"), [=] () {
            d->action(u"toggle"_q, QT_TR_NOOP("Show/Hide"));
            d->action(u"clear"_q, QT_TR_NOOP("Clear"));
        });
        d->action(u"find-subtitle"_q, QT_TR_NOOP("Find Subtitle"));
        d->action(u"subtitle"_q, QT_TR_NOOP("Subtitle View"));
        d->action(u"playinfo"_q, QT_TR_NOOP("Playback Information"));

        d->separator();

        d->action(u"pref"_q, QT_TR_NOOP("Preferences"))->setMenuRole(QAction::PreferencesRole);
        d->action(u"reload-skin"_q, QT_TR_NOOP("Reload Skin"));

        d->separator();

        d->action(u"auto-exit"_q, QT_TR_NOOP("Auto-exit"), true);
        d->action(u"auto-shutdown"_q, QT_TR_NOOP("Auto-shutdown"), true);
    });

    d->menu(u"window"_q, QT_TR_NOOP("Window"), [=] () {
        d->enumMenuCheckable<StaysOnTop>(true);

        d->separator();

        d->actionToGroup(u"proper"_q, QT_TR_NOOP("Proper Size"), false, u"size"_q)->setData(0.0);
        d->actionToGroup(u"100%"_q, "100%", false, u"size"_q)->setData(1.0);
        d->actionToGroup(u"200%"_q, "200%", false, u"size"_q)->setData(2.0);
        d->actionToGroup(u"300%"_q, "300%", false, u"size"_q)->setData(3.0);
        d->actionToGroup(u"400%"_q, "400%", false, u"size"_q)->setData(4.0);
        d->actionToGroup(u"full"_q, QT_TR_NOOP("Fullscreen"), false, u"size"_q)->setData(-1.0);

        d->separator();

        d->action(u"minimize"_q, QT_TR_NOOP("Minimize"));
        d->action(u"maximize"_q, QT_TR_NOOP("Maximize"));
        d->action(u"close"_q, QT_TR_NOOP("Close"));
    });

    d->menu(u"help"_q, QT_TR_NOOP("Help"), [=] () {
        d->action(u"about"_q, QT_TR_NOOP("About bomi"))->setMenuRole(QAction::AboutRole);
    });

    d->action(u"exit"_q, QT_TR_NOOP("Exit"))->setMenuRole(QAction::QuitRole);

    Q_ASSERT(d->parent == this);
}

RootMenu::~RootMenu()
{
    delete d;
    obj = nullptr;
}

auto RootMenu::id(QAction *action) const -> QString
{
    return d->infos.value(action).id;
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
    for (auto &info : d->infos) {
        if (info.id.isEmpty())
            continue;
        if (!info.trans)
            _Error("'%%' is not tranlsatable.", info.id);
        else
            info.trans();
    }
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

auto RootMenu::description(const QString &longId) const -> QString
{
    auto action = this->action(longId);
    if (!action)
        return QString();
    auto it = d->infos.find(action);
    if (it == d->infos.end())
        return QString();
    if (it->desc)
        return tr(it->desc);
    if (action->menu())
        return tr("%1 Menu").arg(action->menu()->title());
    if (qobject_cast<BaseEnumAction*>(action)) {
        const auto idx = longId.lastIndexOf('/'_q);
        if (idx != -1) {
            auto menuAction = this->action(longId.left(idx));
            if (menuAction && menuAction->menu())
                return menuAction->menu()->title() % ": "_a % action->text();
        }
    }
    return action->text();
}
