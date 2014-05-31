#ifndef RECORD_HPP
#define RECORD_HPP

#include "tmp/type_test.hpp"

template<class T> class EnumInfo;

namespace detail {

template<class T>
class convertible_string {
    template<class S>
    static auto to(S* p) -> decltype(p->toString(), std::true_type());
    template<class>
    static auto to(...) -> std::false_type;
    template<class S>
    static auto from(S* p) -> decltype(S::fromString(QString()), std::true_type());
    template<class>
    static auto from(...) -> std::false_type;
public:
    static constexpr bool value = decltype(to<T>(0))::value
                                  && decltype(from<T>(0))::value;
};

template<class T>
class convertible_json {
    template<class S>
    static auto to(S* p) -> decltype(p->toJson(), std::true_type());
    template<class>
    static auto to(...) -> std::false_type;
    template<class S>
    static auto from(S* p) -> decltype(p->setFromJson(QJsonObject()), std::true_type());
    template<class>
    static auto from(...) -> std::false_type;
public:
    static constexpr bool value = decltype(to<T>(0))::value
                                  && decltype(from<T>(0))::value;
};

enum DataType { Variant, String, Json };

template<class T>
static constexpr auto data_type() -> DataType
{
    return (std::is_class<T>::value && convertible_string<T>::value) ? String
         : (std::is_class<T>::value && convertible_json<T>::value)   ? Json
                                                                     : Variant;
}

template<class T>
struct is_list : std::false_type {};
template<class T>
struct is_list<QList<T>> : std::true_type {};

template <class T, bool enum_ = tmp::is_enum_class<T>(),
                   DataType dataType = data_type<T>()>
struct RecordIoOne {};

template <class T>
struct RecordIoOne<T, false, Variant> {
    static_assert(!is_list<T>::value, "assert!");
    static auto read(QSettings &r, T &value, const QString &key) -> void
        { value = r.value(key, QVariant::fromValue<T>(value)).template value<T>(); }
    static auto write(QSettings &r, const T &value, const QString &key) -> void
        { r.setValue(key, QVariant::fromValue<T>(value)); }
    static auto default_() -> T { return T(); }
};

template <class T>
struct RecordIoOne<T, true, Variant> {
    static auto read(QSettings &r, T &v, const QString &key) -> void
        { v = _EnumFrom(r.value(key, _EnumName(v)).toString(), v); }
    static auto write(QSettings &r, const T &value, const QString &key) -> void
        { r.setValue(key, _EnumName(value)); }
    static auto default_() -> T { return EnumInfo<T>::items()[0].value; }
};

template <class T>
struct RecordIoOne<T, false, String> {
    static_assert(!is_list<T>::value, "assert!");
    static auto read(QSettings &r, T &value, const QString &key) -> void
        { value = T::fromString(r.value(key, value.toString()).toString()); }
    static auto write(QSettings &r, const T &value, const QString &key) -> void
        { r.setValue(key, value.toString()); }
    static auto default_() -> T { return T(); }
};

template <class T>
struct RecordIoOne<T, false, Json> {
    static_assert(!is_list<T>::value, "assert!");
    static auto read(QSettings &r, T &value, const QString &key) -> void
    {
        const auto doc = r.value(key, _JsonToString(value.toJson())).toString();
        value.setFromJson(_JsonFromString(doc));
    }
    static auto write(QSettings &r, const T &value, const QString &key) -> void
        { r.setValue(key, _JsonToString(value.toJson())); }
    static auto default_() -> T { return T(); }
};

template <class T>
struct RecordIoList {
    using One = RecordIoOne<T>;
    static auto read(QSettings &r, QList<T> &values, const QString &key) -> void
    {
        if (!r.value(key % "_exists"_a, false).toBool())
            return;
        const int size = r.beginReadArray(key);
        values.clear();    values.reserve(size);
        T t = One::default_(); for (int i = 0; i<size; ++i) {values.append(t);}
        for (int i=0; i<size; ++i) {
            r.setArrayIndex(i);
            One::read(r, values[i], "data");
        }
        r.endArray();
    }
    static auto write(QSettings &r, const QList<T> &values,
                      const QString &key) -> void
    {
        r.setValue(key % "_exists"_a, true);
        r.beginWriteArray(key, values.size());
        for (int i=0; i<values.size(); ++i) {
            r.setArrayIndex(i);
            One::write(r, values[i], "data");
        }
        r.endArray();
    }
};

}

class Record : public QSettings {
public:
//    Record() {}
    Record(const QString &root): m_root(root) {
        m_version = value("version", 0).toInt();
        if (!m_root.isEmpty()) beginGroup(root);
    }
    Record(const QString &root, int version)
        : m_root(root), m_version(version) {
        setValue("version", m_version);
        if (!m_root.isEmpty()) beginGroup(root);
    }
    ~Record() {
        if (!m_root.isEmpty()) endGroup();
    }
    auto version() const -> int { return m_version; }
    template <class T>
    auto write(const T &value, const QString &key) -> void
        { detail::RecordIoOne<T>::write(*this, value, key); }
    template <class T>
    auto read(T &value, const QString &key) -> void
        { detail::RecordIoOne<T>::read(*this, value, key); }
    template <class T> auto write(const QList<T> &value,
                                  const QString &key) -> void
        { detail::RecordIoList<T>::write(*this, value, key); }
    template <class T>
    auto read(QList<T> &value, const QString &key) -> void
        { detail::RecordIoList<T>::read(*this, value, key); }
    template <class T>
    auto write(const T &value, const char *key) -> void
        { write<T>(value, _L(key)); }
    template <class T>
    auto write(const QList<T> &value, const char *key) -> void
        { write<T>(value, _L(key)); }
    template <class T>
    auto read(T &value, const char *key) -> void { read<T>(value, _L(key)); }
    template <class T>
    auto read(QList<T> &value, const char *key) -> void
        { read<T>(value, _L(key)); }
private:
    const QString m_root = {};
    int m_version = 0;
};


#define RECORD_READ(a) { RECORD_VAR.read(a, #a); }
#define RECORD_WRITE(a) { RECORD_VAR.write(a, #a); }


#endif // RECORD_HPP
