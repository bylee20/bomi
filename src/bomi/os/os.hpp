#ifndef OS_HPP
#define OS_HPP

#include <QWindow>

enum class DeintMethod;                 enum class CodecId;
struct mp_image_pool;                   struct mp_image;
struct mp_hwdec_ctx;

namespace OS {

auto initialize() -> void;
auto finalize() -> void;

auto setScreensaverEnabled(bool enabled) -> void;
auto setImeEnabled(QWindow *w, bool enabled) -> void;

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
    auto isMovingByDrag() const -> bool { return m_moving; }
    auto isMoveByDragStarted() const -> bool { return m_started; }
    auto posForMovingByDrag() const -> QPoint { return m_winStartPos; }
    auto mousePosForMovingByDrag() const -> QPoint { return m_mouseStartPos; }
    auto winId() const -> WId { return m_widget->winId(); }
    auto widget() const -> QWidget* { return m_widget; }
protected:
    WindowAdapter(QWidget *parent);
    auto setMovingByDrag(bool moving) { m_moving = moving; }
private:
    QWidget *m_widget = nullptr;

    bool m_started = false, m_moving = false;
    QPoint m_winStartPos, m_mouseStartPos;
};

auto adapter(QWidget *w) -> WindowAdapter*;

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
