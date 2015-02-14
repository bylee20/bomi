#ifndef JSON_HPP
#define JSON_HPP

#include "tmp/static_for.hpp"
#include "is_convertible.hpp"
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <deque>
#include <vector>
#include <list>

template<class T>
class EnumInfo;

template<class T>
struct JsonIO { };

struct JsonQStringType { SCA qt_type = QJsonValue::String; };

namespace detail {

enum class JsonValueType { Number, Enum, Convertible, None };

template<class T>
SCIA jsonValueType() -> JsonValueType
{
    return tmp::is_enum_class<T>() ? JsonValueType::Enum
         : tmp::is_arithmetic<T>() ? JsonValueType::Number
         : tmp::is_convertible_to_json<T>() ? JsonValueType::Convertible
                                            : JsonValueType::None;
}

template<class T, detail::JsonValueType type = detail::jsonValueType<T>()>
struct JsonValueIO;

template<class T>
struct JsonValueIO<T, detail::JsonValueType::Number>  {
    static_assert(!tmp::is_enum<T>() && !tmp::is_enum_class<T>(), "!!!");
    auto toJson(T t) const -> QJsonValue { return (double)t; }
    auto fromJson(T &val, const QJsonValue &json) const -> bool
    {
        if (!json.isDouble())
            return false;
        val = json.toDouble(val);
        return true;
    }
    SCA qt_type = QJsonValue::Double;
};

template<class T>
struct JsonValueIO<T, detail::JsonValueType::Convertible> {
    template<class S>
    using JsonType = typename tmp::convert_json<S>::JsonType;
    template<class S>
    SCIA is_array() -> bool { return tmp::is_same<QJsonArray, JsonType<S>>(); }
    auto toJson(const T &t) const -> JsonType<T> { return t.toJson(); }
    template<class S = T>
    auto fromJson(S &val, const QJsonValue &json) const -> tmp::enable_if_t<!is_array<S>(), bool>
    { return json.isObject() ? fromJson(val, json.toObject()) : false; }
    template<class S = T>
    auto fromJson(S &val, const QJsonValue &json) const -> tmp::enable_if_t<is_array<S>(), bool>
    { return json.isArray() ? fromJson(val, json.toArray()) : false; }
    auto fromJson(T &val, const JsonType<T> &json) const -> bool
    { return val.setFromJson(json); }
    SCA qt_type = is_array<T>() ? QJsonValue::Array : QJsonValue::Object;
};

template<class T>
struct JsonValueIO<T, detail::JsonValueType::Enum> : JsonQStringType {
    auto toJson(T t) const -> QJsonValue { return EnumInfo<T>::name(t); }
    auto fromJson(T &val, const QJsonValue &json) const -> bool
    { return EnumInfo<T>::fromName(val, json.toString()); }
};

template<>
struct JsonValueIO<bool> {
    auto toJson(bool val) const -> QJsonValue { return val; }
    auto fromJson(bool &val, const QJsonValue &json) const -> bool
    {
        if (!json.isBool())
            return false;
        val = json.toBool(val);
        return true;
    }
    SCA qt_type = QJsonValue::Bool;
};

template<class T>
struct IOType {
    using U = tmp::remove_cref_t<T>;
    template<class S>
    static auto checkIO(S *) -> decltype(::JsonIO<S>::io(), std::true_type());
    template<class>
    static auto checkIO(...) -> std::false_type;
    SCA primitive = detail::jsonValueType<U>() != detail::JsonValueType::None;
    SCA inner_io = decltype(checkIO<T>(0))::value;
};

template<class T, bool is_primitive = IOType<T>::primitive, bool inner_io = IOType<T>::inner_io>
struct JsonIOSelector;

template<class T>
struct JsonIOSelector<T, true, false> {
    using U = tmp::remove_cref_t<T>;
    static auto select(const T*) { static const JsonValueIO<U> io{}; return &io; }
};

template<class T>
struct JsonIOSelector<T, false, true> {
    using U = tmp::remove_cref_t<T>;
    static auto select(const T*) { return ::JsonIO<U>::io(); }
};

template<class T>
struct JsonIOSelector<T, false, false> {
    using U = tmp::remove_cref_t<T>;
    static auto select(const T*) { static const JsonIO<U> io{}; return &io; }
};

}

