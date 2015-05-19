#include "mpv.hpp"
#include "video/mpvosdrenderer.hpp"
#include <QOpenGLContext>
#include <QLibrary>

struct PropertyObservation {
    int event;
    const char *name = nullptr;
    std::function<void(int)> notify = nullptr;      // post from mpv to qt
    std::function<void(QEvent*)> process = nullptr; // handle posted event
};

static constexpr const int UpdateEventBegin = QEvent::User + 10000;

auto Mpv::e2l(int error) -> Log::Level
{
    if (error >= 0)
        return Log::Off;
    switch (error) {
    case MPV_ERROR_NOMEM:
    case MPV_ERROR_UNINITIALIZED:
        return Log::Fatal;
    case MPV_ERROR_PROPERTY_UNAVAILABLE:
        return Log::Trace;
    default:
        return Log::Error;
    }
}

struct Mpv::Data {
    Mpv *p = nullptr;
    mpv_opengl_cb_context *gl = nullptr;
    MpvOsdRenderer osd;
    bool quit = false;
    QVector<PropertyObservation> observations;
    QVector<std::function<void(mpv_event*)>> events;
    QMap<QByteArray, std::function<void(void)>> hooks;
    int updateEventMax = ::UpdateEventBegin;
    int hookId = 0;
    std::function<void(void)> update;
    auto observation(int event) -> const PropertyObservation&
    {
        Q_ASSERT(UpdateEventBegin <= event && event < updateEventMax);
        Q_ASSERT(event == observations[event - UpdateEventBegin].event);
        return observations[event - UpdateEventBegin];
    }
    auto reset()
    {
        quit = false;
        observations.clear();
        events.clear();
        hooks.clear();
        updateEventMax = ::UpdateEventBegin;
        hookId = 0;
    }
};

Mpv::Mpv(QObject *parent)
    : QThread(parent), d(new Data)
{
    d->p = this;
}

Mpv::~Mpv()
{
    delete d;
}

auto Mpv::create() -> void
{
    if (!m_handle) {
        m_handle = mpv_create();
        setOption("config", "no");
        setOption("fs", "no");
        setOption("quiet", "yes");
        setOption("input-terminal", "no");
    }
}

auto Mpv::initialize(Log::Level lv, bool ogl) -> void
{
    Q_ASSERT(m_handle && !d->gl);

    QByteArray loglv = "no";
    switch (lv) {
    case Log::Trace: loglv = "trace"; break;
    case Log::Debug: loglv = "v";     break;
    case Log::Info:  loglv = "info";  break;
    case Log::Warn:  loglv = "warn";  break;
    case Log::Error: loglv = "error"; break;
    case Log::Fatal: loglv = "fatal"; break;
    case Log::Off:   loglv = "no";    break;
    }
    mpv_request_log_messages(m_handle, loglv.constData());

    fatal(mpv_initialize(m_handle), "Couldn't initialize mpv.");
    if (ogl) {
        auto ptr = mpv_get_sub_api(m_handle, MPV_SUB_API_OPENGL_CB);
        d->gl = static_cast<mpv_opengl_cb_context*>(ptr);
    }
}

auto Mpv::destroy() -> void
{
    if (m_handle) {
        mpv_terminate_destroy(m_handle);
        m_handle = nullptr;
        d->gl = nullptr;
        d->reset();
    }
}

auto Mpv::update() -> void
{
    if (d->update)
        d->update();
}

auto Mpv::setUpdateCallback(std::function<void ()> &&cb) -> void
{
    Q_ASSERT(cb);
    auto update = [] (void *p) -> void { static_cast<Data*>(p)->update(); };
    d->update = std::move(cb);
    mpv_opengl_cb_set_update_callback(d->gl, update, d);
}

auto Mpv::render(OpenGLFramebufferObject *frame, OpenGLFramebufferObject *osd, const QMargins &m) -> int
{
    int ret = 0;
    if (frame) {
        ret = mpv_opengl_cb_draw(d->gl, frame->id(), frame->width(), frame->height());
    }
    if (osd) {
        d->osd.prepare(osd);
        mpv_opengl_cb_render_osd(d->gl, osd->width(), osd->height(),
                                 m.left(), m.top(), m.right(), m.bottom(),
                                 1.0, MpvOsdRenderer::callback, &d->osd);
        d->osd.end();
    }
    return ret;
}

auto Mpv::frameSwapped() -> void
{
    mpv_opengl_cb_report_flip(d->gl, 0);
}

auto Mpv::initializeGL(QOpenGLContext *ctx) -> void
{
    auto getProcAddr = [] (void *ctx, const char *name) -> void* {
        auto gl = static_cast<QOpenGLContext*>(ctx);
        if (!gl)
            return nullptr;
        auto res = gl->getProcAddress(QByteArray(name));
#ifdef Q_OS_WIN
        if (!res)
            res = QLibrary::resolve(u"opengl32.dll"_q, name);
#endif
        return reinterpret_cast<void*>(res);
    };
    auto err = mpv_opengl_cb_init_gl(d->gl, nullptr, getProcAddr, ctx);
    Q_UNUSED(err); Q_ASSERT(err >= 0);
    d->osd.initialize();
}

