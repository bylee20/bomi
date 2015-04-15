#include "jsonstorage.hpp"
#include "json.hpp"
#include "misc/log.hpp"
#include "pref/pref.hpp"
#include "player/mrlstate.hpp"
#include "video/interpolatorparams.hpp"

DECLARE_LOG_CONTEXT(JSON)

template<>
struct JsonIO<Mrl> : JsonQStringType {
    static auto toJson(const Mrl &mrl) -> QJsonValue { return mrl.toString(); }
    static auto fromJson(Mrl &mrl, const QJsonValue &json) -> bool
    {
        if (!json.isString())
            return false;
        mrl = Mrl::fromString(json.toString());
        return true;
    }
};

JsonStorage::JsonStorage(const QString &fileName) noexcept
    : m_fileName(fileName)
{

}

auto JsonStorage::write(const QJsonObject &json) noexcept -> bool
{
    setError(NoError);
    QFile file(m_fileName);
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        setError(OpenError);
        return false;
    }
    QJsonDocument doc(json);
    file.write(doc.toJson());
    return true;
}

auto JsonStorage::setError(Error error) noexcept -> void
{
    m_error = error;
    switch (m_error) {
    case NoFile:
        _Warn("Warning: '%%' file doesn't exist", m_fileName);
        break;
    case OpenError:
        _Error("Error: Cannot open '%%' file", m_fileName);
        break;
    case ParseError:
        _Error("Error: Cannot parse '%%' file at %%(%%)",
               m_fileName, m_parseError.offset, m_parseError.errorString());
        break;
    default:
        break;
    }
}

auto JsonStorage::read() noexcept -> QJsonObject
{
    setError(NoError);
    QFile file(m_fileName);
    if (!file.exists()) {
        setError(NoFile);
        return QJsonObject();
    }
    if (!file.open(QFile::ReadOnly)) {
        setError(OpenError);
        return QJsonObject();
    }
    const auto json = QJsonDocument::fromJson(file.readAll(), &m_parseError);
    if (m_parseError.error)
        setError(ParseError);
    return json.object();
}

/******************************************************************************/

template<class T>
SIA _Is(int type) -> bool { return type == qMetaTypeId<T>(); }

struct JVConvert;

using f_j2v = auto (*)(const JVConvert *d, const QJsonValue &, QVariant&) -> bool;
using f_v2j = auto (*)(const JVConvert *d, const QVariant &) -> QJsonValue;
using f_def = auto (*)(const JVConvert *d) -> QVariant;
struct JVConvert {
    f_j2v j2v;
    f_v2j v2j;
    QVariant def;
    QJsonValue::Type jsonType;
    EnumNameVariantConverter enum_;
    int metaType;
};

static auto convs() -> const QHash<int, JVConvert>&
{
    static const QHash<int, JVConvert> convs = [] () {
        QHash<int, JVConvert> c;

        using uint = unsigned int;
        using ushort = unsigned short;
        using uchar = unsigned char;
        using schar = signed char;
        using ulong = unsigned long;

    #define INSERT(type) \
        c[qMetaTypeId<type>()] = { \
            [] (const JVConvert*, const QJsonValue &j, QVariant &var) -> bool { \
                type t = var.value<type>(); \
                if (json_io<type>()->fromJson(t, j)) { \
                    var.setValue<type>(t); \
                    return true; \
                } \
                return false; \
            }, \
            [] (const JVConvert*, const QVariant &v) -> QJsonValue { \
                return _ToJson(v.value<type>()); \
            }, QVariant::fromValue<type>(type()), _JsonType<type>(), {}, qMetaTypeId<type>() \
        }

        INSERT(bool);

        INSERT(char);
        INSERT(uchar);
        INSERT(schar);
        INSERT(int);
        INSERT(uint);
        INSERT(short);
        INSERT(ushort);
        INSERT(long);
        INSERT(ulong);
        INSERT(qlonglong);
        INSERT(qulonglong);
        INSERT(float);
        INSERT(double);

        INSERT(QDateTime);
        INSERT(QString);
        INSERT(QPoint);
        INSERT(QPointF);
        INSERT(QSize);
        INSERT(QSizeF);
        INSERT(QColor);
        INSERT(QStringList);
        INSERT(QFont);

        INSERT(VideoColor);
        INSERT(OpenMediaInfo);
        INSERT(MotionIntrplOption);
        INSERT(OsdTheme);
        INSERT(ControlsTheme);
        INSERT(PlaylistTheme);
        INSERT(ChannelLayoutMap);
        INSERT(Autoloader);
        INSERT(OsdStyle);
        INSERT(MouseActionMap);
        INSERT(DeintOptionSet);
        INSERT(AudioNormalizerOption);
        INSERT(Mrl);
        INSERT(VideoEffects);
        INSERT(StreamList);
        INSERT(AudioEqualizer);
        INSERT(StreamList);
        INSERT(Locale);
        INSERT(LogOption);
        INSERT(QList<CodecId>);
        INSERT(Shortcuts);
        INSERT(ShortcutMap);
        INSERT(EncodingInfo);
        INSERT(Steps);
        INSERT(IntrplParamSetMap);
        INSERT(WindowSize);
        INSERT(QList<WindowSize>);

        for (auto type : _EnumMetaTypeIds()) {
            auto &ec = c[type];
            ec.enum_ = _EnumNameVariantConverter(type);
            ec.j2v = [] (const JVConvert *d, const QJsonValue &j, QVariant &var) {
                Q_ASSERT(!d->enum_.isNull());
                var = d->enum_.nameToVariant(j.toString());
                return true;
            };
            ec.v2j = [] (const JVConvert *d, const QVariant &v) -> QJsonValue {
                Q_ASSERT(!d->enum_.isNull());
                return d->enum_.variantToName(v);
            };
            ec.jsonType = QJsonValue::String;
        }

        return c;
    }();
    return convs;
}

