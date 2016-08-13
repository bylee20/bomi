#include "x11.hpp"

#ifdef Q_OS_LINUX

#include "tmp/algorithm.hpp"
#include "enum/codecid.hpp"
#include <QDesktopWidget>
#include <QMouseEvent>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtX11Extras/QX11Info>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>
#include <sys/sysctl.h>
#include <sys/types.h>
#include <xcb/xcb.h>
#include <xcb/randr.h>
#include <xcb/xproto.h>
#include <xcb/screensaver.h>
#include <xcb/xtest.h>

extern "C" {
#if HAVE_VAAPI
#include <video/vaapi.h>
#endif
#if HAVE_VDPAU
#include <video/vdpau.h>
#endif
#include <video/img_format.h>
#include <video/mp_image.h>
#include <video/mp_image_pool.h>
}

#ifdef bool
#undef bool
#endif

namespace OS {

DECLARE_LOG_CONTEXT(X11)

enum XcbAtom {
    _NET_WM_STATE,
    _NET_WM_STATE_MODAL,
    _NET_WM_STATE_STICKY,
    _NET_WM_STATE_MAXIMIZED_VERT,
    _NET_WM_STATE_MAXIMIZED_HORZ,
    _NET_WM_STATE_SHADED,
    _NET_WM_STATE_SKIP_TASKBAR,
    _NET_WM_STATE_SKIP_PAGER,
    _NET_WM_STATE_HIDDEN,
    _NET_WM_STATE_FULLSCREEN,
    _NET_WM_STATE_ABOVE,
    _NET_WM_STATE_BELOW,
    _NET_WM_STATE_DEMANDS_ATTENTION,
    _NET_WM_STATE_STAYS_ON_TOP,
    _NET_WM_MOVERESIZE,
    XcbAtomEnd,
    XcbAtomBegin = _NET_WM_STATE
};

static auto getAtom(xcb_connection_t *conn, const char *name) -> xcb_atom_t
{
    auto cookie = xcb_intern_atom(conn, 0, strlen(name), name);
    auto reply = xcb_intern_atom_reply(conn, cookie, nullptr);
    if (!reply)
        return 0;
    auto ret = reply->atom;
    free(reply);
    return ret;
}

enum class ScreensaverMethod {
    Auto, Gnome, Freedesktop, Xss
};

struct X11 : public QObject {
    X11();
    ~X11();

    struct {
        QTimer reset;
        QDBusInterface *iface = nullptr;
        QDBusReply<uint> reply;
        bool inhibit = false;
        ScreensaverMethod method = ScreensaverMethod::Auto;
    } ss;

    xcb_connection_t *connection = nullptr;
    xcb_window_t root = 0;
    Display *display = nullptr;
    xcb_atom_t atoms[XcbAtomEnd];
    HwAccX11 *api = nullptr;

    int statm = 0;

    template<class... Args>
    auto send(WId wid, XcbAtom type, Args... args) -> void
    {
        uint32_t data32[5] = { static_cast<uint32_t>(args)... };
        xcb_client_message_event_t event;
        memset(&event, 0, sizeof(event));
        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = 32;
        event.window = wid;
        event.type = atoms[type];
        memcpy(event.data.data32, data32, sizeof(data32));
        const auto mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
        xcb_send_event(connection, 0, root, mask, (const char *)&event);
        xcb_flush(connection);
    }

