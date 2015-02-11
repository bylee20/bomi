#ifndef MPV_HPP
#define MPV_HPP

#include "mpv_property.hpp"
#include "misc/log.hpp"
#include "misc/dataevent.hpp"
#include <libmpv/client.h>
#include <libmpv/opengl_cb.h>
#include <functional>

SCIA s2ms(double s) -> int { return s*1000 + 0.5; }

#define CHECK_MPV(err, fmt, ...) \
    (isSuccess(err) ? true : (_WriteLog(e2l(err), "Failed to " fmt ": %%", __VA_ARGS__, e2s(err)), false))

struct PropertyObservation {
    int event;
    const char *name = nullptr;
    std::function<void(int)> notify = nullptr;      // post from mpv to qt
    std::function<void(QEvent*)> process = nullptr; // handle posted event
};

class Mpv : public QThread {
    Q_OBJECT
    template<class R, class...Args> using func = std::function<R(Args...)>;
    template<class T> using trait = mpv_trait<T>;
    template<class T> using type  = typename trait<T>::mpv_type;
public:
    Mpv(QObject *parent = nullptr);
    ~Mpv();

    SCIA getLogContext() -> const char* { return "mpv"; }

    auto handle() const -> mpv_handle* { return m_handle; }

    auto create() -> void;
    auto initialize() -> void;
    auto destroy() -> void;
    auto process(QEvent *event) -> bool;

    template<class... Args>
    auto fatal(int err, const char *msg, const Args &... args) const -> void;

    template<class T>
    auto get(const char *name, T &def) const -> bool;
    template<class T>
    auto get(const char *name) const -> T { T t = T(); get<T>(name, t); return t; }
    auto get_osd(const char *name) const -> QString;

    auto setOption(const char *name, const char *data) -> void;

    template <class T>
    auto set(const char *name, const T &value) -> bool;
    template<class T>
    auto setAsync(QByteArray &&name, const T &value) -> bool;
    template<class T, int N>
    auto setAsync(const char (&name)[N], const T &value) -> bool;

    template<class... Args>
    auto tell(const char *name, const Args&... args) -> bool;
    template<int N, class... Args>
    auto tellAsync(const char (&name)[N], const Args&... args) -> bool;
    template<class... Args>
    auto tellAsync(QByteArray &&name, const Args&... args) -> bool;
    auto flush() { mpv_wait_async_requests(m_handle); }

    auto setObserver(QObject *observer) -> void { m_observer = observer; }
    template<class Get, class Set>
    auto observe(const char *name, Get get, Set set) -> tmp::enable_if_callable_t<Get, int>;
    template<class T, class Update>
    auto observe(const char *name, T &t, Update update) -> tmp::enable_unless_callable_t<T, int>;
    template<class Update>
    auto observeTime(const char *name, int &t, Update update) -> int;
    template<class Set>
    auto observe(const char *name, Set set) -> int;
    template<class Check>
    auto observeState(const char *name, Check ck) -> int;
    auto hook(const char *name, std::function<void(void)> &&run) -> void;
    auto request(mpv_event_id id, std::function<void(mpv_event*)> &&proc) -> void;
    template<class Proc>
    auto request(mpv_event_id id, Proc proc) -> std::enable_if_t<!tmp::func_args<Proc>(), void>
        { request(id, [=] (mpv_event*) -> void { proc(); }); }
    auto setUpdateCallback(std::function<void(void)> &&cb) -> void;
    auto render(GLuint fbo, const QSize &size) -> int;
    auto initializeGL(QOpenGLContext *ctx) -> void;
    auto finalizeGL() -> void;
private:
    static auto e2s(int error) -> const char* { return mpv_error_string(error); }
    static auto e2l(int error) -> Log::Level;
    auto run() -> void override;
    auto fill(mpv_node *) { }
    template<class T, class... Args>
    auto fill(mpv_node *it, const T &t, const Args&... args)
        { mpv_trait<T>::node_fill(*it, t); fill(++it, args...); }
    static auto error(int err) -> const char* { return mpv_error_string(err); }
    static auto isSuccess(int error) -> bool { return error >=0 ; }
    template<class Func, class... Args>
    auto command(const char *name, Func &&f, const Args&... args) -> bool
    {
        std::array<mpv_node, sizeof...(args) + 1> nodes;
        mpv_node_list list = { nodes.size(), nodes.data(), nullptr };
        fill(list.values, name, args...);
        mpv_node node;
        node.format = MPV_FORMAT_NODE_ARRAY;
        node.u.list = &list;
        const int error = f(&node);
        return CHECK_MPV(error, "execute %%", name);
    }
    auto newObservation(const char *name, std::function<void(int)> &&notify,
                        std::function<void(QEvent*)> &&process) -> int;
    struct Data; Data *d;
    mpv_handle *m_handle = nullptr;
    QObject *m_observer = nullptr;
};

