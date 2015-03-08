#include "rootmenu.hpp"
#include "shortcutmap.hpp"
#include "video/videocolor.hpp"
#include "misc/log.hpp"
#include "misc/stepactionpair.hpp"
#include "misc/encodinginfo.hpp"
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

using GetText = std::function<QString(void)>;
using Translate = std::function<void(void)>;

struct MenuActionInfo
{
    MenuActionInfo() = default;
    Translate trans; const char *desc = nullptr;
};

struct RootMenu::Data {
    Menu *parent = nullptr;
    QHash<QAction*, MenuActionInfo> infos;
    MenuActionInfo *info = nullptr;
    QMap<QString, QString> alias;
    QMap<QString, QAction*> actions;

    auto find(const QString &longId) const -> QAction*
    {
        auto it = actions.find(longId);
        if (it == actions.end()) {
            auto ait = alias.find(longId);
            if (ait != alias.end())
                it = actions.find(*ait);
        }
        return it != actions.end() ? *it : nullptr;
    }

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
        const auto &prefix = parent->menuAction()->objectName();
        const QString id = prefix.isEmpty() ? key : (prefix % '/'_q % key);
        action->setObjectName(id);
        actions[id] = action;
        this->info = &info;
        return &info;
    }
    auto newInfo(QMenu *menu, const QString &key) -> MenuActionInfo*
        { return newInfo(menu->menuAction(), key); }
    static auto translate(QAction *action, const QString &text) -> void
        { if (!text.isEmpty()) action->setText(text); }
    static auto translate(QMenu *menu, const QString &title) -> void
        { if (!title.isEmpty()) menu->setTitle(title); }

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


    auto stepPair(const QString &inc, const QString &dec,
                  const QString &pair, const QString &g = QString()) -> StepActionPair*
    {
        auto p = parent->addStepActionPair(inc, dec, pair, g);
        reg(p->increase(), inc, [=] () { p->increase()->retranslate(); });
        reg(p->decrease(), dec, [=] () { p->decrease()->retranslate(); });
        return p;
    }

    auto stepPair(const QString &g = QString()) -> StepActionPair*
    { return stepPair(u"increase"_q, u"decrease"_q, QString(), g); }

    auto stepReset(const QString &g = u""_q) -> StepActionPair*
    {
        auto reset = new StepAction(ChangeValue::Reset);
        parent->addActionToGroup(reset, u"reset"_q, g);
        reg(reset, u"reset"_q, [=] () { reset->setText(tr("Reset")); });
        separator();
        return stepPair(g);
    }

    auto menuStepReset(const QString &key, const char *tr,
                       const QString &g = u""_q) -> Menu*
    { return menu(key, tr, [=] () { stepReset(g); }); }

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
        const QString g = _L(EnumInfo<T>::typeKey());
        if (cycle) {
            const auto toggle = EnumInfo<T>::size() <= 2;
            if (toggle)
                action(u"cycle"_q, QT_TRANSLATE_NOOP(RootMenu, "Toggle"));
            else
                action(u"cycle"_q, QT_TRANSLATE_NOOP(RootMenu, "Select Next"));
            separator();
        }
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

    Q_UNUSED(QT_TR_NOOP("Toggle")); // dummy to tranlsate

#define ALIAS(from, to) {d->alias[u"" #from ""_q] = u"" #to ""_q;}
    ALIAS(audio/channel/next, audio/channel/cycle);
    ALIAS(audio/track/next, audio/track/cycle);
    ALIAS(subtitle/track/next, subtitle/track/cycle);
    ALIAS(video/dithering/next, video/dithering/cycle);
    ALIAS(video/interpolator/next, video/interpolator/cycle);
    ALIAS(video/deinterlacing/toggle, video/deinterlacing/cycle);
#undef ALIAS