auto _JsonFromQVariant(const QVariant &var) -> QJsonValue
{
    const int metaType = var.userType();
    auto it = convs().find(metaType);
    if (it != convs().end())
        return it->v2j(&it.value(), var);
    _Error("Unknown type for conversion: %%", var.typeName());
    return QJsonValue(QJsonValue::Undefined);
}

auto _JsonToQVariant(const QJsonValue &json, int metaType, const QVariant &def) -> QVariant
{
    QVariant var = def;
    const auto it = convs().find(metaType);
    if (it != convs().end()) {
        if (it->j2v(&it.value(), json, var))
            return var;
    } else
        _Error("Unknown type for conversion: %%", QMetaType::typeName(metaType));
    return def;
}

auto _JsonToQVariant(const QJsonValue &json, int metaType) -> QVariant
{
    return _JsonToQVariant(json, metaType, QVariant());
}

auto _JsonSetToQVariant(const QJsonValue &json, QVariant &var) -> bool
{
    auto v = _JsonToQVariant(json, var.userType());
    if (v.isValid())
        var = v;
    return v.isValid();
}

auto _JsonType(int metaType) -> QJsonValue::Type
{
    const auto it = convs().find(metaType);
    if (it != convs().end())
        return it->jsonType;
    _Error("Unknown type for conversion: %%", QMetaType::typeName(metaType));
    return QJsonValue::Undefined;
}

auto _JsonToQVariant(const QJsonValue &json, const QVariant &def) -> QVariant
{
    auto v = _JsonToQVariant(json, def.userType());
    return v.isValid() ? v : def;
}

auto _JsonToQObject(const QJsonObject &json, QObject *obj) -> bool
{
    bool res = true;
    auto mo = obj->metaObject();
    for (int i = 1; i < mo->propertyCount(); ++i) {
        auto p = mo->property(i);
        auto it = json.find(QString::fromLatin1(p.name()));
        if (it == json.end()) {
            _Warn("Cannot find property '%%' in '%%'", p.name(), mo->className());
            res = false;
            continue;
        }
        const auto var = _JsonToQVariant(*it, p.read(obj));
        Q_ASSERT(var.userType() == p.userType());
        p.write(obj, var);
    }
    return res;
}

auto _JsonFromQObject(const QObject *obj) -> QJsonObject
{
    QJsonObject json;
    auto mo = obj->metaObject();
    for (int i = 1; i < mo->propertyCount(); ++i) {
        auto p = mo->property(i);
        const auto value = _JsonFromQVariant(p.read(obj));
        json.insert(QString::fromLatin1(p.name()), value);
    }
    return json;
}

auto _QVariantFromType(int metaType) -> QVariant
{
    const auto it = convs().find(metaType);
    if (it != convs().end())
        return it->def;
    return QVariant();
}