template<class T>
SIA json_io(const T*) { return detail::JsonIOSelector<T>::select((T*)0); }

template<class T>
SIA json_io() { return json_io((T*)0); }

template<class T, class D>
struct JsonMemberEntry {
    JsonMemberEntry() = default;
    JsonMemberEntry(const QString &key, D T::*data): key(key), data(data) { }
    QString key; D T::*data = nullptr;
};

template<class T, class D>
struct JsonReferenceEntry {
    JsonReferenceEntry() = default;
    JsonReferenceEntry(const QString &key, D&(T::*ref)()): key(key), ref(ref) { }
    QString key; D&(T::*ref)() = nullptr;
};

template<class T, class Get, class Set>
struct JsonPropertyEntry {
    JsonPropertyEntry() = default;
    JsonPropertyEntry(const QString &key, Get get, Set set)
        : key(key), get(get), set(set) { }
    QString key; Get get; Set set;
};

namespace detail {

template<class T>
struct WriteToJsonObject {
    WriteToJsonObject(const T *t): from(t) { }
    template<class D>
    auto operator () (const JsonMemberEntry<T, D> &entry) const -> void
    { m_json.insert(entry.key, json_io<D>()->toJson(from->*entry.data)); }
    template<class D>
    auto operator () (const JsonReferenceEntry<T, D> &entry) const -> void
    { m_json.insert(entry.key, json_io<D>()->toJson((const_cast<T*>(from)->*entry.ref)())); }
    template<class Getter, class Setter>
    auto operator () (const JsonPropertyEntry<T, Getter, Setter> &entry) const -> void
    { auto d = (from->*entry.get)(); m_json.insert(entry.key, json_io(&d)->toJson(d)); }
    auto json() const -> const QJsonObject& { return m_json; }
private:
    const T *from = nullptr;
    mutable QJsonObject m_json;
};

template<class T>
struct ReadFromJsonObject {
    ReadFromJsonObject(T *t, const QJsonObject &json): json(json), to(t) { }
    template<class D>
    auto operator () (const JsonMemberEntry<T, D> &entry) const -> void
    {
        const auto it = json.constFind(entry.key);
        if (it != json.constEnd() && !json_io<D>()->fromJson(to->*entry.data, *it))
            m_ok = false;
    }
    template<class D>
    auto operator () (const JsonReferenceEntry<T, D> &entry) const -> void
    {
        const auto it = json.constFind(entry.key);
        if (it != json.constEnd() && !json_io<D>()->fromJson((to->*entry.ref)(), *it))
            m_ok = false;
    }
    template<class Getter, class Setter>
    auto operator () (const JsonPropertyEntry<T, Getter, Setter> &entry) const -> void
    {
        const auto it = json.constFind(entry.key);
        if (it != json.constEnd()) {
            auto d = tmp::remove_cref_t<decltype((to->*entry.get)())>();
            if (!json_io(&d)->fromJson(d, *it))
                m_ok = false;
            else
                (to->*entry.set)(d);
        }
    }
    bool isOk() const { return m_ok; }
private:
    QJsonObject json;
    T *to = nullptr;
    mutable bool m_ok = true;
};
}

template<class T, class... Entry>
class JsonObjectIO {
    SCA Num = sizeof...(Entry);
public:
    JsonObjectIO(Entry&&... entry) {
        m_entries = std::make_tuple(entry...);
    }
    auto toJson(const T &from) const -> QJsonObject
    {
        detail::WriteToJsonObject<T> write(&from);
        tmp::static_for_each<0, Num>(m_entries, write);
        return write.json();
    }
    auto fromJson(T &to, const QJsonValue &json) const -> bool
    { return json.isObject() ? fromJson(to, json.toObject()) : false; }
    auto fromJson(T &to, const QJsonObject &json) const -> bool
    {
        detail::ReadFromJsonObject<T> read(&to, json);
        tmp::static_for_each<0, Num>(m_entries, read);
        return read.isOk();
    }
    SCA qt_type = QJsonValue::Object;
private:
    std::tuple<Entry...> m_entries;
};

