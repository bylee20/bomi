#include "trayicon.hpp"
#ifdef Q_OS_LINUX
#include <glib-object.h>
#endif
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(Tray)

enum AppIndicatorCategory { APP_INDICATOR_CATEGORY_APPLICATION_STATUS };
enum AppIndicatorStatus {
    APP_INDICATOR_STATUS_PASSIVE,
    APP_INDICATOR_STATUS_ACTIVE,
    APP_INDICATOR_STATUS_ATTENTION
};

using AppIndicator = void;
using GtkWidget    = void;
using GtkMenu      = void;
using GtkMenuShell = void;

#define GTK_MENU_SHELL(a) a
#define GTK_MENU(a)       a

#ifdef Q_OS_LINUX
#define DEC_FUNC(name, prototype) using f_##name = prototype; static f_##name name = nullptr;
DEC_FUNC(app_indicator_new           , AppIndicator*(*)(const char *id, const char *icon_name, AppIndicatorCategory category))
DEC_FUNC(app_indicator_set_status    , void(*)(AppIndicator *self, AppIndicatorStatus status))
DEC_FUNC(app_indicator_set_menu      , void(*)(AppIndicator *self, GtkMenu *menu))
DEC_FUNC(gtk_menu_new                , GtkWidget*(*)())
DEC_FUNC(gtk_menu_item_new_with_label, GtkWidget*(*)(const char *label))
DEC_FUNC(gtk_menu_shell_append       , void(*)(GtkMenuShell *menu_shell, GtkWidget *child))
DEC_FUNC(gtk_widget_show             , void(*)(GtkWidget *widget))
#undef DEC_FUNC
#endif

struct TrayIcon::Data {
    TrayIcon *p = nullptr;
    bool unity = isUnity();
    QSystemTrayIcon *tray = nullptr;
    AppIndicator *indicator = nullptr;
    GtkMenu *gmenu = nullptr;
};

TrayIcon::TrayIcon(const QIcon &icon, QObject *parent)
: QObject(parent), d(new Data) {
    d->p = this;
    if (!isAvailable())
        return;
#ifdef Q_OS_LINUX
    if (d->unity) {
        d->gmenu = gtk_menu_new();
        auto quit = gtk_menu_item_new_with_label(tr("Quit").toLocal8Bit());
        auto show = gtk_menu_item_new_with_label(tr("Show").toLocal8Bit());
        gtk_menu_shell_append(GTK_MENU_SHELL(d->gmenu), show);
        gtk_menu_shell_append(GTK_MENU_SHELL(d->gmenu), quit);
        gtk_widget_show(show);
        gtk_widget_show(quit);
        g_signal_connect(show, "activate", G_CALLBACK(onShow), this);
        g_signal_connect(quit, "activate", G_CALLBACK(onQuit), this);
        d->indicator = app_indicator_new("net.xylosper.bomi.AppIndicator", "bomi", APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
        app_indicator_set_menu(d->indicator, GTK_MENU(d->gmenu));
        return;
    }
#endif
    d->tray = new QSystemTrayIcon(icon, this);
    connect(d->tray, &QSystemTrayIcon::activated, [this] (QSystemTrayIcon::ActivationReason r) {
        emit activated(static_cast<ActivationReason>(r));
    });
}

TrayIcon::~TrayIcon() {
    delete d->tray;
    delete d;
}

auto TrayIcon::setVisible(bool visible) -> void
{
#ifdef Q_OS_LINUX
    if (d->unity && d->indicator) {
        app_indicator_set_status(d->indicator, visible ? APP_INDICATOR_STATUS_ACTIVE : APP_INDICATOR_STATUS_PASSIVE);
    } else
#endif
    if (d->tray)
        d->tray->setVisible(visible);
}

auto TrayIcon::onShow(void *menu, void *arg) -> void
{
    Q_UNUSED(menu);
    auto p = static_cast<TrayIcon*>(arg);
    emit p->activated(Show);
}

auto TrayIcon::onQuit(void *menu, void *arg) -> void
{
    Q_UNUSED(menu);
    auto p = static_cast<TrayIcon*>(arg);
    emit p->activated(Quit);
}

#ifdef Q_OS_LINUX
static bool tryUnity() {
    static bool init = false;
    static bool good = false;
    if (!init) {
        init = true;
        _Debug("DE is Unity. Fallback to AppIndicator instead of QSytemTrayIcon.");
        QLibrary gtk(u"gtk-x11-2.0"_q, 0), ai(u"libappindicator"_q, 1);
        if (!gtk.load() || !ai.load())
            return false;
        auto lib = &gtk;
#define DEC_FUNC(name) if (!(name = (f_##name)lib->resolve(#name))) return false;
        DEC_FUNC(gtk_menu_new)
        DEC_FUNC(gtk_menu_item_new_with_label)
        DEC_FUNC(gtk_menu_shell_append)
        DEC_FUNC(gtk_widget_show)
        lib = &ai;
        DEC_FUNC(app_indicator_new)
        DEC_FUNC(app_indicator_set_menu)
        DEC_FUNC(app_indicator_set_status)
#undef DEC_FUNC
        good = true;
    }
    return good;
}
#endif

auto TrayIcon::isAvailable() -> bool
{
#ifdef Q_OS_MAC
    return false;
#elif defined(Q_OS_LINUX)
    if (isUnity())
        return tryUnity();
#endif
    return QSystemTrayIcon::isSystemTrayAvailable();
}