    auto sendState(WId wid, bool on, XcbAtom a1, XcbAtom a2 = XcbAtomEnd) -> void
    {
        xcb_client_message_event_t event;
        memset(&event, 0, sizeof(event));
        event.response_type = XCB_CLIENT_MESSAGE;
        event.format = 32;
        event.window = wid;
        event.type = atoms[_NET_WM_STATE];
        event.data.data32[0] = on ? 1 : 0;
        event.data.data32[1] = atoms[a1];
        if (a2 != XcbAtomEnd)
            event.data.data32[2] = atoms[a2];
        const auto mask = XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY
                | XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
        xcb_send_event(connection, 0, root, mask, (const char *)&event);
        xcb_flush(connection);
    }
};

static X11 *d = nullptr;

auto initialize() -> void { _Renew(d); }
auto finalize() -> void { _Delete(d); }

X11::X11()
{
    connection = QX11Info::connection();
    display = QX11Info::display();
    root = QX11Info::appRootWindow();

    Q_ASSERT(connection);

#define GET_ATOM(id) {atoms[id] = getAtom(connection, #id);}
    GET_ATOM(_NET_WM_STATE);
    GET_ATOM(_NET_WM_STATE_MODAL);
    GET_ATOM(_NET_WM_STATE_STICKY);
    GET_ATOM(_NET_WM_STATE_MAXIMIZED_VERT);
    GET_ATOM(_NET_WM_STATE_MAXIMIZED_HORZ);
    GET_ATOM(_NET_WM_STATE_SHADED);
    GET_ATOM(_NET_WM_STATE_SKIP_TASKBAR);
    GET_ATOM(_NET_WM_STATE_SKIP_PAGER);
    GET_ATOM(_NET_WM_STATE_HIDDEN);
    GET_ATOM(_NET_WM_STATE_FULLSCREEN);
    GET_ATOM(_NET_WM_STATE_ABOVE);
    GET_ATOM(_NET_WM_STATE_BELOW);
    GET_ATOM(_NET_WM_STATE_DEMANDS_ATTENTION);
    GET_ATOM(_NET_WM_STATE_STAYS_ON_TOP);
    GET_ATOM(_NET_WM_MOVERESIZE);
#undef GET_ATOM

    ss.reset.setInterval(20000);
    connect(&ss.reset, &QTimer::timeout, this, [=] () {
        Q_ASSERT(ss.method == ScreensaverMethod::Xss);
        _Debug("Force XCB_SCREEN_SAVER_RESET.");
        xcb_force_screen_saver(connection, XCB_SCREEN_SAVER_RESET);
        xcb_flush(connection);
    });

#if HAVE_VAAPI && HAVE_VDPAU
    api = new VaApiInfo;
    if (!api->isNative()) {
        delete api;
        api = new VdpauInfo;
        if (!api->isNative())
            _Delete(api);
    }
#elif HAVE_VAAPI
    api = new VaApiInfo;
#elif HAVE_VDPAU
    api = new VdpauInfo;
#endif
    if (api && !api->isOk()) {
        _Info("Failed to initialize hardware acceration API.");
        _Delete(api);
    }
    if (api)
        _Info("Initialized hardware acceleration API: %%.", api->name());
    else
        _Info("No available hardware acceleration API.");

    statm = ::open("/proc/self/statm", O_RDONLY);
}

X11::~X11()
{
    delete api;
    delete ss.iface;
    ::close(statm);
}

auto getHwAcc() -> HwAcc* { return d->api; }

template<class T>
static inline QSharedPointer<T> _Reply(T *t) { return QSharedPointer<T>(t, free); }

auto refreshRate() -> qreal
{
    auto sr = _Reply(xcb_randr_get_screen_resources_current_reply(
        d->connection,
        xcb_randr_get_screen_resources_current_unchecked(d->connection, d->root),
        nullptr));
    const auto len = xcb_randr_get_screen_resources_current_crtcs_length(sr.data());
    const int n = qApp->desktop()->screenNumber(qApp->topLevelWidgets().value(0));
    if (len < 1 || n >= len)
        return -1;
    auto crtc = xcb_randr_get_screen_resources_current_crtcs(sr.data())[n];
    auto ci = _Reply(xcb_randr_get_crtc_info_reply(
        d->connection,
        xcb_randr_get_crtc_info_unchecked(d->connection, crtc, XCB_CURRENT_TIME),
        nullptr));

    auto mode_refresh = [] (xcb_randr_mode_info_t *mode_info) -> double
    {
        double vtotal = mode_info->vtotal;
        if (mode_info->mode_flags & XCB_RANDR_MODE_FLAG_DOUBLE_SCAN)
            vtotal *= 2;
        if (mode_info->mode_flags & XCB_RANDR_MODE_FLAG_INTERLACE)
            vtotal /= 2;
        if (mode_info->htotal && vtotal)
            return ((double) mode_info->dot_clock / ((double) mode_info->htotal * (double) vtotal));
        return -1;
    };

    auto it = xcb_randr_get_screen_resources_current_modes_iterator(sr.data());
    while (it.rem) {
        if (it.data->id == ci->mode)
            return mode_refresh(it.data);
        xcb_randr_mode_info_next(&it);
    }
    return -1;
}

static auto methodName(ScreensaverMethod method) -> QString
{
    switch (method) {
    case ScreensaverMethod::Gnome:
        return u"org.gnome.SessionManager.Inhibit"_q;
    case ScreensaverMethod::Freedesktop:
        return u"org.freedesktop.ScreenSaver.Inhibit"_q;
    case ScreensaverMethod::Xss:
        return u"XScreenSaver"_q;
    default:
        return u"auto"_q;
    }
}

static auto methodFromName(const QString &name) -> ScreensaverMethod
{
    if (name == "org.gnome.SessionManager.Inhibit"_a)
        return ScreensaverMethod::Gnome;
    else if (name == "org.freedesktop.ScreenSaver.Inhibit"_a)
        return ScreensaverMethod::Freedesktop;
    else if (name == "XScreenSaver"_a)
        return ScreensaverMethod::Xss;
    return ScreensaverMethod::Auto;
}

auto setScreensaverMethod(const QString &name) -> void
{
    bool was = d->ss.inhibit;
    if (was)
        setScreensaverEnabled(true);
    if (_Change(d->ss.method, methodFromName(name))) {
        _Delete(d->ss.iface);
        d->ss.reset.stop();
    }
    if (was)
        setScreensaverEnabled(false);
}

auto screensaverMethods() -> QStringList
{
    return {
        u"auto"_q,
        u"org.gnome.SessionManager.Inhibit"_q,
        u"org.freedesktop.ScreenSaver.Inhibit"_q,
        u"XScreenSaver"_q
    };
}

auto setScreensaverEnabled(bool enabled) -> void
{
    const auto disabled = !enabled;
    auto &s = d->ss;
    if (s.inhibit == disabled)
        return;

    s.reset.stop();

    if (!s.iface && s.method != ScreensaverMethod::Xss) {
        auto getGnome = [&] () {
            _Renew(s.iface, u"org.gnome.SessionManager"_q,
                 u"/org/gnome/SessionManager"_q, u"org.gnome.SessionManager"_q);
            return s.iface;
        };
        auto getFreedesktop = [&] () {
            _Renew(s.iface, u"org.freedesktop.ScreenSaver"_q,
                   u"/ScreenSaver"_q, u"org.freedesktop.ScreenSaver"_q);
            return s.iface;
        };
        const auto dbus = [&] () {
            if (s.method == ScreensaverMethod::Gnome)
                return getGnome()->isValid();
            if (s.method == ScreensaverMethod::Freedesktop)
                return getFreedesktop()->isValid();
            if (s.method == ScreensaverMethod::Xss)
                return false;
            if (getGnome()->isValid()) {
                s.method = ScreensaverMethod::Gnome;
                return true;
            }
            if (getFreedesktop()->isValid()) {
                s.method = ScreensaverMethod::Freedesktop;
                return true;
            }
            return false;
        }();
        if (!dbus) {
            _Delete(s.iface);
            s.method = ScreensaverMethod::Xss;
        }
        _Info("Selected screensaver method: %%", methodName(s.method));
    }
    Q_ASSERT(s.method != ScreensaverMethod::Auto);

    if (disabled) {
        if (s.iface) {
            if (s.method == ScreensaverMethod::Gnome)
                s.reply = s.iface->call(u"Inhibit"_q, u"bomi"_q, 0u,
                                        u"Running player"_q, 4u | 8u);
            else
                s.reply = s.iface->call(u"Inhibit"_q, u"bomi"_q,
                                        u"Running player"_q);
            if (!s.reply.isValid()) {
                _Error("DBus '%%' error: %%", s.iface->interface(),
                       s.reply.error().message());
                _Error("Fallback to XCB_SCREEN_SAVER_RESET.");
                _Delete(s.iface);
                s.method = ScreensaverMethod::Xss;
            } else
                _Debug("Disable screensaver with '%%'.", s.iface->interface());
        }
        if (s.method == ScreensaverMethod::Xss) {
            xcb_screensaver_suspend(d->connection, 1);
            _Debug("Disable screensaver with XScreenSaver.");
            s.reset.start();
        }
    } else {
        if (s.iface) {
            const auto func = s.method == ScreensaverMethod::Gnome ? u"Uninhibit"_q : u"UnInhibit"_q;
            auto res = s.iface->call(func, s.reply.value());
            if (res.type() == QDBusMessage::ErrorMessage)
                _Error("DBus '%%' error: [%%] %%", s.iface->interface(),
                       res.errorName(), res.errorMessage());
            else
                _Debug("Enable screensaver with '%%'.", s.iface->interface());
        }
        if (s.method == ScreensaverMethod::Xss) {
            xcb_screensaver_suspend(d->connection, 0);
            _Debug("Enable screensaver with XScreenSaver.");
        }
    }
    s.inhibit = disabled;
}

X11WindowAdapter::X11WindowAdapter(QWindow* w)
    : WindowAdapter(w)
{
    connect(&m_timer, &QTimer::timeout, this, [=] () {
        auto cookie = xcb_query_pointer_unchecked(d->connection, winId());
        auto ptr = xcb_query_pointer_reply(d->connection, cookie, nullptr);
        const auto pressed = ptr->mask & XCB_BUTTON_MASK_1;
        free(ptr);
        if (!pressed)
            stopDrag();
    });
    m_timer.setInterval(10);
}

auto X11WindowAdapter::setFullScreen(bool fs) -> void
{
    if (isFullScreen() != fs)
        d->sendState(winId(), fs, _NET_WM_STATE_FULLSCREEN);
}

auto X11WindowAdapter::stopDrag() -> void
{
    m_timer.stop();
    if (!isMovingByDrag())
        return;

    // hack to get back focus
    auto pos = QCursor::pos();
    d->send(winId(), _NET_WM_MOVERESIZE, pos.x(), pos.y(), 11, 1, 0);
    auto reset = [&] () {
        QCursor::setPos(pos + QPoint(10, 0));
        QCursor::setPos(pos - QPoint(10, 0));
        QCursor::setPos(pos);
    };
    xcb_test_fake_input(d->connection, XCB_BUTTON_PRESS, XCB_BUTTON_INDEX_1,
                        XCB_WINDOW_NONE, d->root, 0, 0, 0);
    reset();
    xcb_test_fake_input(d->connection, XCB_BUTTON_RELEASE, XCB_BUTTON_INDEX_1,
                        XCB_WINDOW_NONE, d->root, 0, 0, 0);
    xcb_flush(d->connection);
    reset();
}

auto X11WindowAdapter::startMoveByDrag(const QPointF &m) -> void
{
    WindowAdapter::startMoveByDrag(m);
    stopDrag();
}

auto X11WindowAdapter::moveByDrag(const QPointF &m) -> void
{
    if (isMovingByDrag())
        return;
    xcb_ungrab_pointer(d->connection, XCB_TIME_CURRENT_TIME);
    d->send(winId(), _NET_WM_MOVERESIZE,
            m.x(), m.y(),
            8, // _NET_WM_MOVERESIZE_MOVE
            1, // button 1
            0); // source indication: normal
    setMovingByDrag(true);
    m_timer.start();
}

auto X11WindowAdapter::endMoveByDrag() -> void
{
    stopDrag();
    WindowAdapter::endMoveByDrag();
}

auto X11WindowAdapter::isAlwaysOnTop() const -> bool
{
    const auto cookie = xcb_get_property_unchecked
        (d->connection, 0, winId(), d->atoms[_NET_WM_STATE], XCB_ATOM_ATOM, 0, 1024);
    auto reply = _Reply(xcb_get_property_reply(d->connection, cookie, nullptr));
    if (!reply || reply->format != 32 || reply->type != XCB_ATOM_ATOM)
        return false;
    auto begin = static_cast<const xcb_atom_t *>(xcb_get_property_value(reply.data()));
    auto end = begin + reply->length;
    if (std::find(begin, end, d->atoms[_NET_WM_STATE_STAYS_ON_TOP]) != end)
        return true;
    if (std::find(begin, end, d->atoms[_NET_WM_STATE_ABOVE]) != end)
        return true;
    return false;
}

auto X11WindowAdapter::setAlwaysOnTop(bool on) -> void
{
    d->sendState(winId(), on, _NET_WM_STATE_ABOVE, _NET_WM_STATE_STAYS_ON_TOP);
}

auto createAdapter(QWindow *w) -> WindowAdapter*
{
    return new X11WindowAdapter(w);
}

auto opticalDrives() -> QStringList
{
    static const QStringList filter = QStringList() << u"sr*"_q
        << u"sg*"_q << u"scd*"_q << u"dvd*"_q << u"cd*"_q;
    QDir dir(u"/dev"_q);
    QStringList devices;
    for (auto &dev : dir.entryList(filter, QDir::System))
        devices.append(u"/dev/"_q % dev);
    return devices;
}

auto canShutdown() -> bool
{
    // Can I check a dbus call is actually callable without calling it?
    return true;
}

auto shutdown() -> bool
{
    using Iface = QDBusInterface;
    auto bus = QDBusConnection::sessionBus();
    _Debug("Try KDE session manger to shutdown.");
    Iface kde(u"org.kde.ksmserver"_q, u"/KSMServer"_q,
              u"org.kde.KSMServerInterface"_q, bus);
    auto response = kde.call(u"logout"_q, 0, 2, 2);
    auto check = [&] (const char *fmt, const char *fb) -> bool {
        if (response.type() != QDBusMessage::ErrorMessage)
            return true;
        _Debug(fmt, response.errorName(), response.errorMessage());
        _Debug(fb);
        return false;
    };
    if (check("KDE session manager does not work: [%%] %%",
              "Fallback to Gnome session manager."))
        return true;
    Iface gnome(u"org.gnome.SessionManager"_q,
                u"/org/gnome/SessionManager"_q,
                u"org.gnome.SessionManager"_q, bus);
    response = gnome.call(u"RequestShutdown"_q);
    if (check("Gnome session manager does not work: [%%] %%",
              "Fallback to gnome-power-cmd.sh."))
        return true;
    if (QProcess::startDetached(u"gnome-power-cmd.sh shutdown"_q)
            || QProcess::startDetached(u"gnome-power-cmd shutdown"_q))
        return true;
    bus = QDBusConnection::systemBus();
    _Debug("gnome-power-cmd.sh does not work.");
    _Debug("Fallback to HAL.");
    Iface hal(u"org.freedesktop.Hal"_q,
              u"/org/freedesktop/Hal/devices/computer"_q,
              u"org.freedesktop.Hal.Device.SystemPowerManagement"_q, bus);
    response = hal.call(u"Shutdown"_q);
    if (check("HAL does not work: [%%] %%", "Fallback to ConsoleKit."))
        return true;
    Iface consoleKit(u"org.freedesktop.ConsoleKit"_q,
                     u"/org/freedesktop/ConsoleKit/Manager"_q,
                     u"org.freedesktop.ConsoleKit.Manager"_q, bus);
    response = consoleKit.call(u"Stop"_q);
    if (check("ConsoleKit does not work: [%%] %%",
              "Sorry, there's no way to shutdown."))
        return true;
    return false;
}

auto systemTime() -> quint64
{
    struct timeval t;
    gettimeofday(&t, 0);
    return t.tv_sec*1000000llu + t.tv_usec;
}

auto processTime() -> quint64
{
    timespec ts;
    if (clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts ) == -1)
        return 0;
    return ts.tv_sec*1000000llu + ts.tv_nsec / 1000;
}

