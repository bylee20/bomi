#ifndef RECORD_HPP
#define RECORD_HPP

#include "stdafx.hpp"
#include "info.hpp"
#include "video/videocolor.hpp"
#include "video/deintinfo.hpp"
#include "audio/channelmanipulation.hpp"
#include "mrlstate.hpp"
#include <type_traits>

namespace detail {

template <class T>
SIA fromVariant(const QVariant &data) -> T {return data.value<T>();}
template <class T>
SIA toVariant(const T &t) -> QVariant {return QVariant::fromValue(t);}
#define DEC_WITH_STRING(T) \
template<> \
inline auto fromVariant(const QVariant &data) -> T \
    { return T::fromString(data.toString()); } \
template<> \
inline auto toVariant(const T &t) -> QVariant { return t.toString(); }
DEC_WITH_STRING(DeintOption)
DEC_WITH_STRING(DeintCaps)
DEC_WITH_STRING(QKeySequence)
DEC_WITH_STRING(ChannelLayoutMap)
#undef DEC_WITH_STRING

template<class T>
struct is_list : std::false_type {};
template<class T>
struct is_list<QList<T>> : std::true_type {};

template <class T, bool enum_ = std::is_enum<T>::value>
struct RecordIoOne {};

template <class T>
struct RecordIoOne<T, false> {
    static_assert(!is_list<T>::value, "assert!");
    static auto read(QSettings &r, T &value, const QString &key) -> void
        { value = fromVariant<T>(r.value(key, toVariant<T>(value))); }
    static auto write(QSettings &r, const T &value, const QString &key) -> void
        { r.setValue(key, toVariant<T>(value)); }
    static auto default_() -> T { return T(); }
};

template <class T>
struct RecordIoOne<T, true> {
    static auto read(QSettings &r, T &v, const QString &key) -> void
        { v = EnumInfo<T>::from(r.value(key, EnumInfo<T>::name(v)).toString(), v); }
    static auto write(QSettings &r, const T &value, const QString &key) -> void
        { r.setValue(key, EnumInfo<T>::name(value)); }
    static auto default_() -> T { return EnumInfo<T>::items()[0].value; }
};

template <>
struct RecordIoOne<VideoColor, false> {
    static auto write(QSettings &r, const VideoColor &v,
                      const QString &key) -> void {
        VideoColor::for_type([&] (VideoColor::Type type) {
            r.setValue(key % _L('_') % VideoColor::name(type), v.get(type));
        });
    }
    static auto read(QSettings &r, VideoColor &v,
                     const QString &k) -> void {
        VideoColor::for_type([&] (VideoColor::Type type) {
            v.set(type, r.value(k % _L('_') % VideoColor::name(type)).toInt());
        });
    }
};

template <class T>
struct RecordIoList {
    using One = RecordIoOne<T>;
    static auto read(QSettings &r, QList<T> &values, const QString &key) -> void
    {
        if (!r.value(key % _L("_exists"), false).toBool())
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
        r.setValue(key % _L("_exists"), true);
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
    ~Record() {
        if (!m_root.isEmpty()) endGroup();
        setValue("version", Info::versionNumber());
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


#endif // RECORD_HPP