template<class T, class Container = QList<T>>
struct JsonArrayIO {
    auto toJson(const Container &from) const -> QJsonArray
    {
        QJsonArray json;
        for (auto it = from.begin(); it != from.end(); ++it)
            json.push_back(json_io((T*)nullptr)->toJson(*it));
        return json;
    }
    auto fromJson(Container &to, const QJsonValue &json) const -> bool
    { return json.isArray() ? fromJson(to, json.toArray()) : false; }
    auto fromJson(Container &to, const QJsonArray &json) const -> bool
    {
        const auto io = json_io((T*)nullptr);
        Container array;
        for (auto it = json.begin(); it != json.end(); ++it) {
            if (io->qt_type != (*it).type())
                return false;
            array.push_back(T());
            io->fromJson(array.back(), *it);
        }
        to = array;
        return true;
    }
    SCA qt_type = QJsonValue::Array;
};

namespace detail {
template<class T,
         bool use_str = tmp::is_convertible_to_string<T>(),
         bool use_json = tmp::is_convertible_to_json<T>()>
struct Jsonkey;

template<class T, bool use_json>
struct Jsonkey<T, true, use_json>  {
    SIA from(const T &t) -> QString { return tmp::convert_string<T>::from(t); }
    SIA to(const QString &json) -> T { return tmp::convert_string<T>::to(json); }
};
template<class T>
struct Jsonkey<T, false, true>  {
    SIA from(const T &t) -> QString { return _JsonToString(tmp::convert_json<T>::from(t)); }
    SIA to(const QString &json) -> T { return tmp::convert_json<T>::to(_JsonFromString(json)); }
};
}

template<class T>
SIA json_key_from(const T &t) -> QString { return detail::Jsonkey<T>::from(t); }
template<>
inline auto json_key_from<int>(const int &t) -> QString { return QString::number(t); }
template<class T>
SIA json_key_to(const QString &json) -> T { return detail::Jsonkey<T>::to(json); }
template<>
inline auto json_key_to<int>(const QString &json) -> int { return json.toInt(); }

template<class Key, class T, class Container = QMap<Key, T>>
struct JsonMapIO {
    auto toJson(const Container &from) const -> QJsonObject
    {
        QJsonObject json;
        for (auto it = from.begin(); it != from.end(); ++it)
            json.insert(json_key_from(it.key()), json_io<T>()->toJson(*it));
        return json;
    }
    auto fromJson(Container &to, const QJsonValue &json) const -> bool
    { return json.isObject() ? fromJson(to, json.toObject()) : false; }
    auto fromJson(Container &to, const QJsonObject &json) const -> bool
    {
        const auto io = json_io<T>();
        Container map;
        for (auto it = json.begin(); it != json.end(); ++it) {
            if ((*it).type() != io->qt_type)
                return false;
            auto &data = map[json_key_to<Key>(it.key())];
            io->fromJson(data, *it);
        }
        to = map;
        return true;
    }
    SCA qt_type = QJsonValue::Object;
};

template<class T, class D>
SCIA _JE(const QString &key, D T::*data)
    { return JsonMemberEntry<T, D>(key, data); }

template<class T, class D>
SCIA _JE(const QString &key, D&(T::*ref)())
    { return JsonReferenceEntry<T, D>(key, ref); }

template<class T, class D, class Set>
SCIA _JE(const QString &key, D(T::*get)()const, Set set)
    { return JsonPropertyEntry<T, decltype(get), Set>(key, get, set); }

template<class T, class... Entry>
SCIA _JIO(Entry&&... entry)
    { return JsonObjectIO<T, Entry...>(std::forward<Entry>(entry)...); }

//#define JSON_CLASS ClassName
#define JE(a, ...) _JE(u###a##_q, &JSON_CLASS::a, ##__VA_ARGS__)
#define JIO(...) _JIO<JSON_CLASS>(__VA_ARGS__)
#define JSON_DECLARE_FROM_TO_FUNCTIONS_IO(j) \
auto JSON_CLASS::setFromJson(const QJsonObject &json) -> bool \
    { return j.fromJson(*this, json); } \
auto JSON_CLASS::toJson() const -> QJsonObject { return j.toJson(*this); }
#define JSON_DECLARE_FROM_TO_FUNCTIONS JSON_DECLARE_FROM_TO_FUNCTIONS_IO(jio)