auto totalMemory() -> double
{
    return (sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE)) / double(1024 * 1024);
}

auto usingMemory() -> double
{
    if (!d->statm)
        return 0;
    // this is not thread safe!!
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    static char buffer[BUFSIZ];
    ::lseek(d->statm, 0, SEEK_SET);
    int len = ::read(d->statm, buffer, BUFSIZ);
    if (len <= 0)
        return 0;
    int size = 0, resident = 0;
    sscanf(buffer, "%d %d", &size, &resident);
    return resident * sysconf(_SC_PAGESIZE) / double(1024*1024);
}

/******************************************************************************/

struct HwAccCodec {
    HwAccCodec(CodecId id = CodecId::Invalid,
               const QVector<uint32_t> &profiles = QVector<uint32_t>())
        : id(id), profiles(profiles) { }
    CodecId id;
    QVector<uint32_t> profiles;
};

#define HA_CODEC(c, ...) { CodecId::c, __VA_ARGS__ }
#if HAVE_VAAPI
#define VA(v) (VAProfile##v)
static const HwAccCodec s_vaCodecs[] = {
//    VA_CODEC(Mpeg1, {}),
    HA_CODEC(Mpeg2, {VA(MPEG2Simple), VA(MPEG2Main)})
  , HA_CODEC(Mpeg4, {VA(MPEG4AdvancedSimple), VA(MPEG4Main), VA(MPEG4Simple)})
  , HA_CODEC(H264,  {VA(H264Baseline), VA(H264High), VA(H264Main)})
  , HA_CODEC(Vc1,   {VA(VC1Advanced), VA(VC1Main), VA(VC1Simple)}) // same as wmv3
  , HA_CODEC(Wmv3,  {VA(VC1Advanced), VA(VC1Main), VA(VC1Simple)})
#if VA_CHECK_VERSION(0, 37, 0)
  , HA_CODEC(Hevc,  {VA(HEVCMain), VA(HEVCMain10)})
#endif
};
#undef VA
#endif

