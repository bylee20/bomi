#ifndef JSON_HPP
#define JSON_HPP

#include "stdafx.hpp"

template<class T>
class EnumInfo;

class JsonObject;

class JsonValue {
public:
    enum Type {
        Null =  QJsonValue::Null,
        Bool = QJsonValue::Bool,
        Double = QJsonValue::Double,
        String = QJsonValue::String,
        Array = QJsonValue::Array,
        Object = QJsonValue::Object,
        Undefined = QJsonValue::Undefined
    };

    JsonValue() = default;
    JsonValue(const QJsonValue &json): m(json) { }
    JsonValue(bool b): m(b) { }
    JsonValue(double n): m(n) { }
    JsonValue(int n): m(n) { }
    JsonValue(qint64 n): m(n) { }
    JsonValue(const QString &s): m(s) { }
    JsonValue(QLatin1String s): m(s) { }
    JsonValue(const QColor &c): m(c.name(QColor::HexArgb)) { }
    JsonValue(const QFont &f): m(f.toString()) { }
    JsonValue(const QPointF &p)
    { QJsonObject json; json["x"] = p.x(); json["y"] = p.y(); m = json; }
    JsonValue(const QPoint &p)
    { QJsonObject json; json["x"] = p.x(); json["y"] = p.y(); m = json; }
    JsonValue(const QJsonArray &a): m(a) { }
    JsonValue(const QJsonObject &o): m(o) { }
    JsonValue(const JsonObject &o);
    template<class T, typename std::enable_if<std::is_enum_class<T>::value, int>::type = 0>
    JsonValue(T t): m(_EnumName<T>(t)) { }

    auto type() const -> Type { return (Type)m.type(); }
    auto isNull() const -> bool { return type() == Null; }
    auto isBool() const -> bool { return type() == Bool; }
    auto isDouble() const -> bool { return type() == Double; }
    auto isString() const -> bool { return type() == String; }
    auto isArray() const -> bool { return type() == Array; }
    auto isObject() const -> bool { return type() == Object; }
    auto isUndefined() const -> bool { return type() == Undefined; }

    auto toBool(bool def = false) const -> bool { return m.toBool(def); }
    auto toInt(int def = 0) const -> int { return m.toInt(def); }
    auto toDouble(double def = 0.0) const -> double { return m.toDouble(def); }
    auto toString(const QString &def = QString()) const -> QString
        { return m.toString(def); }
    auto toPoint(const QPoint &def = QPoint()) const -> QPoint
    {
        const auto json = m.toObject();
        return QPoint(json.value("x").toInt(def.x()),
                      json.value("y").toInt(def.y()));
    }
    auto toPointF(const QPointF &def = QPointF()) const -> QPointF
    {
        const auto json = m.toObject();
        return QPointF(json.value("x").toDouble(def.x()),
                       json.value("y").toDouble(def.y()));
    }
    auto toColor(const QColor &def = QColor()) const -> QColor
        { return QColor(m.toString(def.name(QColor::HexArgb))); }
    auto toFont(const QFont &def = QFont()) const -> QFont
        { QFont font; return font.fromString(m.toString()) ? font : def; }
    template<class T, typename std::enable_if<std::is_enum_class<T>::value, int>::type = 0>
    auto toEnum(const T &def = EnumInfo<T>::default_()) -> T
        { return _EnumFrom(m.toString(), def); }
    auto toArray() const -> QJsonArray { return m.toArray(); }
    auto toArray(const QJsonArray &def) const -> QJsonArray
        { return m.toArray(def); }
    auto toObject() const -> JsonObject;
    auto toObject(const JsonObject &def) const -> JsonObject;

    auto operator==(const JsonValue &rhs) const -> bool { return m == rhs.m; }
    auto operator!=(const JsonValue &rhs) const -> bool { return m != rhs.m; }
    auto qt() const -> const QJsonValue& { return m; }
private:
    QJsonValue m;
};

class JsonValueRef
{
public:
    JsonValueRef(const QJsonValueRef &ref): m(ref) { }
    JsonValueRef(QJsonArray *array, int idx): m(array, idx) { }
    JsonValueRef(QJsonObject *object, int idx): m(object, idx) { }

    operator JsonValue() const { return toValue(); }
    auto operator = (const JsonValue &val) -> JsonValueRef&
        { m = val.qt(); return *this; }
    auto operator = (const JsonValueRef &val) -> JsonValueRef&
        { m = val.m; return *this; }
    auto type() const -> JsonValue::Type
        { return (JsonValue::Type)m.type(); }
    auto isNull() const -> bool { return type() == JsonValue::Null; }
    auto isBool() const -> bool { return type() == JsonValue::Bool; }
    auto isDouble() const -> bool { return type() == JsonValue::Double; }
    auto isString() const -> bool { return type() == JsonValue::String; }
    auto isArray() const -> bool { return type() == JsonValue::Array; }
    auto isObject() const -> bool { return type() == JsonValue::Object; }
    auto isUndefined() const -> bool { return type() == JsonValue::Undefined; }

