#include "os/os.hpp"
#include "enum/codecid.hpp"
#include "enum/deintmethod.hpp"
#include <QFontDatabase>
#include <QScreen>

namespace OS {

auto defaultFont() -> QFont
{
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont);
}

#ifndef Q_OS_WIN
auto canAssociateFileTypes() -> bool { return false; }
auto unassociateFileTypes(QWindow *, bool) -> bool { return false; }
auto associateFileTypes(QWindow *, bool, const QStringList &) -> bool { return false; }

auto defaultFixedFont() -> QFont
{
    return QFontDatabase::systemFont(QFontDatabase::FixedFont);
}
#endif

#ifndef Q_OS_LINUX
auto screensaverMethods() -> QStringList { return { u"auto"_q }; }
auto setScreensaverMethod(const QString &) -> void { }
#endif

auto getHwAcc() -> HwAcc*;

auto hwAcc() -> HwAcc*
{
    auto api = getHwAcc();
    if (!api) {
        static HwAcc none;
        api = &none;
    }
    return api;
}

auto HwAcc::fullCodecList() -> QList<CodecId>
{
    static const QList<CodecId> ids = QList<CodecId>()
        << CodecId::Mpeg1 << CodecId::Mpeg2 << CodecId::Mpeg4
        << CodecId::H264 << CodecId::Vc1 << CodecId::Wmv3 << CodecId::Hevc;
    return ids;
}

struct ApiInfo {
    HwAcc::Api api;
    QString name, desc;
};

const std::array<ApiInfo, HwAcc::NoApi> s_infos = [] () {
    std::array<ApiInfo, HwAcc::NoApi> ret;
#define SET(a, n, d) {ret[HwAcc::a] = {HwAcc::a, n, d};}
    SET(VaApiGLX, u"vaapi"_q, u"VA-API(Video Acceleration API)"_q);
    SET(VdpauX11, u"vdpau"_q, u"VDPAU(Video Decode and Presentation API for Unix)"_q );
    SET(Dxva2Copy, u"dxva2-copy"_q, u"DXVA 2.0(DirectX Video Accelaction 2.0)"_q );
#undef SET
    return ret;
}();

struct HwAcc::Data {
    Api api = NoApi;
    QList<CodecId> codecs;
    QList<DeintMethod> deints;
};

HwAcc::HwAcc(Api api)
    : d(new Data)
{
    d->api = api;
}

HwAcc::~HwAcc()
{
    delete d;
}

auto HwAcc::isAvailable() const -> bool
{
    return d->api != NoApi;
}

auto HwAcc::supports(CodecId codec) -> bool
{
    return d->codecs.contains(codec);
}

auto HwAcc::supports(DeintMethod method) -> bool
{
    return d->deints.contains(method);
}

auto HwAcc::api() const -> Api
{
    return d->api;
}

auto HwAcc::name() const -> QString
{
    return name(d->api);
}

auto HwAcc::description() const -> QString
{
    return description(d->api);
}

auto HwAcc::name(Api api) -> QString
{
    if (_InRange0(api, NoApi))
        return s_infos[api].name;
    return QString();
}

auto HwAcc::description(Api api) -> QString
{
    if (_InRange0(api, NoApi))
        return s_infos[api].desc;
    return QString();
}

auto HwAcc::api(const QString &name) -> Api
{
    for (auto &info : s_infos) {
        if (info.name == name)
            return info.api;
    }
    return NoApi;
}

auto HwAcc::setSupportedCodecs(const QList<CodecId> &codecs) -> void
{
    d->codecs = codecs;
}

auto HwAcc::setSupportedDeints(const QList<DeintMethod> &deints) -> void
{
    d->deints = deints;
}

auto HwAcc::download(mp_hwdec_ctx *, const mp_image *, mp_image_pool *) -> mp_image*
{
    return nullptr;
}

#ifndef Q_OS_WIN
auto setImeEnabled(QWindow *w, bool enabled) -> void
{
    Q_UNUSED(w); Q_UNUSED(enabled);
}
#endif

WindowAdapter::WindowAdapter(QWindow *parent)
    : QObject(parent)
{
    m_window = parent;
#ifndef Q_OS_WIN
    connect(m_window, &QWindow::windowStateChanged, this, &WindowAdapter::setState);
#endif
}