#if HAVE_VDPAU
#define VDP(v) (VDP_DECODER_PROFILE_##v)
static const HwAccCodec s_vdpCodecs[] = {
    HA_CODEC(Mpeg1, {VDP(MPEG1)})
    , HA_CODEC(Mpeg2, {VDP(MPEG2_SIMPLE), VDP(MPEG2_MAIN)})
    , HA_CODEC(Mpeg4, {VDP(MPEG4_PART2_ASP), VDP(MPEG4_PART2_SP)})
    , HA_CODEC(H264,  {VDP(H264_BASELINE), VDP(H264_MAIN), VDP(H264_HIGH)
#ifdef VDP_DECODER_PROFILE_H264_HIGH_444_PREDICTIVE
                     , VDP(H264_EXTENDED), VDP(H264_HIGH_444_PREDICTIVE)
                     , VDP(H264_PROGRESSIVE_HIGH), VDP(H264_CONSTRAINED_HIGH)
#endif
    })
    , HA_CODEC(Vc1,   {VDP(VC1_ADVANCED), VDP(VC1_MAIN), VDP(VC1_SIMPLE)})
    , HA_CODEC(Wmv3,  {VDP(VC1_ADVANCED), VDP(VC1_MAIN), VDP(VC1_SIMPLE)})
#ifdef VDP_DECODER_PROFILE_HEVC_MAIN_444
    , HA_CODEC(Hevc,  {VDP(HEVC_MAIN), VDP(HEVC_MAIN_10), VDP(HEVC_MAIN_STILL)
                     , VDP(HEVC_MAIN_12), VDP(HEVC_MAIN_444)})
#endif
};
#undef VDP
#endif
#undef HA_CODEC

