#include "jsonstorage.hpp"
#include "json.hpp"
#include "misc/log.hpp"
#include "player/pref.hpp"
#include "player/mrlstate.hpp"

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
        QVariant var;
#define QT_CASE(vt, type) \
    case QMetaType::vt: var.setValue(_FromJson<type>(*it)); break;
        switch ((int)p.type()) {
        QT_CASE(Bool, bool)
        QT_CASE(Int, int)
        QT_CASE(UInt, uint)
        QT_CASE(LongLong, qlonglong)
        QT_CASE(ULongLong, qulonglong)
        QT_CASE(Double, double)
#undef QT_CASE
#define QT_CASE(type) \
        case QMetaType::type: var.setValue(_FromJson<type>(*it)); break;
        QT_CASE(QString)
        QT_CASE(QPoint)
        QT_CASE(QPointF)
        QT_CASE(QSize)
        QT_CASE(QSizeF)
        QT_CASE(QColor)
        QT_CASE(QStringList)
#undef QT_CASE
        case QVariant::UserType: {
            const auto user = p.userType();
            auto conv = _EnumNameVariantConverter(user);
            if (!conv.isNull())
                var = conv.nameToVariant(it->toString());
#define USER_CASE(type) \
            else if (_Is<type>(user)) { var.setValue(_FromJson<type>(*it)); }
            USER_CASE(VideoColor)
            USER_CASE(OpenMediaInfo)
            USER_CASE(OpenMediaInfo)
            USER_CASE(OsdTheme)
            USER_CASE(PlaylistTheme)
            USER_CASE(ChannelLayoutMap)
            USER_CASE(Autoloader)
            USER_CASE(Autoloader)
            USER_CASE(OsdStyle)
            USER_CASE(MouseActionMap)
            USER_CASE(DeintCaps)
            USER_CASE(DeintCaps)
            USER_CASE(AudioNormalizerOption)
            USER_CASE(Shortcuts)
            USER_CASE(Mrl)
#undef USER_CASE
            else {
                qDebug("USER_CASE(%s)", p.typeName());
                continue;
            }
            break;
        } default:
            _Fatal("Typename '%%' is not handled.", p.typeName());
            break;
        }
        Q_ASSERT(var.type() == p.type());
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
        const QVariant var = p.read(obj);
        QJsonValue value;
        switch ((int)var.type()) {
        case QVariant::Bool:
            value = var.toBool();
            break;
        case QVariant::Int:
        case QVariant::UInt:
            value = var.toInt();
            break;
        case QVariant::LongLong:
        case QVariant::ULongLong:
        case QVariant::Double:
            value = var.toDouble();
            break;
#define QT_CASE(type) \
        case QMetaType::type: { value = _ToJson(var.value<type>()); break; }
        QT_CASE(QString)
            QT_CASE(QPoint)
            QT_CASE(QPointF)
            QT_CASE(QSize)
            QT_CASE(QSizeF)
            QT_CASE(QColor)
            QT_CASE(QStringList)
#undef QT_CASE
        case QVariant::UserType: {
            const auto user = var.userType();
            auto conv = _EnumNameVariantConverter(user);
            if (!conv.isNull())
                value = conv.variantToName(var);
#define USER_CASE(type) else if (_Is<type>(user)) {value = _ToJson(var.value<type>());}
            USER_CASE(VideoColor)
            USER_CASE(OpenMediaInfo)
            USER_CASE(OpenMediaInfo)
            USER_CASE(OsdTheme)
            USER_CASE(PlaylistTheme)
            USER_CASE(ChannelLayoutMap)
            USER_CASE(Autoloader)
            USER_CASE(Autoloader)
            USER_CASE(OsdStyle)
            USER_CASE(MouseActionMap)
            USER_CASE(DeintCaps)
            USER_CASE(DeintCaps)
            USER_CASE(AudioNormalizerOption)
            USER_CASE(Shortcuts)
            USER_CASE(Mrl)
            else {
                qDebug("USER_CASE(%s)", var.typeName());
                continue;
            }
            break;
        } default:
            _Error("Typename '%%' is not handled.", var.typeName());
            Q_ASSERT(false);
            break;
        }
        json.insert(QString::fromLatin1(p.name()), value);
    }
    return json;
}