//    setTitle(u"Root Menu"_q);

    d->parent = this;
    d->infos[menuAction()] = { };

    d->menu(u"open"_q, QT_TR_NOOP("Open"), [=] () {
        d->action(u"file"_q, QT_TR_NOOP("File"));
        d->action(u"folder"_q, QT_TR_NOOP("Folder"));
        d->action(u"url"_q, QT_TR_NOOP("URL"));
        d->action(u"dvd"_q, QT_TR_NOOP("DVD"));
        d->action(u"bluray"_q, QT_TR_NOOP("Blu-ray"));

        d->separator();

        d->menu(u"recent"_q, QT_TR_NOOP("Recent"), [=] () {
            d->separator();
            d->action(u"clear"_q, QT_TR_NOOP("Clear"));
        });
    });

    d->menu(u"play"_q, QT_TR_NOOP("Play"), [=] () {
        d->action(u"pause"_q, QT_TR_NOOP("Start"));
        d->action(u"stop"_q, QT_TR_NOOP("Stop"));
        d->separator();
        d->action(u"prev"_q, QT_TR_NOOP("Previous"));
        d->action(u"next"_q, QT_TR_NOOP("Next"));

        d->separator();

        d->menuStepReset(u"speed"_q, QT_TR_NOOP("Speed"));

        d->menu(u"repeat"_q, QT_TR_NOOP("A-B Repeat"), [=] () {
            d->actionToGroup(u"range"_q, QT_TR_NOOP("Set Range to Current Time"))->setData(int('r'));
            d->actionToGroup(u"subtitle"_q, QT_TR_NOOP("Repeat Current Subtitle"))->setData(int('s'));
            d->actionToGroup(u"quit"_q, QT_TR_NOOP("Quit Repetition"))->setData(int('q'));
        });

        d->separator();

        d->action(u"disc-menu"_q, QT_TR_NOOP("Disc Menu"));
        d->menu(u"seek"_q, QT_TR_NOOP("Seek"), [=] () {
            d->action(u"begin"_q, QT_TR_NOOP("Beginning"));

            d->separator();

            const QString forward(u"forward%1"_q), backward(u"backward%1"_q);
            const QString seekStep(u"seek%1"_q);
            for (int i = 1; i <= 3; ++i) {
                d->stepPair(forward.arg(i), backward.arg(i), seekStep.arg(i), u"relative"_q);
            }

            d->separator();

            d->actionToGroup(u"prev-frame"_q, QT_TR_NOOP("Previous Frame"), false, u"frame"_q)->setData(-1);
            d->actionToGroup(u"next-frame"_q, QT_TR_NOOP("Next Frame"), false, u"frame"_q)->setData(1);
            d->action(u"black-frame"_q, QT_TR_NOOP("Next Black Frame"));

            d->separator();

            d->actionToGroup(u"prev-subtitle"_q, QT_TR_NOOP("Previous Subtitle"), false, u"subtitle"_q)->setData(-1);
            d->actionToGroup(u"current-subtitle"_q, QT_TR_NOOP("Current Subtitle"), false, u"subtitle"_q)->setData(0);
            d->actionToGroup(u"next-subtitle"_q, QT_TR_NOOP("Next Subtitle"), false, u"subtitle"_q)->setData(1);
        });
        d->menu(u"title"_q, QT_TR_NOOP("Title/Edition"), [=] () {
            d->group()->setExclusive(true);
        })->setEnabled(false);
        d->menu(u"chapter"_q, QT_TR_NOOP("Chapter"), [=] () {
            d->group()->setExclusive(true);
            d->action(u"prev"_q, QT_TR_NOOP("Previous"));
            d->action(u"next"_q, QT_TR_NOOP("Next"));
            d->separator();
        })->setEnabled(false);

        d->separator() ;

        d->action(u"state"_q, QT_TR_NOOP("Show State"));
    });

    d->menu(u"video"_q, QT_TR_NOOP("Video"), [=] () {
        d->menu(u"track"_q, QT_TR_NOOP("Track"), [=] () { })->setEnabled(false);

        d->separator();

        d->menu(u"snapshot"_q, QT_TR_NOOP("Take Snapshot"), [=] () {
            d->action(u"quick"_q, QT_TR_NOOP("Quick Snapshot"));
            d->action(u"quick-nosub"_q, QT_TR_NOOP("Quick Snapshot(No Subtitles)"));
            d->action(u"tool"_q, QT_TR_NOOP("Snapshot Tool"));
        });

        d->separator();

        d->menu(u"aspect"_q, QT_TR_NOOP("Aspect Ratio"), [=] () {
            d->action(u"cycle"_q, QT_TR_NOOP("Select Next"));
            d->separator();
            auto g = u"preset"_q;
            d->group(g)->setExclusive(true);
            d->actionToGroup(u"source"_q, QT_TR_NOOP("Same as Source"), true, g)->setData(-1.0);
            d->actionToGroup(u"window"_q, QT_TR_NOOP("Same as Window"), true, g)->setData(0.0);
            d->actionToGroup(u"4:3"_q, QT_TR_NOOP("4:3 (TV)"), true, g)->setData(4./3.);
            d->actionToGroup(u"16:10"_q, QT_TR_NOOP("16:10 (Wide Monitor)"), true, g)->setData(16./10.);
            d->actionToGroup(u"16:9"_q, QT_TR_NOOP("16:9 (HDTV)"), true, g)->setData(16./9.);
            d->actionToGroup(u"1.85:1"_q, QT_TR_NOOP("1.85:1 (Wide Vision)"), true, g)->setData(1.85);
            d->actionToGroup(u"2.35:1"_q, QT_TR_NOOP("2.35:1 (CinemaScope)"), true, g)->setData(2.35);
            d->separator();
            d->stepPair(u"adjust"_q);
        });
        d->enumMenuCheckable<VideoRatio>(u"crop"_q, QT_TR_NOOP("Crop"), true);
        d->menuStepReset(u"zoom"_q, QT_TR_NOOP("Zoom"));

        d->menu(u"move"_q, QT_TR_NOOP("Screen Position"), [=] () {
            d->action(u"reset"_q, QT_TR_NOOP("Reset"));
            d->separator();
            auto add = [&] (const QString &pre, const char *format) -> void
            {
                const auto p = d->stepPair(pre % '+'_q, pre % '-'_q, pre, pre);
                p->setFormatter([=] () { return tr(format); });
            };
            add(u"horizontal"_q, QT_TR_NOOP("Horizontally %1"));
            d->separator();
            add(u"vertical"_q, QT_TR_NOOP("Vertically %1"));
        });

        d->menu(u"align"_q, QT_TR_NOOP("Screen Alignment"), [=] () {
            d->enumActionsCheckable<VerticalAlignment>(false);
            d->separator();
            d->enumActionsCheckable<HorizontalAlignment>(false);
        });

        d->separator();

        d->enumMenuCheckable<ColorSpace>(true);
        d->enumMenuCheckable<ColorRange>(true);

        d->separator();

        d->menu(u"chroma-upscaler"_q, QT_TR_NOOP("Chroma Upscaler"), [=] () {
            d->action(u"advanced"_q, QT_TR_NOOP("Advanced..."));
            d->enumActionsCheckable<Interpolator>(true);
        });
        d->menu(u"interpolator"_q, QT_TR_NOOP("Interpolator"), [=] () {
            d->action(u"advanced"_q, QT_TR_NOOP("Advanced..."));
            d->enumActionsCheckable<Interpolator>(true);
        });
        d->menu(u"hq-scaling"_q, QT_TR_NOOP("High Quality Scaling"), [=] () {
            d->action(u"up"_q, QT_TR_NOOP("Upscaling"), true);
            d->action(u"down"_q, QT_TR_NOOP("Downscaling"), true);
        });

        d->action(u"motion"_q, QT_TR_NOOP("Motion Smoothing"), true);

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
            d->action(u"editor"_q, QT_TR_NOOP("Color Editor"), false);
            auto format = [] (VideoColor::Type type) -> GetText
                { return [=] () { return VideoColor::formatText(type); }; };
            d->actionToGroup(u"reset"_q, format(VideoColor::TypeMax));
            d->separator();
            VideoColor::for_type([=] (VideoColor::Type type) {
                const auto str = VideoColor::name(type);
                const auto pair = d->stepPair(str % '+'_q, str % '-'_q, str, str);
                pair->setFormatter([=]() { return VideoColor::formatText(type); });
            });
        });
    });

    d->menu(u"audio"_q, QT_TR_NOOP("Audio"), [=] () {
        d->menu(u"track"_q, QT_TR_NOOP("Track"), [=] () {
            d->desc(d->action(u"open"_q, QT_TR_NOOP("Open File")),
                    QT_TR_NOOP("Open Audio Track File"));
            d->desc(d->action(u"auto-load"_q, QT_TR_NOOP("Auto-load File")),
                    QT_TR_NOOP("Auto-load Audio Track File"));
            d->desc(d->action(u"reload"_q, QT_TR_NOOP("Reload File")),
                    QT_TR_NOOP("Reload Audio Track File"));
            d->desc(d->action(u"clear"_q, QT_TR_NOOP("Clear File")),
                    QT_TR_NOOP("Clear Audio Track File"));
            d->group()->setExclusive(true);
            d->separator();
            d->desc(d->action(u"cycle"_q, QT_TR_NOOP("Select Next")),
                    QT_TR_NOOP("Select Next Audio Track"));
            d->separator();
        })->setEnabled(false);
        d->menuStepReset(u"sync"_q, QT_TR_NOOP("Sync"));

        d->separator();

        d->menu(u"volume"_q, QT_TR_NOOP("Volume"), [=] () {
            d->action(u"mute"_q, QT_TR_NOOP("Mute"), true);
            d->separator();
            d->stepPair();
        });
        d->menuStepReset(u"amp"_q, QT_TR_NOOP("Amp"));
        d->action(u"equalizer"_q, QT_TR_NOOP("Equalizer"));
        d->enumMenuCheckable<ChannelLayout>(true);

        d->separator();

        d->action(u"normalizer"_q, QT_TR_NOOP("Normalizer"), true);
        d->action(u"tempo-scaler"_q, QT_TR_NOOP("Tempo Scaler"), true);
    });


    d->menu(u"subtitle"_q, QT_TR_NOOP("Subtitle"), [=] () {
        d->menu(u"track"_q, QT_TR_NOOP("Track"), [=] () {
            d->group(u"exclusive"_q)->setExclusive(false);
            d->group(u"inclusive"_q)->setExclusive(false);

            d->action(u"open"_q, QT_TR_NOOP("Open File"));
            d->action(u"auto-load"_q, QT_TR_NOOP("Auto-load File"));
            d->menu(u"reload"_q, QT_TR_NOOP("Reload File"), [=] () {
                d->actionToGroup(u"current"_q, QT_TR_NOOP("Current Encoding"))->setData(-1);
                d->actionToGroup(u"auto"_q, QT_TR_NOOP("Autodetect Encoding"))->setData(0);
                d->separator();
                auto g = d->group();
                for (auto &c : EncodingInfo::grouped()) {
                    if (c.isEmpty())
                        continue;
                    const auto &e = c.front();
                    auto mkey = e.group().toLower();
                    auto title = e.group();
                    if (!e.subgroup().isEmpty()) {
                        mkey += '-'_q % e.subgroup().toLower();
                        title += " ("_a % e.subgroup() % ')'_q;
                    }
                    d->menu(mkey, "", [&] () {
                        for (auto &e : c) {
                            auto a = d->action(e.name().toLower(), "");
                            a->setText(e.name());
                            a->setData(e.mib());
                            g->addAction(a);
                        }
                    })->setTitle(title);
                }
            });
            d->action(u"clear"_q, QT_TR_NOOP("Clear File"));

            d->separator();

            d->action(u"cycle"_q, QT_TR_NOOP("Select Next"));
            d->action(u"all"_q, QT_TR_NOOP("Select All"));
            d->action(u"hide"_q, QT_TR_NOOP("Hide"), true);

            d->separator();
        })->setEnabled(false);

        d->separator();

        d->menu(u"override-ass"_q, QT_TR_NOOP("Override ASS"), [=] () {
            d->action(u"position"_q, QT_TR_NOOP("Position"), true);
            d->action(u"scale"_q, QT_TR_NOOP("Scale"), true);
            d->action(u"text"_q, QT_TR_NOOP("Text Style"), true);
        });
        d->enumMenuCheckable<VerticalAlignment>(u"align"_q, QT_TR_NOOP("Alignment"),
                             {VerticalAlignment::Top, VerticalAlignment::Bottom}, true);
        d->menu(u"position"_q, QT_TR_NOOP("Position"), [=] () {
            d->stepReset(u"adjust"_q);
            d->separator();
            d->enumActions<SubtitleDisplay>(true);
        });
        d->menuStepReset(u"scale"_q, QT_TR_NOOP("Scale"));

        d->separator();

        d->menu(u"sync"_q, QT_TR_NOOP("Sync"), [&] () {
            d->stepReset(u"step"_q);
            d->separator();
            d->actionToGroup(u"prev"_q, QT_TR_NOOP("Bring Previous Lines"), false, u"bring"_q)->setData(-1);
            d->actionToGroup(u"next"_q, QT_TR_NOOP("Bring Next Lines"), false, u"bring"_q)->setData(1);
        });
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
        d->action(u"subtitle"_q, QT_TR_NOOP("Subtitle Viewer"));
        d->action(u"playinfo"_q, QT_TR_NOOP("Playback Information"));
        d->action(u"log"_q, QT_TR_NOOP("Log Viewer"));
        d->separator();

        d->action(u"pref"_q, QT_TR_NOOP("Preferences"))->setMenuRole(QAction::PreferencesRole);
        d->action(u"reload-skin"_q, QT_TR_NOOP("Reload Skin"));

        d->separator();

        d->action(u"auto-exit"_q, QT_TR_NOOP("Auto-exit"), true);
        d->action(u"auto-shutdown"_q, QT_TR_NOOP("Auto-shutdown"), true);
    });

    d->menu(u"window"_q, QT_TR_NOOP("Window"), [=] () {
        d->enumMenuCheckable<StaysOnTop>(true);
        d->action(u"frameless"_q, QT_TR_NOOP("Remove Frame"), true);

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

    d->action(u"context-menu"_q, QT_TR_NOOP("Show Context Menu"));
    d->action(u"exit"_q, QT_TR_NOOP("Exit"))->setMenuRole(QAction::QuitRole);

    Q_ASSERT(d->parent == this);
}

RootMenu::~RootMenu()
{
    delete d;
}

static RootMenu *obj = nullptr;

auto RootMenu::instance() -> RootMenu&
{
    if (!obj)
        obj = new RootMenu;
    return*obj;
}

auto RootMenu::finalize() -> void
{
    _Delete(obj);
}

auto RootMenu::execute(const QString &id) -> bool
{
    if (auto action = RootMenu::instance().d->find(id)) {
        if (action->menu())
            action->menu()->exec(QCursor::pos());
        else
            action->trigger();
        return true;
    } else {
        _Warn("Cannot execute '%%'", id);
        return false;
    }
}

auto RootMenu::setShortcutMap(const ShortcutMap &map) -> void
{
    m_keymap.clear();
    for (auto it = d->actions.cbegin(); it != d->actions.cend(); ++it) {
        const auto &keys = map.keys(it.key());
        const auto action = *it;
        action->setShortcuts(keys);
        for (auto &key : keys) {
            if (!key.isEmpty())
                m_keymap[key] = action;
        }
    }
#ifdef Q_OS_MAC
    a(u"exit"_q)->setShortcut(QKeySequence());
#endif
}

auto RootMenu::retranslate() -> void
{
    for (auto it = d->infos.begin(); it != d->infos.end(); ++it) {
        if (it.key()->objectName().isEmpty())
            continue;
        if (!it->trans)
            _Error("'%%' is not tranlsatable.", it.key()->objectName());
        else
            it->trans();
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
    QString desc = action->text();
    Menu *menu = nullptr;
    if (action->menu())
        menu = qobject_cast<Menu*>(action->menu()->parent());
    else
        menu = qobject_cast<Menu*>(action->parent());
    while (menu && menu != this) {
        desc = menu->title() % '>'_q % desc;
        menu = qobject_cast<Menu*>(menu->parent());
        Q_ASSERT(menu);
    }
    return desc;
}

auto RootMenu::action(const QString &id) const -> QAction*
{
    return d->find(id);
}

struct DumpInfo {
    QString id, desc;
    auto operator < (const DumpInfo &rhs) const -> bool { return id < rhs.id; }
};

auto RootMenu::dumpInfo() -> void
{
    auto &menu = instance();
    menu.retranslate();
    auto d = menu.d;
    int width = 0;
    for (auto it = d->actions.begin(); it != d->actions.end(); ++it)
        width = std::max(width, it.key().size());
    QByteArray fill;
    for (auto it = d->actions.begin(); it != d->actions.end(); ++it) {
        const auto id = it.key();
        fill.resize(width - id.size());
        fill.fill(' ');
        qDebug().nospace() << id.toLatin1().constData() << fill.constData()
                           << " (" << menu.description(it.key()).toLatin1().constData() << ')';
    }
}
