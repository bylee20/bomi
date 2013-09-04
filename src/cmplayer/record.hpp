#ifndef RECORD_HPP
#define RECORD_HPP

#include "stdafx.hpp"
#include "colorproperty.hpp"
#include "deintinfo.hpp"
#include "enums.hpp"
#include <type_traits>

template <typename T> static inline T fromVariant(const QVariant &data) {return data.value<T>();}
template <typename T> static inline QVariant toVariant(const T &t) {return QVariant::fromValue(t);}

enum class RecordType : quint32 {Normal, Enum, List};

template <typename T>
struct RecordInfo {
	constexpr static RecordType type = std::is_enum<T>::value ? RecordType::Enum : RecordType::Normal;
	constexpr static bool builtIn = type == RecordType::Normal && (QMetaTypeId2<T>::Defined || QMetaTypeId<T>::Defined);
};

template <>
struct RecordInfo<QStringList> {
	constexpr static RecordType type = RecordType::List;
	constexpr static bool builtIn = true;
};

template <typename T>
struct RecordInfo<QList<T> > {
	constexpr static bool builtIn = std::is_same<T, QVariant>::value;
	constexpr static RecordType type = builtIn ? RecordType::Normal : RecordType::List;

};

template <typename T, RecordType type = RecordInfo<T>::type, bool builtIn = RecordInfo<T>::builtIn>
struct RecordIo {
	static_assert(builtIn, "Wrong specialization!");
	static void read(QSettings &r, T &value, const char *key) {value = fromVariant<T>(r.value(_L(key), toVariant<T>(value)));}
	static void write(QSettings &r, const T &value, const char *key) {r.setValue(_L(key), toVariant<T>(value));}
};

template <typename T>
struct RecordIo<T, RecordType::Enum, false> {
	static void read(QSettings &r, T &value, const char *key) {value = EnumInfo<T>::from(r.value(_L(key), EnumInfo<T>::name(value)).toString(), value);}
	static void write(QSettings &r, const T &value, const char *key) {r.setValue(_L(key), EnumInfo<T>::name(value));}
};

template<>
struct RecordIo<DeintInfo, RecordType::Normal, false> {
	static void read(QSettings &r, DeintInfo &value, const char *key) {
		value = DeintInfo::fromString(r.value(_L(key), value.toString()).toString());
	}
	static void write(QSettings &r, const DeintInfo &value, const char *key) {
		r.setValue(_L(key), value.toString());
	}
};

template <typename T>
struct RecordIo<QList<T>, RecordType::List, false> {
	static void read(QSettings &r, QList<T> &values, const char *key) {
		if (!r.value(_L(key) % _L("_exists"), false).toBool())
			return;
		const int size = r.beginReadArray(key);
		values.clear();	values.reserve(size);
		T t; for (int i = 0; i<size; ++i) {values.append(t);}
		for (int i=0; i<size; ++i) {
			r.setArrayIndex(i);
			RecordIo<T>::read(r, values[i], "data");
		}
		r.endArray();
	}
	static void write(QSettings &r, const QList<T> &values, const char *key) {
		r.setValue(_L(key) % _L("_exists"), true);
		r.beginWriteArray(key, values.size());
		for (int i=0; i<values.size(); ++i) {
			r.setArrayIndex(i);
			RecordIo<T>::write(r, values[i], "data");
		}
		r.endArray();
	}
};

template <>
struct RecordIo<ColorProperty, RecordType::Normal, false> {
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

class Record : public QSettings {
public:
	Record() {}
	Record(const QString &root): m_root(root) {if (!m_root.isEmpty()) beginGroup(root);}
	~Record() {if (!m_root.isEmpty()) endGroup();}
	template <typename T> void write(const T &value, const char *key) {RecordIo<T>::write(*this, value, key);}
	template <typename T> void read(T &value, const char *key) {RecordIo<T>::read(*this, value, key);}
private:
	const QString m_root = {};
};

inline QKeySequence fromVariant(const QVariant &data) {return QKeySequence::fromString(data.toString());}
inline QVariant toVariant(const QKeySequence &seq) {return seq.toString();}

#endif // RECORD_HPP
