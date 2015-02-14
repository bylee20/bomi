#ifndef OS_HPP
#define OS_HPP

enum class DeintMethod;                 enum class CodecId;
struct mp_image_pool;                   struct mp_image;
struct mp_hwdec_ctx;

namespace OS {

auto initialize() -> void;
auto finalize() -> void;

auto isFullScreen(const QWidget *w) -> bool;
auto isAlwaysOnTop(const QWidget *w) -> bool;
auto setAlwaysOnTop(QWidget *w, bool onTop) -> void;
auto setFullScreen(QWidget *w, bool fs) -> void;
auto setScreensaverDisabled(bool disabled) -> void;

auto shutdown() -> bool;
auto canShutdown() -> bool;

auto processTime() -> quint64; // us
auto systemTime()  -> quint64; // us
auto totalMemory() -> double;
auto usingMemory() -> double;

auto opticalDrives() -> QStringList;
auto refreshRate() -> qreal;

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