auto Mpv::finalizeGL() -> void
{
    d->osd.finalize();
    mpv_opengl_cb_uninit_gl(d->gl);
}

auto Mpv::hook(const QByteArray &when, std::function<void ()> &&run) -> void
{
    Q_ASSERT(!d->hooks.contains(when));
    tell("hook_add", when, d->hookId++, 0);
    d->hooks[when] = std::move(run);
}

auto Mpv::request(mpv_event_id id, std::function<void(mpv_event*)> &&proc) -> void
{
    if (d->events.size() < id + 1)
        d->events.resize(id + 1);
    mpv_request_event(m_handle, id, true);
    d->events[id] = std::move(proc);
}

auto Mpv::newObservation(const char *name, std::function<void(int)> &&notify,
                  std::function<void(QEvent*)> &&process) -> int
{
    const int event = d->updateEventMax++;
    PropertyObservation ob;
    ob.event = event;
    ob.name = name;
    ob.notify = std::move(notify);
    ob.process = std::move(process);
    d->observations.append(ob);
    Q_ASSERT(d->observations.size() == d->updateEventMax - UpdateEventBegin);
    mpv_observe_property(m_handle, ob.event, ob.name, MPV_FORMAT_NONE);
    return event;
}

auto Mpv::setOption(const char *name, const char *data) -> void
{
    const auto err = mpv_set_option_string(m_handle, name, data);
    fatal(err, "Couldn't set option %%=%%.", name, data);
}

auto Mpv::get_osd(const char *name) const -> QString
{
    auto buf = mpv_get_property_osd_string(m_handle, name);
    auto ret = QString::fromLatin1(buf); mpv_free(buf); return ret;
}

auto Mpv::run() -> void
{
    _Debug("Start playloop thread");
    d->quit = false;
    while (!d->quit) {
        auto ev = mpv_wait_event(m_handle, 0.005);
        switch (ev->event_id) {
        case MPV_EVENT_NONE:
            break;
        case MPV_EVENT_PROPERTY_CHANGE: {
            auto &o = d->observation(ev->reply_userdata);
            o.notify(o.event);
            break;
        } case MPV_EVENT_LOG_MESSAGE: {
            auto msg = static_cast<mpv_event_log_message*>(ev->data);
            if (msg->log_level == MPV_LOG_LEVEL_NONE)
                break;
            auto getLevel = [&]() {
                switch (msg->log_level) {
                case MPV_LOG_LEVEL_TRACE: return Log::Trace;
                case MPV_LOG_LEVEL_V:
                case MPV_LOG_LEVEL_DEBUG: return Log::Debug;
                case MPV_LOG_LEVEL_INFO:  return Log::Info;
                case MPV_LOG_LEVEL_WARN:  return Log::Warn;
                default:                  return Log::Error;
                }
            };
            const auto lv = getLevel();
            Log::print(lv, Log::parse(lv, m_logContext + '/' + msg->prefix, msg->text));
            break;
        } case MPV_EVENT_CLIENT_MESSAGE: {
            auto message = static_cast<mpv_event_client_message*>(ev->data);
            if (message->num_args < 1)
                break;
            if (!qstrcmp(message->args[0], "hook_run") && message->num_args == 3) {
                QByteArray when(message->args[2]);
                Q_ASSERT(d->hooks.contains(when));
                d->hooks[when]();
                tell("hook_ack", when);
            }
            break;
        } case MPV_EVENT_SET_PROPERTY_REPLY: {
            QScopedPointer<QByteArray> name(reinterpret_cast<QByteArray*>(ev->reply_userdata));
            if (!isSuccess(ev->error)) {
                _Debug("Error %%: Couldn't set property %%.",
                       mpv_error_string(ev->error), *name);
            }
            break;
        } case MPV_EVENT_COMMAND_REPLY: {
            QScopedPointer<QByteArray> name(reinterpret_cast<QByteArray*>(ev->reply_userdata));
            if (!isSuccess(ev->error)) {
                _Debug("Error %%: Couldn't execute command %%.",
                       mpv_error_string(ev->error), *name);
            }
            break;
        } case MPV_EVENT_GET_PROPERTY_REPLY: {
            auto event = static_cast<mpv_event_property*>(ev->data);
            _Error("Never requested reply: %%", event->name);
            break;
        } case MPV_EVENT_SHUTDOWN:
            d->quit = true;
            break;
        default: {
            if (ev->event_id >= d->events.size())
                break;
            if (auto &proc = d->events[ev->event_id])
                proc(ev);
        }}
    }
    _Debug("Finish playloop thread");
}

auto Mpv::process(QEvent *event) -> bool
{
    const int type = event->type();
    if (UpdateEventBegin <= type && type < d->updateEventMax) {
        d->observation(type).process(event);
        return true;
    }
    return false;
}
