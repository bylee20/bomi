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

auto opticalDrives() -> QStringList;
auto refreshRate() -> qreal;

class WindowAdapter : public QObject {
    Q_OBJECT
public:
    virtual auto isFullScreen() const -> bool;
    virtual auto setFullScreen(bool fs) -> void;
    virtual auto isAlwaysOnTop() const -> bool = 0;
    virtual auto setAlwaysOnTop(bool onTop) -> void = 0;
    auto winId() const -> WId { return m_widget->winId(); }
    auto widget() const -> QWidget* { return m_widget; }
protected:
    WindowAdapter(QWidget *parent);
private:
    QWidget *m_widget = nullptr;
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