template<class... Args>
auto Mpv::fatal(int err, const char *msg, const Args &... args) const -> void
{
    if (!isSuccess(err))
        _Fatal("%%: %%", error(err), Log::parse(msg, args...));
}

template<class T>
auto Mpv::get(const char *name, T &def) const -> bool
{
    if (!m_handle) return false;
    MpvGetScopedData<T> data;
    int error = mpv_get_property(m_handle, name, data.format(), data.raw());
    if (!CHECK_MPV(error, "get %%", name))
        return false;
    data.get(def);
    _Trace("get %%: %%", name, def);
    return true;
}

template<class T, int N>
auto Mpv::setAsync(const char (&name)[N], const T &value) -> bool
    { return setAsync<T>(QByteArray::fromRawData(name, N), value); }

template<class T>
auto Mpv::setAsync(QByteArray &&name, const T &value) -> bool
{
    Q_ASSERT(m_handle);
    auto user = new QByteArray(std::move(name));
    MpvSetScopedData<T> data(value);
    int error = mpv_set_property_async(m_handle, (quint64)user,
                                       *user, data.format(), data.raw());
    return CHECK_MPV(error, "set_async %%", name);
}

template <class T>
auto Mpv::set(const char *name, const T &value) -> bool
{
    Q_ASSERT(m_handle);
    MpvSetScopedData<T> data(value);
    int error = mpv_set_property(m_handle, name, data.format(), data.raw());
    return CHECK_MPV(error, "set %%=%%", name, value);
}

template<class... Args>
auto Mpv::tell(const char *name, const Args&... args) -> bool
{
    return command(name, [&] (auto *node) {
        return mpv_command_node(m_handle, node, nullptr);
    }, args...);
}

template<class... Args>
auto Mpv::tellAsync(QByteArray &&name, const Args&... args) -> bool
{
    return command(name, [&] (auto *node) {
        auto user = new QByteArray(std::move(name));
        return mpv_command_node_async(m_handle, (quint64)user, node);
    }, args...);
}

template<int N, class... Args>
auto Mpv::tellAsync(const char (&name)[N], const Args&... args) -> bool
    { return tellAsync(QByteArray::fromRawData(name, N), args...); }

template<class Get, class Set>
auto Mpv::observe(const char *name, Get get, Set set) -> tmp::enable_if_callable_t<Get, int>
{
    using T = tmp::remove_cref_t<decltype(get())>;
    return newObservation(name, [=] (int e) { _PostEvent(m_observer, e, get()); },
                          [=] (QEvent *event) { set(_MoveData<T>(event)); });
}

template<class T, class Update>
auto Mpv::observe(const char *name, T &t, Update update) -> tmp::enable_unless_callable_t<T, int>
{
    return observe(name, [=] () { return get<T>(name); },
                   [=, &t] (T &&v) { if (_Change(t, v)) update(); });
}

template<class Update>
auto Mpv::observeTime(const char *name, int &t, Update update) -> int
{
    return observe(name, [=] () { return s2ms(get<double>(name)); },
                   [=, &t] (int &&v) { if (_Change(t, v)) update(); });
}

template<class Set>
auto Mpv::observe(const char *name, Set set) -> int {
    using T = tmp::remove_ref_t<tmp::func_arg_t<Set, 0>>;
    return observe(name, [=] () { return get<T>(name); }, set);
}

template<class Check>
auto Mpv::observeState(const char *name, Check ck) -> int
{
    using T = tmp::remove_ref_t<tmp::func_arg_t<Check, 0>>;
    return newObservation(name, [=] (int) { ck(get<T>(name)); }, [](QEvent*){});
}

#endif // MPV_HPP