    auto toBool() const -> bool { return toValue().toBool(); }
    auto toInt() const -> int { return toValue().toInt(); }
    auto toDouble() const -> double { return toValue().toDouble(); }
    auto toString() const -> QString { return toValue().toString(); }
    auto toArray() const -> QJsonArray { return m.toArray(); }
    auto toObject() const -> QJsonObject { return m.toObject(); }

    auto operator==(const JsonValue &rhs) const -> bool { return toValue() == rhs; }
    auto operator!=(const JsonValue &rhs) const -> bool { return toValue() != rhs; }
private:
    auto toValue() const -> JsonValue { return QJsonValue(m); }
    QJsonValueRef m;
};

namespace detail {

template<class T, bool integral = std::is_integral<T>::value, bool floating = std::is_floating_point<T>::value, bool is_enum = std::is_enum_class<T>::value>
struct json_type;

#define SCA static constexpr auto
template<>
struct json_type<bool> {
    SCA to_func = &JsonValue::toBool;
};
template<class T>
struct json_type<T, true, false, false> {
    SCA to_func = &JsonValue::toInt;
};
template<class T>
struct json_type<T, false, true, false> {
    SCA to_func = &JsonValue::toDouble;
};
template<>
struct json_type<qint64> : json_type<double> { };
template<>
struct json_type<quint64> : json_type<double> { };
template<>
struct json_type<QString> {
    SCA to_func = &JsonValue::toString;
};
template<>
struct json_type<QPoint> {
    SCA to_func = &JsonValue::toPoint;
};
template<>
struct json_type<QPointF> {
    SCA to_func = &JsonValue::toPointF;
};
template<>
struct json_type<QFont> {
    SCA to_func = &JsonValue::toFont;
};
template<>
struct json_type<QColor> {
    SCA to_func = &JsonValue::toColor;
};
template<class T>
struct json_type<T, false, false, true> {
    SCA to_func = &JsonValue::toEnum<T>;
};
#undef SCA
}

class JsonObject {
public:
    using const_iterator = QJsonObject::const_iterator;
    using iterator = QJsonObject::iterator;
    using mapped_type = QJsonValue;
    using key_type = QString;
    using size_type = int;

    JsonObject() = default;
    JsonObject(const QJsonObject &json): m(json) { }

    auto operator==(const JsonObject &rhs) const -> bool { return m == rhs.m; }
    auto operator!=(const JsonObject &rhs) const -> bool { return m != rhs.m; }
    static auto fromVariantMap(const QVariantMap &map) -> JsonObject
    {
        JsonObject json;
        json.m = QJsonObject::fromVariantMap(map);
        return json;
    }
    auto toVariantMap() const -> QVariantMap { return m.toVariantMap(); }
    auto keys() const -> QStringList { return m.keys(); }
    auto size() const -> int { return m.size(); }
    auto isEmpty() const -> bool { return m.isEmpty(); }
    auto empty() const -> bool { return m.isEmpty(); }

    auto value(const QString &key) const -> JsonValue { return m.value(key); }
    auto operator[] (const QString &key) const -> JsonValue { return m.value(key); }
    auto operator[] (const QString &key) -> JsonValueRef { return m[key]; }
    auto value(const char *key) const -> JsonValue { return m.value(_L(key)); }
    auto operator[] (const char *key) const -> JsonValue { return m.value(_L(key)); }
    auto operator[] (const char *key) -> JsonValueRef { return m[_L(key)]; }

    auto remove(const QString &key) -> void { m.remove(key); }
    auto take(const QString &key) -> JsonValue { return m.take(key); }
    auto contains(const QString &key) const -> bool { return m.contains(key); }
    auto find(const QString &key) -> iterator { return m.find(key); }
    auto find(const QString &key) const -> const_iterator { return m.constFind(key); }
    auto cfind(const QString &key) const -> const_iterator { return m.constFind(key); }
    auto insert(const QString &key, const JsonValue &value) -> iterator { return m.insert(key, value.qt()); }

    // STL style
    auto begin() -> iterator { return m.begin(); }
    auto begin() const -> const_iterator { return m.begin(); }
    auto cbegin() const -> const_iterator { return m.constBegin(); }
    auto end() -> iterator { return m.end(); }
    auto end() const -> const_iterator { return m.end(); }
    auto cend() const -> const_iterator { return m.constEnd(); }
    auto erase(iterator it) -> iterator { return m.erase(it); }

    auto qt() const -> const QJsonObject& { return m; }
    template<class T>
    auto get(const char *key, T &value) const -> void
    { value = (this->value(key).*detail::json_type<T>::to_func)(value); }
private:
    QJsonObject m;
};

inline JsonValue::JsonValue(const JsonObject &o): m(o.qt()) { }

inline auto JsonValue::toObject() const -> JsonObject { return m.toObject(); }
inline auto JsonValue::toObject(const JsonObject &def) const -> JsonObject
    { return m.toObject(def.qt()); }

#endif // JSON_HPP