auto WindowAdapter::screen() const -> QScreen*
{
    return m_window->screen();
}

auto WindowAdapter::setFramelessHint(bool frameless) -> void
{
    auto flags = m_window->flags();
    if (frameless)
        flags |= Qt::FramelessWindowHint;
    else
        flags &= ~Qt::FramelessWindowHint;
    m_window->setFlags(flags);
}

auto WindowAdapter::setFullScreenHint(bool fs) -> void
{
    m_window->setWindowState(fs ? Qt::WindowFullScreen : m_oldState);
}

auto WindowAdapter::isFullScreen() const -> bool
{
    return m_window->windowState() & Qt::WindowFullScreen;
}

auto WindowAdapter::setFullScreen(bool fs) -> void
{
    auto visible = m_window->isVisible();
    setFullScreenHint(fs);
    if (visible)
        m_window->setVisible(true);
}

auto WindowAdapter::setState(Qt::WindowState ws) -> void
{
    if (m_state != ws) {
        m_oldState = m_state;
        m_state = ws;
        emit stateChanged(m_state, m_oldState);
    }
}

auto WindowAdapter::startMoveByDrag(const QPointF &m) -> void
{
    m_started = true;
    m_mouseStartPos = m.toPoint();
    m_winStartPos = m_window->position();
}

auto WindowAdapter::positionArea(const QRect &r, const QSize &s) const -> QRect
{
    const auto m = frameMargins();
    return r.adjusted(m.left(), m.top(), -m.right() - s.width() + 1,
                      -m.bottom() - s.height() + 1);
}

auto WindowAdapter::snapHint(const QPoint &pos, const QSize &size,
                             Qt::Edges edges, int threshold) const -> QPoint
{
    auto p = pos;
    const auto screen = this->screen();
    const auto s = positionArea(screen->availableGeometry(), size);
    const auto g = positionArea(screen->geometry(), size);
    auto check = [&] (int &p, int v) -> bool
        { return (qAbs(p - v) > threshold) ? false : (p = v, true); };
    bool x = false, y = false;
    x = !x && (edges & Qt::LeftEdge) && (check(p.rx(), s.left()) || check(p.rx(), g.left()));
    x = !x && (edges & Qt::RightEdge) && (check(p.rx(), s.right()) || check(p.rx(), g.right()));
    y = !y && (edges & Qt::TopEdge) && (check(p.ry(), s.top()) || check(p.ry(), g.top()));
    y = !y && (edges & Qt::BottomEdge) && (check(p.ry(), s.bottom()) || check(p.ry(), g.bottom()));
    return p;
}

auto WindowAdapter::snapHint(const QPoint &pos, Qt::Edges edges, int threshold) const -> QPoint
{
    return snapHint(pos, m_window->size(), edges, threshold);
}

auto WindowAdapter::moveByDrag(const QPointF &m) -> void
{
    if (m_started) {
        auto p = m_winStartPos + (m.toPoint() - m_mouseStartPos);
        if (isSnappableToEdge())
            p = snapHint(p);
        m_window->setPosition(p);
        setMovingByDrag(true);
    }
}

auto WindowAdapter::endMoveByDrag() -> void
{
    setMovingByDrag(false);
    m_started = false;
    m_mouseStartPos = m_winStartPos = QPoint();
}

auto WindowAdapter::isFrameless() const -> bool
{
    return m_window->flags() & Qt::FramelessWindowHint;
}

auto WindowAdapter::setFrameless(bool frameless) -> void
{
    if (WindowAdapter::isFrameless() == frameless)
        return;
    const bool visible = m_window->isVisible();
    setFramelessHint(frameless);
    if (visible)
        m_window->show();
}

auto createAdapter(QWindow *w) -> WindowAdapter*;

auto adapter(QWindow *w) -> WindowAdapter*
{
    static const constexpr char *property = "_b_window_adpater";
    auto a = static_cast<WindowAdapter*>(w->property(property).value<WindowAdapter*>());
    if (!a) {
        a = createAdapter(w);
        w->setProperty(property, QVariant::fromValue(a));
    }
    return a;
}

}