#define JSON_IO_POINT(c) _JIO<c>(_JE<c>(u"x"_q, &c::rx), _JE<c>(u"y"_q, &c::ry))
auto json_io(const QPoint*) -> const decltype(JSON_IO_POINT(QPoint))*;
auto json_io(const QPointF*) -> const decltype(JSON_IO_POINT(QPointF))*;

#define JSON_IO_SIZE(c) _JIO<c>(_JE<c>(u"width"_q, &c::rwidth), _JE<c>(u"height"_q, &c::rheight))
auto json_io(const QSize*) -> const decltype(JSON_IO_SIZE(QSize))*;
auto json_io(const QSizeF*) -> const decltype(JSON_IO_SIZE(QSizeF))*;

#define JSON_DECLARE_MAP_IO(C) \
template<class Key, class T> \
auto json_io(const C<Key, T>*) \
{ static const JsonMapIO<Key, T, C<Key, T>> io{}; return &io; }
JSON_DECLARE_MAP_IO(QMap)
JSON_DECLARE_MAP_IO(QHash)
JSON_DECLARE_MAP_IO(std::map)

#define JSON_DECLARE_ARRAY_IO(C) \
template<class T> \
auto json_io(const C<T>*) \
{ static const JsonArrayIO<T, C<T>> io{}; return &io; }
JSON_DECLARE_ARRAY_IO(QList)
JSON_DECLARE_ARRAY_IO(QVector)
JSON_DECLARE_ARRAY_IO(QLinkedList)
JSON_DECLARE_ARRAY_IO(std::vector)
JSON_DECLARE_ARRAY_IO(std::list)
JSON_DECLARE_ARRAY_IO(std::deque)
auto json_io(const QStringList*) -> const JsonArrayIO<QString, QStringList>*;

template<>
struct JsonIO<QString> : JsonQStringType {
    auto toJson(const QString &val) const -> QJsonValue { return val; }
    auto fromJson(QString &val, const QJsonValue &json) const -> bool
    {
        if (!json.isString())
            return false;
        val = json.toString(val);
        return true;
    }
};

template<>
struct JsonIO<QByteArray> : JsonQStringType {
    auto toJson(const QByteArray &val) const -> QJsonValue { return QString::fromUtf8(val); }
    auto fromJson(QByteArray &val, const QJsonValue &json) const -> bool
    {
        if (!json.isString())
            return false;
        val = json.toString().toUtf8();
        return true;
    }
};

template<>
struct JsonIO<QColor> : JsonQStringType {
    auto toJson(const QColor &t) const -> QJsonValue {return t.name(QColor::HexArgb);}
    auto fromJson(QColor &val, const QJsonValue &json) const -> bool
    {
        const QColor color(json.toString());
        if (!color.isValid())
            return false;
        val = color;
        return true;
    }
};

template<>
struct JsonIO<QFont> : JsonQStringType {
    auto toJson(const QFont &t) const -> QJsonValue { return t.toString(); }
    auto fromJson(QFont &val, const QJsonValue &json) const -> bool
    { return val.fromString(json.toString()); }
};

template<>
struct JsonIO<QKeySequence> : JsonQStringType {
    auto toJson(const QKeySequence &t) const -> QJsonValue { return t.toString(); }
    auto fromJson(QKeySequence &val, const QJsonValue &json) const -> bool
    {
        if (!json.isString())
            return false;
        val = QKeySequence::fromString(json.toString());
        return true;
    }
};

template<>
struct JsonIO<QDateTime> {
    static auto toJson(const QDateTime &dt) -> QJsonValue
        { return (double)dt.toMSecsSinceEpoch(); }
    static auto fromJson(QDateTime &dt, const QJsonValue &json) -> bool
    {
        if (!json.isDouble())
            return false;
        dt.setMSecsSinceEpoch(std::llround(json.toDouble()));
        return true;
    }
    SCA qt_type = QJsonValue::Double;
};

template<class T>
auto _ToJson(const T &t) { return json_io<T>()->toJson(t); }

template<class T>
auto _FromJson(const QJsonValue &json) -> T
{ T t = T(); json_io<T>()->fromJson(t, json); return t; }

template<class T>
auto _JsonType() -> QJsonValue::Type { return json_io<T>()->qt_type; }

#endif // JSON_HPP
