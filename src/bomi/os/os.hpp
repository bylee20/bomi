#ifndef OS_HPP
#define OS_HPP

#include <QWindow>

enum class DeintMethod;                 enum class CodecId;
struct mp_image_pool;                   struct mp_image;
struct mp_hwdec_ctx;

namespace OS {

auto initialize() -> void;
auto finalize() -> void;

auto canAssociateFileTypes() -> bool;
auto unassociateFileTypes(QWindow *w, bool global) -> bool;
auto associateFileTypes(QWindow *w, bool global, const QStringList &exts) -> bool;

auto screensaverMethods() -> QStringList;
auto setScreensaverMethod(const QString &method) -> void;
auto setScreensaverEnabled(bool enabled) -> void;

auto shutdown() -> bool;
auto canShutdown() -> bool;

auto processTime() -> quint64; // us
auto systemTime()  -> quint64; // us
auto totalMemory() -> double;
auto usingMemory() -> double;

auto defaultFont() -> QFont;
auto defaultFixedFont() -> QFont;

auto opticalDrives() -> QStringList;
auto refreshRate() -> qreal;

class WindowAdapter : public QObject {
    Q_OBJECT
public:
    virtual auto isFullScreen() const -> bool;
    virtual auto setFullScreen(bool fs) -> void;
    virtual auto isFrameless() const -> bool;
    virtual auto setFrameless(bool frameless) -> void;
    virtual auto isAlwaysOnTop() const -> bool = 0;
    virtual auto setAlwaysOnTop(bool onTop) -> void = 0;
    virtual auto startMoveByDrag(const QPointF &m) -> void;
    virtual auto moveByDrag(const QPointF &m) -> void;
    virtual auto endMoveByDrag() -> void;
    virtual auto setImeEnabled(bool enabled) -> void = 0;
    virtual auto isImeEnabled() const -> bool = 0;
    virtual auto isSnappableToEdge() const -> bool { return false; }
    virtual auto showMaximized() -> void { m_window->showMaximized(); }
    virtual auto showMinimized() -> void { m_window->showMinimized(); }
    virtual auto showNormal() -> void { m_window->showNormal(); }
    virtual auto screen() const -> QScreen*;
    auto snapHint(const QPoint &pos, Qt::Edges edges = Qt::TopEdge
                    | Qt::BottomEdge | Qt::RightEdge | Qt::LeftEdge,
                  int threshold = 10) const -> QPoint;
    auto snapHint(const QPoint &pos, const QSize &size,
                  Qt::Edges edges = Qt::TopEdge | Qt::BottomEdge
                    | Qt::RightEdge | Qt::LeftEdge,
                  int threshold = 10) const -> QPoint;
    auto state() const -> Qt::WindowState { return m_state; }
    auto oldState() const -> Qt::WindowState { return m_oldState; }
    auto positionArea(const QRect &rect, const QSize &size) const -> QRect;
    auto frameMargins() const -> QMargins
        { return isFrameVisible() ? m_window->frameMargins() : QMargins(); }
    auto isFrameVisible() const -> bool { return !isFullScreen() && !isFrameless(); }
    auto geometry() const -> QRect { return isFrameVisible() ? m_window->geometry()
                                                             : m_window->frameGeometry(); }
    auto isMovingByDrag() const -> bool { return m_moving; }
    auto isMoveByDragStarted() const -> bool { return m_started; }
    auto posForMovingByDrag() const -> QPoint { return m_winStartPos; }
    auto mousePosForMovingByDrag() const -> QPoint { return m_mouseStartPos; }
    auto winId() const -> WId { return m_window->winId(); }
    auto window() const -> QWindow* { return m_window; }
signals:
    void stateChanged(Qt::WindowState state, Qt::WindowState old);
protected:
    WindowAdapter(QWindow *parent);
    auto setMovingByDrag(bool moving) { m_moving = moving; }
    auto setState(Qt::WindowState ws) -> void;
    auto setFramelessHint(bool frameless) -> void;
    auto setFullScreenHint(bool fs) -> void;
private:
    QWindow *m_window = nullptr;
    bool m_started = false, m_moving = false;
    QPoint m_winStartPos, m_mouseStartPos;
    Qt::WindowState m_state = Qt::WindowNoState, m_oldState = Qt::WindowNoState;
};

auto adapter(QWindow *w) -> WindowAdapter*;

class HwAcc {
public:
    enum Api { VaApiGLX, VdpauX11, Dxva2Copy, NoApi };
    virtual ~HwAcc();
    auto isAvailable() const -> bool;
    auto supports(CodecId codec) -> bool;
    auto supports(DeintMethod method) -> bool;
    auto api() const -> Api;
    auto name() const -> QString;
    auto description() const -> QString;
    virtual auto download(mp_hwdec_ctx *ctx, const mp_image *mpi,
                          mp_image_pool *pool) -> mp_image*;
    static auto fullCodecList() -> QList<CodecId>;
    static auto name(Api api) -> QString;
    static auto description(Api api) -> QString;
    static auto api(const QString &name) -> Api;
protected:
    HwAcc(Api api = NoApi);
    auto setSupportedCodecs(const QList<CodecId> &codecs) -> void;
    auto setSupportedDeints(const QList<DeintMethod> &deints) -> void;
private:
    struct Data;
    Data *d;
    friend auto hwAcc() -> HwAcc*;
};

auto hwAcc() -> HwAcc*;

}

Q_DECLARE_METATYPE(OS::HwAcc::Api);

#endif // OS_HPP
