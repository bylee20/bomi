#ifndef RECORD_HPP
#define RECORD_HPP

#include "stdafx.hpp"
#include "info.hpp"
#include "videocolor.hpp"
#include "deintinfo.hpp"
#include "channelmanipulation.hpp"
#include "enums.hpp"
#include "mrlstate.hpp"
#include <type_traits>

template <typename T> static inline T fromVariant(const QVariant &data) {return data.value<T>();}
template <typename T> static inline QVariant toVariant(const T &t) {return QVariant::fromValue(t);}
#define DEC_WITH_STRING(T) \
template<> inline T        fromVariant(const QVariant &data) { return T::fromString(data.toString()); }\
template<> inline QVariant toVariant(const T &t) { return t.toString(); }
DEC_WITH_STRING(DeintOption)
DEC_WITH_STRING(DeintCaps)
DEC_WITH_STRING(QKeySequence)
DEC_WITH_STRING(ChannelLayoutMap)
#undef DEC_WITH_STRING

template<class T> struct is_list : std::false_type {};
template<class T> struct is_list<QList<T>> : std::true_type {};

template <typename T, bool enum_ = std::is_enum<T>::value> struct RecordIoOne {};
template <typename T>
struct RecordIoOne<T, false> {
	static_assert(!is_list<T>::value, "assert!");
	static void read(QSettings &r, T &value, const QString &key) {value = fromVariant<T>(r.value(key, toVariant<T>(value)));}
	static void write(QSettings &r, const T &value, const QString &key) {r.setValue(key, toVariant<T>(value));}
	static T default_() { return T(); }
};

template <typename T>
struct RecordIoOne<T, true> {
	static void read(QSettings &r, T &value, const QString &key) {value = EnumInfo<T>::from(r.value(key, EnumInfo<T>::name(value)).toString(), value);}
	static void write(QSettings &r, const T &value, const QString &key) {r.setValue(key, EnumInfo<T>::name(value));}
	static T default_() { return EnumInfo<T>::items()[0].value; }
};

template <typename T>
struct RecordIoList {
	using One = RecordIoOne<T>;
	static void read(QSettings &r, QList<T> &values, const QString &key) {
		if (!r.value(key % _L("_exists"), false).toBool())
			return;
		const int size = r.beginReadArray(key);
		values.clear();	values.reserve(size);
		T t = One::default_(); for (int i = 0; i<size; ++i) {values.append(t);}
		for (int i=0; i<size; ++i) {
			r.setArrayIndex(i);
			One::read(r, values[i], "data");
		}
		r.endArray();
	}
	static void write(QSettings &r, const QList<T> &values, const QString &key) {
		r.setValue(key % _L("_exists"), true);
		r.beginWriteArray(key, values.size());
		for (int i=0; i<values.size(); ++i) {
			r.setArrayIndex(i);
			One::write(r, values[i], "data");
		}
		r.endArray();
	}
};

template <>
struct RecordIoOne<VideoColor, false> {
	static void write(QSettings &r, const VideoColor &value, const QString &key) {
		r.setValue(key % _L("_brightness"), value.brightness());
		r.setValue(key % _L("_contrast"), value.contrast());
		r.setValue(key % _L("_saturation"), value.saturation());
		r.setValue(key % _L("_hue"), value.hue());
	}
	static void read(QSettings &r, VideoColor &value, const QString &key) {
		value.brightness() = r.value(key % _L("_brightness"), value.brightness()).toDouble();
		value.contrast() = r.value(key % _L("_contrast"), value.contrast()).toDouble();
		value.saturation() = r.value(key % _L("_saturation"), value.saturation()).toDouble();
		value.hue() = r.value(key % _L("_hue"), value.hue()).toDouble();
	}
};

extern template struct RecordIoOne<int>;
extern template struct RecordIoOne<bool>;
extern template struct RecordIoOne<double>;
extern template struct RecordIoOne<float>;
extern template struct RecordIoOne<QString>;
extern template struct RecordIoOne<QStringList>;
extern template struct RecordIoOne<QVariant>;
extern template struct RecordIoOne<QLocale>;
extern template struct RecordIoOne<QKeySequence>;

class Record : public QSettings {
public:
//	Record() {}
	Record(const QString &root): m_root(root) {
		m_version = value("version", 0).toInt();
		if (!m_root.isEmpty()) beginGroup(root);
	}
	~Record() {
		if (!m_root.isEmpty()) endGroup();
		setValue("version", Info::versionNumber());
	}
	int version() const { return m_version; }
	template <typename T> void write(const T &value, const QString &key) {
		RecordIoOne<T>::write(*this, value, key);
	}
	template <typename T> void read(T &value, const QString &key) {
		RecordIoOne<T>::read(*this, value, key);
	}
	template <typename T> void write(const QList<T> &value, const QString &key) {
		RecordIoList<T>::write(*this, value, key);
	}
	template <typename T> void read(QList<T> &value, const QString &key) {
		RecordIoList<T>::read(*this, value, key);
	}
	template <typename T> void write(const T &value, const char *key) { write<T>(value, _L(key)); }
	template <typename T> void write(const QList<T> &value, const char *key) { write<T>(value, _L(key)); }
	template <typename T> void read(T &value, const char *key) { read<T>(value, _L(key)); }
	template <typename T> void read(QList<T> &value, const char *key) { read<T>(value, _L(key)); }
private:
	const QString m_root = {};
	int m_version = 0;
};


#endif // RECORD_HPP