#if HAVE_VAAPI

VaApiInfo::VaApiInfo(): HwAccX11(VaApiGLX)
{
    setLogContext("VAAPI");
    setOkStatus(VA_STATUS_SUCCESS);
    setGetErrorStringFunction([] (qint64 s) { return vaErrorStr(s); });
    const auto xdpy = QX11Info::display();
    auto display = vaGetDisplayGLX(xdpy);
    if (!check(display ? VA_STATUS_SUCCESS : VA_STATUS_ERROR_UNIMPLEMENTED,
               "Cannot create VADisplay."))
        return;
    int major, minor;
    if (!check(vaInitialize(display, &major, &minor),
               "Cannot initialize VA-API."))
        return;
    do {
        const auto vendor = QString::fromLatin1(vaQueryVendorString(display));
        if (!vendor.contains("VDPAU"_a))
            setNative(true);
#if HAVE_VDPAU
        if (!isNative())
            break;
#endif
        auto size = vaMaxNumProfiles(display);
        QVector<VAProfile> profiles(size);
        if (!check(vaQueryConfigProfiles(display, profiles.data(), &size),
                   "No available profiles."))
            break;
        profiles.resize(size);
        QList<CodecId> codecs;
        for (auto profile : profiles) {
            int size = vaMaxNumEntrypoints(display);
            QVector<VAEntrypoint> entries(size);
            if (!isOk(vaQueryConfigEntrypoints(display, profile,
                                                    entries.data(), &size)))
                continue;
            entries.resize(size);
            if (!entries.contains(VAEntrypointVLD))
                continue;
            for (auto &codec : s_vaCodecs) {
                if (codec.profiles.contains(profile)) {
                    if (!codecs.contains(codec.id))
                        codecs.push_back(codec.id);
                    break;
                }
            }
        }
        isOk(VA_STATUS_SUCCESS);
        if (codecs.contains(CodecId::Vc1))
            codecs.push_back(CodecId::Wmv3);
        setSupportedCodecs(codecs);
    } while (false);
    vaTerminate(display);
}

