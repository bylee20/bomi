#ifndef HWACC_HPP
#define HWACC_HPP

#include <QObject>

enum class DeintMethod;

class HwAcc {
public:
    enum Type {None, VaApiGLX, VdpauX11};
    static auto isAvailable() -> bool;
    static auto initialize() -> void;
    static auto finalize() -> void;
    static auto fullCodecList() -> QStringList;
    static auto fullDeintList() -> QList<DeintMethod>;
    static auto codecDescription(const QString &codec) -> QString;
    static auto supports(const QString &codecs) -> bool;
    static auto supports(DeintMethod method) -> bool;
    static auto type(const QString &name) -> Type;
    static auto type() -> Type;
    static auto description() -> QString;
    static auto name(Type type) -> QString;
    static auto name() -> QString;
};

Q_DECLARE_METATYPE(HwAcc::Type);

#endif // HWACC_HPP
