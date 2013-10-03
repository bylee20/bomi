#ifndef RECORD_HPP
#define RECORD_HPP

#include "stdafx.hpp"
#include "colorproperty.hpp"
#include "deintinfo.hpp"
#include "enums.hpp"
#include <type_traits>

template <typename T> static inline T fromVariant(const QVariant &data) {return data.value<T>();}
template <typename T> static inline QVariant toVariant(const T &t) {return QVariant::fromValue(t);}
#define DEC_WITH_STRING(T) \
template<> inline T        fromVariant(const QVariant &data) { return T::fromString(data.toString()); }\
template<> inline QVariant toVariant(const T &t) { return t.toString(); }
DEC_WITH_STRING(DeintOption)
DEC_WITH_STRING(DeintCaps)
DEC_WITH_STRING(QKeySequence)
#undef DEC_WITH_STRING

template<class T> struct is_list : std::false_type {};
template<class T> struct is_list<QList<T>> : std::true_type {};

template <typename T, bool enum_ = std::is_enum<T>::value> struct RecordIoOne {};
template <typename T>
struct RecordIoOne<T, false> {
	static_assert(!is_list<T>::value, "assert!");
	static void read(QSettings &r, T &value, const char *key) {value = fromVariant<T>(r.value(_L(key), toVariant<T>(value)));}
	static void write(QSettings &r, const T &value, const char *key) {r.setValue(_L(key), toVariant<T>(value));}
};

template <typename T>
struct RecordIoOne<T, true> {
	static void read(QSettings &r, T &value, const char *key) {value = EnumInfo<T>::from(r.value(_L(key), EnumInfo<T>::name(value)).toString(), value);}
	static void write(QSettings &r, const T &value, const char *key) {r.setValue(_L(key), EnumInfo<T>::name(value));}
};

template <typename T>
struct RecordIoList {
	static void read(QSettings &r, QList<T> &values, const char *key) {
		if (!r.value(_L(key) % _L("_exists"), false).toBool())
			return;
		const int size = r.beginReadArray(key);
		values.clear();	values.reserve(size);
		T t; for (int i = 0; i<size; ++i) {values.append(t);}
		for (int i=0; i<size; ++i) {
			r.setArrayIndex(i);
			RecordIoOne<T>::read(r, values[i], "data");
		}
		r.endArray();
	}
	static void write(QSettings &r, const QList<T> &values, const char *key) {
		r.setValue(_L(key) % _L("_exists"), true);
		r.beginWriteArray(key, values.size());
		for (int i=0; i<values.size(); ++i) {
			r.setArrayIndex(i);
			RecordIoOne<T>::write(r, values[i], "data");
		}
		r.endArray();
	}
};

template <>
struct RecordIoOne<ColorProperty, false> {
	static void write(QSettings &r, const ColorProperty &value, const char *key) {
		r.setValue(_L(key) % _L("_brightness"), value.brightness());
		r.setValue(_L(key) % _L("_contrast"), value.contrast());
		r.setValue(_L(key) % _L("_saturation"), value.saturation());
		r.setValue(_L(key) % _L("_hue"), value.hue());
	}
	static void read(QSettings &r, ColorProperty &value, const char *key) {
		value.brightness() = r.value(_L(key) % _L("_brightness"), value.brightness()).toDouble();
		value.contrast() = r.value(_L(key) % _L("_contrast"), value.contrast()).toDouble();
		value.saturation() = r.value(_L(key) % _L("_saturation"), value.saturation()).toDouble();
		value.hue() = r.value(_L(key) % _L("_hue"), value.hue()).toDouble();
	}
};

extern template class RecordIoOne<int>;
extern template class RecordIoOne<bool>;
extern template class RecordIoOne<double>;
extern template class RecordIoOne<float>;
extern template class RecordIoOne<QString>;
extern template class RecordIoOne<QStringList>;
extern template class RecordIoOne<QVariant>;
extern template class RecordIoOne<QLocale>;

class Record : public QSettings {
public:
	Record() {}
	Record(const QString &root): m_root(root) {if (!m_root.isEmpty()) beginGroup(root);}
	~Record() {if (!m_root.isEmpty()) endGroup();}
	template <typename T> void write(const T &value, const char *key) {
		RecordIoOne<T>::write(*this, value, key);
	}
	template <typename T> void read(T &value, const char *key) {
		RecordIoOne<T>::read(*this, value, key);
	}
	template <typename T> void write(const QList<T> &value, const char *key) {
		RecordIoList<T>::write(*this, value, key);
	}
	template <typename T> void read(QList<T> &value, const char *key) {
		RecordIoList<T>::read(*this, value, key);
	}
private:
	const QString m_root = {};
};


#endif // RECORD_HPP