auto VaApiInfo::download(mp_hwdec_ctx *ctx, const mp_image *mpi,
                         mp_image_pool *pool) -> mp_image*
{
    if (!ctx->vaapi_ctx)
        return nullptr;
    auto img = va_surface_download((mp_image*)mpi, pool);
    if (!img)
        return nullptr;
    mp_image_copy_attributes(img, (mp_image*)mpi);
    return img;
}

#endif

/******************************************************************************/

#if HAVE_VDPAU

VdpauInfo::VdpauInfo()
    : HwAccX11(VdpauX11)
{
    setLogContext("VDPAU");
    setOkStatus(VDP_STATUS_OK);
    setGetErrorStringFunction([this] (qint64 s) {
        if (m_errors.isEmpty() || s < 0 || s >= m_errors.size())
            return "Unknown error code";
        return m_errors[s].constData();
    });
    if (!check(vdp_device_create_x11(QX11Info::display(), QX11Info::appScreen(),
                                     &m_device, &m_proc), "Cannot intialize VDPAU device"))
        return;
    VdpGetErrorString *getErrorString = nullptr;
    VdpDeviceDestroy *deviceDestroy = nullptr;
    VdpDecoderQueryCapabilities *decoderQueryCaps = nullptr;
    VdpGetInformationString *getInformationString = nullptr;
#define PROC(id, f) proc(VDP_FUNC_ID_##id, f)
    PROC(GET_ERROR_STRING,                 getErrorString);
    PROC(DEVICE_DESTROY,                   deviceDestroy);
    PROC(DECODER_QUERY_CAPABILITIES,       decoderQueryCaps);
    PROC(GET_INFORMATION_STRING,           getInformationString);
#undef PROC
    if (getErrorString) {
        m_errors.resize(VDP_STATUS_ERROR);
        for (int i = 0; i < m_errors.size(); ++i)
            m_errors[i] = getErrorString(static_cast<VdpStatus>(i));
    }
    do {
        if (!check(status(), "Cannot get VDPAU functions."))
            break;
        char const *info = nullptr;
        if (!check(getInformationString(&info), "Cannot get VDPAU information."))
            break;
        if (!QString::fromLatin1(info).contains("VAAPI"_a))
            setNative(true);
#if HAVE_VAAPI
        if (!isNative())
            break;
#endif
        auto supports = [=] (VdpDecoderProfile id) -> bool
        {
            VdpBool supported = VDP_FALSE;
            quint32 lv = 0, blocks = 0, w = 0, h = 0;
            return decoderQueryCaps(m_device, id, &supported, &lv, &blocks,
                                    &w, &h) == VDP_STATUS_OK && supported == VDP_TRUE;
        };
        QList<CodecId> codecs;
        for (auto &codec : s_vdpCodecs) {
            for (auto profile : codec.profiles) {
                if (supports(profile)) {
                    codecs.push_back(codec.id);
                    break;
                }
            }
        }
        isOk(VDP_STATUS_OK);
        setSupportedCodecs(codecs);
    } while (false);
    if (deviceDestroy)
        deviceDestroy(m_device);
    m_device = 0;
}

auto VdpauInfo::download(mp_hwdec_ctx *ctx, const mp_image *mpi,
                         mp_image_pool *pool) -> mp_image*
{
    if (!ctx->vdpau_ctx)
        return nullptr;
    auto img = mp_image_pool_get(pool, IMGFMT_420P, mpi->w, mpi->h);
    mp_image_copy_attributes(img, (mp_image*)mpi);
    const VdpVideoSurface surface = (intptr_t)mpi->planes[3];
    if (ctx->vdpau_ctx->vdp.video_surface_get_bits_y_cb_cr(
                surface, VDP_YCBCR_FORMAT_YV12, (void* const*)img->planes,
                (uint32_t*)img->stride) == VDP_STATUS_OK)
        return img;
    talloc_free(img);
    return nullptr;
}

#endif

} // namespace OS

#endif
