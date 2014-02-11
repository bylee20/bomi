
#ifndef ENUMS_HPP
#define ENUMS_HPP

#include <QCoreApplication>
#include <array>
#include "videocolor.hpp"
extern "C" {
#include <audio/chmap.h>
}

template<typename T> class EnumInfo { static constexpr int size() { return 0; } double dummy; };

typedef QString (*EnumVariantToSqlFunc)(const QVariant &var);
typedef QVariant (*EnumVariantFromSqlFunc)(const QVariant &var, const QVariant &def);

template<typename T>
QString _EnumVariantToSql(const QVariant &var) {
	Q_ASSERT(var.userType() == qMetaTypeId<T>());
	return QLatin1Char('\'') % EnumInfo<T>::name(var.value<T>()) % QLatin1Char('\'');
}

template<typename T>
QVariant _EnumVariantFromSql(const QVariant &name, const QVariant &def) {
	const auto enum_ = EnumInfo<T>::from(name.toString(), def.value<T>());
	return QVariant::fromValue<T>(enum_);
}


enum class SpeakerId : int {
	FrontLeft = (int)(1 << 0),
	FrontRight = (int)(1 << 1),
	FrontCenter = (int)(1 << 2),
	LowFrequency = (int)(1 << 3),
	BackLeft = (int)(1 << 4),
	BackRight = (int)(1 << 5),
	FrontLeftCenter = (int)(1 << 6),
	FrontRightCenter = (int)(1 << 7),
	BackCenter = (int)(1 << 8),
	SideLeft = (int)(1 << 9),
	SideRight = (int)(1 << 10)
};

inline bool operator == (SpeakerId e, int i) { return (int)e == i; }
inline bool operator != (SpeakerId e, int i) { return (int)e != i; }
inline bool operator == (int i, SpeakerId e) { return (int)e == i; }
inline bool operator != (int i, SpeakerId e) { return (int)e != i; }
inline int operator & (SpeakerId e, int i) { return (int)e & i; }
inline int operator & (int i, SpeakerId e) { return (int)e & i; }
inline int &operator &= (int &i, SpeakerId e) { return i &= (int)e; }
inline int operator ~ (SpeakerId e) { return ~(int)e; }
inline int operator | (SpeakerId e, int i) { return (int)e | i; }
inline int operator | (int i, SpeakerId e) { return (int)e | i; }
constexpr inline int operator | (SpeakerId e1, SpeakerId e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, SpeakerId e) { return i |= (int)e; }
inline bool operator > (SpeakerId e, int i) { return (int)e > i; }
inline bool operator < (SpeakerId e, int i) { return (int)e < i; }
inline bool operator >= (SpeakerId e, int i) { return (int)e >= i; }
inline bool operator <= (SpeakerId e, int i) { return (int)e <= i; }
inline bool operator > (int i, SpeakerId e) { return i > (int)e; }
inline bool operator < (int i, SpeakerId e) { return i < (int)e; }
inline bool operator >= (int i, SpeakerId e) { return i >= (int)e; }
inline bool operator <= (int i, SpeakerId e) { return i <= (int)e; }

Q_DECLARE_METATYPE(SpeakerId)

template<>
class EnumInfo<SpeakerId> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef SpeakerId Enum;
public:
    typedef SpeakerId type;
    using Data =  mp_speaker_id;
    struct Item { Enum value; QString name, key; mp_speaker_id data; };
	static constexpr int size() { return 11; }
    static constexpr const char *typeName() { return "SpeakerId"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        auto it = std::find_if(info.cbegin(), info.cend(), [e](const Item &info) { return info.value == e; }); return it != info.cend() ? &(*it) : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static mp_speaker_id data(Enum e) { auto i = item(e); return i ? i->data : mp_speaker_id(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::FrontLeft: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::FrontRight: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::FrontCenter: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::LowFrequency: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::BackLeft: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::BackRight: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::FrontLeftCenter: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::FrontRightCenter: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::BackCenter: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::SideLeft: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::SideRight: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 11> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const mp_speaker_id &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return SpeakerId::FrontLeft; }
private:
	static const std::array<Item, 11> info;
};

using SpeakerIdInfo = EnumInfo<SpeakerId>;

enum class ChannelLayout : int {
	Default = (int)0,
	Mono = (int)((int)SpeakerId::FrontCenter),
	_2_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight),
	_2_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::LowFrequency),
	_3_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter),
	_3_0_Back = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::BackCenter),
	_3_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::LowFrequency),
	_4_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight),
	_4_0_Side = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
	_4_0_Diamond = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter),
	_4_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::LowFrequency),
	_4_1_Diamond = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter|(int)SpeakerId::LowFrequency),
	_5_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight),
	_5_0_Side = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
	_5_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::LowFrequency),
	_5_1_Side = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight|(int)SpeakerId::LowFrequency),
	_6_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
	_6_0_Front = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontLeftCenter|(int)SpeakerId::FrontRightCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
	_6_0_Hex = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight),
	_6_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight|(int)SpeakerId::LowFrequency),
	_6_1_Hex = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::LowFrequency),
	_6_1_Front = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontLeftCenter|(int)SpeakerId::FrontRightCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight|(int)SpeakerId::LowFrequency),
	_7_0 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
	_7_0_Front = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::FrontLeftCenter|(int)SpeakerId::FrontRightCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight),
	_7_1 = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight|(int)SpeakerId::LowFrequency),
	_7_1_Wide = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::FrontLeftCenter|(int)SpeakerId::FrontRightCenter|(int)SpeakerId::BackLeft|(int)SpeakerId::BackRight|(int)SpeakerId::LowFrequency),
	_7_1_Side = (int)((int)SpeakerId::FrontLeft|(int)SpeakerId::FrontRight|(int)SpeakerId::FrontCenter|(int)SpeakerId::FrontLeftCenter|(int)SpeakerId::FrontRightCenter|(int)SpeakerId::SideLeft|(int)SpeakerId::SideRight|(int)SpeakerId::LowFrequency)
};

inline bool operator == (ChannelLayout e, int i) { return (int)e == i; }
inline bool operator != (ChannelLayout e, int i) { return (int)e != i; }
inline bool operator == (int i, ChannelLayout e) { return (int)e == i; }
inline bool operator != (int i, ChannelLayout e) { return (int)e != i; }
inline int operator & (ChannelLayout e, int i) { return (int)e & i; }
inline int operator & (int i, ChannelLayout e) { return (int)e & i; }
inline int &operator &= (int &i, ChannelLayout e) { return i &= (int)e; }
inline int operator ~ (ChannelLayout e) { return ~(int)e; }
inline int operator | (ChannelLayout e, int i) { return (int)e | i; }
inline int operator | (int i, ChannelLayout e) { return (int)e | i; }
constexpr inline int operator | (ChannelLayout e1, ChannelLayout e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, ChannelLayout e) { return i |= (int)e; }
inline bool operator > (ChannelLayout e, int i) { return (int)e > i; }
inline bool operator < (ChannelLayout e, int i) { return (int)e < i; }
inline bool operator >= (ChannelLayout e, int i) { return (int)e >= i; }
inline bool operator <= (ChannelLayout e, int i) { return (int)e <= i; }
inline bool operator > (int i, ChannelLayout e) { return i > (int)e; }
inline bool operator < (int i, ChannelLayout e) { return i < (int)e; }
inline bool operator >= (int i, ChannelLayout e) { return i >= (int)e; }
inline bool operator <= (int i, ChannelLayout e) { return i <= (int)e; }

Q_DECLARE_METATYPE(ChannelLayout)

template<>
class EnumInfo<ChannelLayout> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef ChannelLayout Enum;
public:
    typedef ChannelLayout type;
    using Data =  QByteArray;
    struct Item { Enum value; QString name, key; QByteArray data; };
	static constexpr int size() { return 27; }
    static constexpr const char *typeName() { return "ChannelLayout"; }
    static constexpr const char *typeKey() { return "channel"; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "Channel Layout")); }
    static const Item *item(Enum e) {
        auto it = std::find_if(info.cbegin(), info.cend(), [e](const Item &info) { return info.value == e; }); return it != info.cend() ? &(*it) : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QByteArray data(Enum e) { auto i = item(e); return i ? i->data : QByteArray(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Default: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Default"));
		case Enum::Mono: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Mono"));
		case Enum::_2_0: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Stereo"));
		case Enum::_2_1: return tr(QT_TRANSLATE_NOOP("EnumInfo", "2.1ch"));
		case Enum::_3_0: return tr(QT_TRANSLATE_NOOP("EnumInfo", "3.0ch"));
		case Enum::_3_0_Back: return tr(QT_TRANSLATE_NOOP("EnumInfo", "3.0ch(Back)"));
		case Enum::_3_1: return tr(QT_TRANSLATE_NOOP("EnumInfo", "3.1ch"));
		case Enum::_4_0: return tr(QT_TRANSLATE_NOOP("EnumInfo", "4.0ch"));
		case Enum::_4_0_Side: return tr(QT_TRANSLATE_NOOP("EnumInfo", "4.0ch(Side)"));
		case Enum::_4_0_Diamond: return tr(QT_TRANSLATE_NOOP("EnumInfo", "4.0ch(Diamond)"));
		case Enum::_4_1: return tr(QT_TRANSLATE_NOOP("EnumInfo", "4.1ch"));
		case Enum::_4_1_Diamond: return tr(QT_TRANSLATE_NOOP("EnumInfo", "4.1ch(Diamond)"));
		case Enum::_5_0: return tr(QT_TRANSLATE_NOOP("EnumInfo", "5.0ch"));
		case Enum::_5_0_Side: return tr(QT_TRANSLATE_NOOP("EnumInfo", "5.0ch(Side)"));
		case Enum::_5_1: return tr(QT_TRANSLATE_NOOP("EnumInfo", "5.1ch"));
		case Enum::_5_1_Side: return tr(QT_TRANSLATE_NOOP("EnumInfo", "5.1ch(Side)"));
		case Enum::_6_0: return tr(QT_TRANSLATE_NOOP("EnumInfo", "6.0ch"));
		case Enum::_6_0_Front: return tr(QT_TRANSLATE_NOOP("EnumInfo", "6.0ch(Front)"));
		case Enum::_6_0_Hex: return tr(QT_TRANSLATE_NOOP("EnumInfo", "6.0ch(Hexagonal)"));
		case Enum::_6_1: return tr(QT_TRANSLATE_NOOP("EnumInfo", "6.1ch"));
		case Enum::_6_1_Hex: return tr(QT_TRANSLATE_NOOP("EnumInfo", "6.1ch(Back)"));
		case Enum::_6_1_Front: return tr(QT_TRANSLATE_NOOP("EnumInfo", "6.1ch(Front)"));
		case Enum::_7_0: return tr(QT_TRANSLATE_NOOP("EnumInfo", "7.0ch"));
		case Enum::_7_0_Front: return tr(QT_TRANSLATE_NOOP("EnumInfo", "7.0ch(Front)"));
		case Enum::_7_1: return tr(QT_TRANSLATE_NOOP("EnumInfo", "7.1ch"));
		case Enum::_7_1_Wide: return tr(QT_TRANSLATE_NOOP("EnumInfo", "7.1ch(Wide)"));
		case Enum::_7_1_Side: return tr(QT_TRANSLATE_NOOP("EnumInfo", "7.1ch(Side)"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 27> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QByteArray &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return ChannelLayout::Default; }
private:
	static const std::array<Item, 27> info;
};

using ChannelLayoutInfo = EnumInfo<ChannelLayout>;

enum class ColorRange : int {
	Auto = (int)0,
	Limited = (int)1,
	Full = (int)2,
	Remap = (int)3,
	Extended = (int)4
};

inline bool operator == (ColorRange e, int i) { return (int)e == i; }
inline bool operator != (ColorRange e, int i) { return (int)e != i; }
inline bool operator == (int i, ColorRange e) { return (int)e == i; }
inline bool operator != (int i, ColorRange e) { return (int)e != i; }
inline int operator & (ColorRange e, int i) { return (int)e & i; }
inline int operator & (int i, ColorRange e) { return (int)e & i; }
inline int &operator &= (int &i, ColorRange e) { return i &= (int)e; }
inline int operator ~ (ColorRange e) { return ~(int)e; }
inline int operator | (ColorRange e, int i) { return (int)e | i; }
inline int operator | (int i, ColorRange e) { return (int)e | i; }
constexpr inline int operator | (ColorRange e1, ColorRange e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, ColorRange e) { return i |= (int)e; }
inline bool operator > (ColorRange e, int i) { return (int)e > i; }
inline bool operator < (ColorRange e, int i) { return (int)e < i; }
inline bool operator >= (ColorRange e, int i) { return (int)e >= i; }
inline bool operator <= (ColorRange e, int i) { return (int)e <= i; }
inline bool operator > (int i, ColorRange e) { return i > (int)e; }
inline bool operator < (int i, ColorRange e) { return i < (int)e; }
inline bool operator >= (int i, ColorRange e) { return i >= (int)e; }
inline bool operator <= (int i, ColorRange e) { return i <= (int)e; }

Q_DECLARE_METATYPE(ColorRange)

template<>
class EnumInfo<ColorRange> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef ColorRange Enum;
public:
    typedef ColorRange type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 5; }
    static constexpr const char *typeName() { return "ColorRange"; }
    static constexpr const char *typeKey() { return "range"; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "Color Range")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Auto: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Auto"));
		case Enum::Limited: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Limited Range"));
		case Enum::Full: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Full Range"));
		case Enum::Remap: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Remap Range"));
		case Enum::Extended: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Extended Range"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 5> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return ColorRange::Auto; }
private:
	static const std::array<Item, 5> info;
};

using ColorRangeInfo = EnumInfo<ColorRange>;

enum class AdjustColor : int {
	Reset = (int)0,
	BrightnessInc = (int)1,
	BrightnessDec = (int)2,
	ContrastInc = (int)3,
	ContrastDec = (int)4,
	SaturationInc = (int)5,
	SaturationDec = (int)6,
	HueInc = (int)7,
	HueDec = (int)8
};

inline bool operator == (AdjustColor e, int i) { return (int)e == i; }
inline bool operator != (AdjustColor e, int i) { return (int)e != i; }
inline bool operator == (int i, AdjustColor e) { return (int)e == i; }
inline bool operator != (int i, AdjustColor e) { return (int)e != i; }
inline int operator & (AdjustColor e, int i) { return (int)e & i; }
inline int operator & (int i, AdjustColor e) { return (int)e & i; }
inline int &operator &= (int &i, AdjustColor e) { return i &= (int)e; }
inline int operator ~ (AdjustColor e) { return ~(int)e; }
inline int operator | (AdjustColor e, int i) { return (int)e | i; }
inline int operator | (int i, AdjustColor e) { return (int)e | i; }
constexpr inline int operator | (AdjustColor e1, AdjustColor e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, AdjustColor e) { return i |= (int)e; }
inline bool operator > (AdjustColor e, int i) { return (int)e > i; }
inline bool operator < (AdjustColor e, int i) { return (int)e < i; }
inline bool operator >= (AdjustColor e, int i) { return (int)e >= i; }
inline bool operator <= (AdjustColor e, int i) { return (int)e <= i; }
inline bool operator > (int i, AdjustColor e) { return i > (int)e; }
inline bool operator < (int i, AdjustColor e) { return i < (int)e; }
inline bool operator >= (int i, AdjustColor e) { return i >= (int)e; }
inline bool operator <= (int i, AdjustColor e) { return i <= (int)e; }

Q_DECLARE_METATYPE(AdjustColor)

template<>
class EnumInfo<AdjustColor> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef AdjustColor Enum;
public:
    typedef AdjustColor type;
    using Data =  VideoColor;
    struct Item { Enum value; QString name, key; VideoColor data; };
	static constexpr int size() { return 9; }
    static constexpr const char *typeName() { return "AdjustColor"; }
    static constexpr const char *typeKey() { return "color"; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static VideoColor data(Enum e) { auto i = item(e); return i ? i->data : VideoColor(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Reset: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::BrightnessInc: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::BrightnessDec: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::ContrastInc: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::ContrastDec: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::SaturationInc: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::SaturationDec: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::HueInc: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::HueDec: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 9> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const VideoColor &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return AdjustColor::Reset; }
private:
	static const std::array<Item, 9> info;
};

using AdjustColorInfo = EnumInfo<AdjustColor>;

enum class SubtitleDisplay : int {
	OnLetterbox = (int)0,
	InVideo = (int)1
};

inline bool operator == (SubtitleDisplay e, int i) { return (int)e == i; }
inline bool operator != (SubtitleDisplay e, int i) { return (int)e != i; }
inline bool operator == (int i, SubtitleDisplay e) { return (int)e == i; }
inline bool operator != (int i, SubtitleDisplay e) { return (int)e != i; }
inline int operator & (SubtitleDisplay e, int i) { return (int)e & i; }
inline int operator & (int i, SubtitleDisplay e) { return (int)e & i; }
inline int &operator &= (int &i, SubtitleDisplay e) { return i &= (int)e; }
inline int operator ~ (SubtitleDisplay e) { return ~(int)e; }
inline int operator | (SubtitleDisplay e, int i) { return (int)e | i; }
inline int operator | (int i, SubtitleDisplay e) { return (int)e | i; }
constexpr inline int operator | (SubtitleDisplay e1, SubtitleDisplay e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, SubtitleDisplay e) { return i |= (int)e; }
inline bool operator > (SubtitleDisplay e, int i) { return (int)e > i; }
inline bool operator < (SubtitleDisplay e, int i) { return (int)e < i; }
inline bool operator >= (SubtitleDisplay e, int i) { return (int)e >= i; }
inline bool operator <= (SubtitleDisplay e, int i) { return (int)e <= i; }
inline bool operator > (int i, SubtitleDisplay e) { return i > (int)e; }
inline bool operator < (int i, SubtitleDisplay e) { return i < (int)e; }
inline bool operator >= (int i, SubtitleDisplay e) { return i >= (int)e; }
inline bool operator <= (int i, SubtitleDisplay e) { return i <= (int)e; }

Q_DECLARE_METATYPE(SubtitleDisplay)

template<>
class EnumInfo<SubtitleDisplay> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef SubtitleDisplay Enum;
public:
    typedef SubtitleDisplay type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 2; }
    static constexpr const char *typeName() { return "SubtitleDisplay"; }
    static constexpr const char *typeKey() { return "display"; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "Subtitle Display")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::OnLetterbox: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Display on Letterbox"));
		case Enum::InVideo: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Display in Video"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 2> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return SubtitleDisplay::OnLetterbox; }
private:
	static const std::array<Item, 2> info;
};

using SubtitleDisplayInfo = EnumInfo<SubtitleDisplay>;

enum class VideoRatio : int {
	Source = (int)0,
	Window = (int)1,
	_4__3 = (int)2,
	_16__10 = (int)3,
	_16__9 = (int)4,
	_1_85__1 = (int)5,
	_2_35__1 = (int)6
};

inline bool operator == (VideoRatio e, int i) { return (int)e == i; }
inline bool operator != (VideoRatio e, int i) { return (int)e != i; }
inline bool operator == (int i, VideoRatio e) { return (int)e == i; }
inline bool operator != (int i, VideoRatio e) { return (int)e != i; }
inline int operator & (VideoRatio e, int i) { return (int)e & i; }
inline int operator & (int i, VideoRatio e) { return (int)e & i; }
inline int &operator &= (int &i, VideoRatio e) { return i &= (int)e; }
inline int operator ~ (VideoRatio e) { return ~(int)e; }
inline int operator | (VideoRatio e, int i) { return (int)e | i; }
inline int operator | (int i, VideoRatio e) { return (int)e | i; }
constexpr inline int operator | (VideoRatio e1, VideoRatio e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, VideoRatio e) { return i |= (int)e; }
inline bool operator > (VideoRatio e, int i) { return (int)e > i; }
inline bool operator < (VideoRatio e, int i) { return (int)e < i; }
inline bool operator >= (VideoRatio e, int i) { return (int)e >= i; }
inline bool operator <= (VideoRatio e, int i) { return (int)e <= i; }
inline bool operator > (int i, VideoRatio e) { return i > (int)e; }
inline bool operator < (int i, VideoRatio e) { return i < (int)e; }
inline bool operator >= (int i, VideoRatio e) { return i >= (int)e; }
inline bool operator <= (int i, VideoRatio e) { return i <= (int)e; }

Q_DECLARE_METATYPE(VideoRatio)

template<>
class EnumInfo<VideoRatio> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef VideoRatio Enum;
public:
    typedef VideoRatio type;
    using Data =  qreal;
    struct Item { Enum value; QString name, key; qreal data; };
	static constexpr int size() { return 7; }
    static constexpr const char *typeName() { return "VideoRatio"; }
    static constexpr const char *typeKey() { return "size"; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "Size")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static qreal data(Enum e) { auto i = item(e); return i ? i->data : qreal(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Source: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Same as Source"));
		case Enum::Window: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Same as Window"));
		case Enum::_4__3: return tr(QT_TRANSLATE_NOOP("EnumInfo", "4:3 (TV)"));
		case Enum::_16__10: return tr(QT_TRANSLATE_NOOP("EnumInfo", "16:10 (Wide Monitor)"));
		case Enum::_16__9: return tr(QT_TRANSLATE_NOOP("EnumInfo", "16:9 (HDTV)"));
		case Enum::_1_85__1: return tr(QT_TRANSLATE_NOOP("EnumInfo", "1.85:1 (Wide Vision)"));
		case Enum::_2_35__1: return tr(QT_TRANSLATE_NOOP("EnumInfo", "2.35:1 (CinemaScope)"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 7> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const qreal &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return VideoRatio::Source; }
private:
	static const std::array<Item, 7> info;
};

using VideoRatioInfo = EnumInfo<VideoRatio>;

enum class Dithering : int {
	None = (int)0,
	Fruit = (int)1,
	Ordered = (int)2
};

inline bool operator == (Dithering e, int i) { return (int)e == i; }
inline bool operator != (Dithering e, int i) { return (int)e != i; }
inline bool operator == (int i, Dithering e) { return (int)e == i; }
inline bool operator != (int i, Dithering e) { return (int)e != i; }
inline int operator & (Dithering e, int i) { return (int)e & i; }
inline int operator & (int i, Dithering e) { return (int)e & i; }
inline int &operator &= (int &i, Dithering e) { return i &= (int)e; }
inline int operator ~ (Dithering e) { return ~(int)e; }
inline int operator | (Dithering e, int i) { return (int)e | i; }
inline int operator | (int i, Dithering e) { return (int)e | i; }
constexpr inline int operator | (Dithering e1, Dithering e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, Dithering e) { return i |= (int)e; }
inline bool operator > (Dithering e, int i) { return (int)e > i; }
inline bool operator < (Dithering e, int i) { return (int)e < i; }
inline bool operator >= (Dithering e, int i) { return (int)e >= i; }
inline bool operator <= (Dithering e, int i) { return (int)e <= i; }
inline bool operator > (int i, Dithering e) { return i > (int)e; }
inline bool operator < (int i, Dithering e) { return i < (int)e; }
inline bool operator >= (int i, Dithering e) { return i >= (int)e; }
inline bool operator <= (int i, Dithering e) { return i <= (int)e; }

Q_DECLARE_METATYPE(Dithering)

template<>
class EnumInfo<Dithering> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef Dithering Enum;
public:
    typedef Dithering type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 3; }
    static constexpr const char *typeName() { return "Dithering"; }
    static constexpr const char *typeKey() { return "dithering"; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "Dithering")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::None: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Off"));
		case Enum::Fruit: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Random Dithering"));
		case Enum::Ordered: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Ordered Dithering"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return Dithering::Fruit; }
private:
	static const std::array<Item, 3> info;
};

using DitheringInfo = EnumInfo<Dithering>;

enum class DecoderDevice : int {
	None = (int)0,
	CPU = (int)1,
	GPU = (int)2
};

inline bool operator == (DecoderDevice e, int i) { return (int)e == i; }
inline bool operator != (DecoderDevice e, int i) { return (int)e != i; }
inline bool operator == (int i, DecoderDevice e) { return (int)e == i; }
inline bool operator != (int i, DecoderDevice e) { return (int)e != i; }
inline int operator & (DecoderDevice e, int i) { return (int)e & i; }
inline int operator & (int i, DecoderDevice e) { return (int)e & i; }
inline int &operator &= (int &i, DecoderDevice e) { return i &= (int)e; }
inline int operator ~ (DecoderDevice e) { return ~(int)e; }
inline int operator | (DecoderDevice e, int i) { return (int)e | i; }
inline int operator | (int i, DecoderDevice e) { return (int)e | i; }
constexpr inline int operator | (DecoderDevice e1, DecoderDevice e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, DecoderDevice e) { return i |= (int)e; }
inline bool operator > (DecoderDevice e, int i) { return (int)e > i; }
inline bool operator < (DecoderDevice e, int i) { return (int)e < i; }
inline bool operator >= (DecoderDevice e, int i) { return (int)e >= i; }
inline bool operator <= (DecoderDevice e, int i) { return (int)e <= i; }
inline bool operator > (int i, DecoderDevice e) { return i > (int)e; }
inline bool operator < (int i, DecoderDevice e) { return i < (int)e; }
inline bool operator >= (int i, DecoderDevice e) { return i >= (int)e; }
inline bool operator <= (int i, DecoderDevice e) { return i <= (int)e; }

Q_DECLARE_METATYPE(DecoderDevice)

template<>
class EnumInfo<DecoderDevice> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef DecoderDevice Enum;
public:
    typedef DecoderDevice type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 3; }
    static constexpr const char *typeName() { return "DecoderDevice"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        auto it = std::find_if(info.cbegin(), info.cend(), [e](const Item &info) { return info.value == e; }); return it != info.cend() ? &(*it) : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::None: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::CPU: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::GPU: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return DecoderDevice::None; }
private:
	static const std::array<Item, 3> info;
};

using DecoderDeviceInfo = EnumInfo<DecoderDevice>;

enum class DeintMode : int {
	None = (int)0,
	Auto = (int)1
};

inline bool operator == (DeintMode e, int i) { return (int)e == i; }
inline bool operator != (DeintMode e, int i) { return (int)e != i; }
inline bool operator == (int i, DeintMode e) { return (int)e == i; }
inline bool operator != (int i, DeintMode e) { return (int)e != i; }
inline int operator & (DeintMode e, int i) { return (int)e & i; }
inline int operator & (int i, DeintMode e) { return (int)e & i; }
inline int &operator &= (int &i, DeintMode e) { return i &= (int)e; }
inline int operator ~ (DeintMode e) { return ~(int)e; }
inline int operator | (DeintMode e, int i) { return (int)e | i; }
inline int operator | (int i, DeintMode e) { return (int)e | i; }
constexpr inline int operator | (DeintMode e1, DeintMode e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, DeintMode e) { return i |= (int)e; }
inline bool operator > (DeintMode e, int i) { return (int)e > i; }
inline bool operator < (DeintMode e, int i) { return (int)e < i; }
inline bool operator >= (DeintMode e, int i) { return (int)e >= i; }
inline bool operator <= (DeintMode e, int i) { return (int)e <= i; }
inline bool operator > (int i, DeintMode e) { return i > (int)e; }
inline bool operator < (int i, DeintMode e) { return i < (int)e; }
inline bool operator >= (int i, DeintMode e) { return i >= (int)e; }
inline bool operator <= (int i, DeintMode e) { return i <= (int)e; }

Q_DECLARE_METATYPE(DeintMode)

template<>
class EnumInfo<DeintMode> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef DeintMode Enum;
public:
    typedef DeintMode type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 2; }
    static constexpr const char *typeName() { return "DeintMode"; }
    static constexpr const char *typeKey() { return "deinterlacing"; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "Deinterlacing")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::None: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Off"));
		case Enum::Auto: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Auto"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 2> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return DeintMode::Auto; }
private:
	static const std::array<Item, 2> info;
};

using DeintModeInfo = EnumInfo<DeintMode>;

enum class DeintDevice : int {
	None = (int)0,
	CPU = (int)1,
	GPU = (int)2,
	OpenGL = (int)4
};

inline bool operator == (DeintDevice e, int i) { return (int)e == i; }
inline bool operator != (DeintDevice e, int i) { return (int)e != i; }
inline bool operator == (int i, DeintDevice e) { return (int)e == i; }
inline bool operator != (int i, DeintDevice e) { return (int)e != i; }
inline int operator & (DeintDevice e, int i) { return (int)e & i; }
inline int operator & (int i, DeintDevice e) { return (int)e & i; }
inline int &operator &= (int &i, DeintDevice e) { return i &= (int)e; }
inline int operator ~ (DeintDevice e) { return ~(int)e; }
inline int operator | (DeintDevice e, int i) { return (int)e | i; }
inline int operator | (int i, DeintDevice e) { return (int)e | i; }
constexpr inline int operator | (DeintDevice e1, DeintDevice e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, DeintDevice e) { return i |= (int)e; }
inline bool operator > (DeintDevice e, int i) { return (int)e > i; }
inline bool operator < (DeintDevice e, int i) { return (int)e < i; }
inline bool operator >= (DeintDevice e, int i) { return (int)e >= i; }
inline bool operator <= (DeintDevice e, int i) { return (int)e <= i; }
inline bool operator > (int i, DeintDevice e) { return i > (int)e; }
inline bool operator < (int i, DeintDevice e) { return i < (int)e; }
inline bool operator >= (int i, DeintDevice e) { return i >= (int)e; }
inline bool operator <= (int i, DeintDevice e) { return i <= (int)e; }

Q_DECLARE_METATYPE(DeintDevice)

template<>
class EnumInfo<DeintDevice> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef DeintDevice Enum;
public:
    typedef DeintDevice type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 4; }
    static constexpr const char *typeName() { return "DeintDevice"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        auto it = std::find_if(info.cbegin(), info.cend(), [e](const Item &info) { return info.value == e; }); return it != info.cend() ? &(*it) : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::None: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::CPU: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::GPU: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::OpenGL: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 4> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return DeintDevice::None; }
private:
	static const std::array<Item, 4> info;
};

using DeintDeviceInfo = EnumInfo<DeintDevice>;

enum class DeintMethod : int {
	None = (int)0,
	Bob = (int)1,
	LinearBob = (int)2,
	CubicBob = (int)3,
	Median = (int)4,
	LinearBlend = (int)5,
	Yadif = (int)6,
	MotionAdaptive = (int)7
};

inline bool operator == (DeintMethod e, int i) { return (int)e == i; }
inline bool operator != (DeintMethod e, int i) { return (int)e != i; }
inline bool operator == (int i, DeintMethod e) { return (int)e == i; }
inline bool operator != (int i, DeintMethod e) { return (int)e != i; }
inline int operator & (DeintMethod e, int i) { return (int)e & i; }
inline int operator & (int i, DeintMethod e) { return (int)e & i; }
inline int &operator &= (int &i, DeintMethod e) { return i &= (int)e; }
inline int operator ~ (DeintMethod e) { return ~(int)e; }
inline int operator | (DeintMethod e, int i) { return (int)e | i; }
inline int operator | (int i, DeintMethod e) { return (int)e | i; }
constexpr inline int operator | (DeintMethod e1, DeintMethod e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, DeintMethod e) { return i |= (int)e; }
inline bool operator > (DeintMethod e, int i) { return (int)e > i; }
inline bool operator < (DeintMethod e, int i) { return (int)e < i; }
inline bool operator >= (DeintMethod e, int i) { return (int)e >= i; }
inline bool operator <= (DeintMethod e, int i) { return (int)e <= i; }
inline bool operator > (int i, DeintMethod e) { return i > (int)e; }
inline bool operator < (int i, DeintMethod e) { return i < (int)e; }
inline bool operator >= (int i, DeintMethod e) { return i >= (int)e; }
inline bool operator <= (int i, DeintMethod e) { return i <= (int)e; }

Q_DECLARE_METATYPE(DeintMethod)

template<>
class EnumInfo<DeintMethod> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef DeintMethod Enum;
public:
    typedef DeintMethod type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 8; }
    static constexpr const char *typeName() { return "DeintMethod"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::None: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::Bob: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::LinearBob: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::CubicBob: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::Median: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::LinearBlend: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::Yadif: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::MotionAdaptive: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 8> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return DeintMethod::None; }
private:
	static const std::array<Item, 8> info;
};

using DeintMethodInfo = EnumInfo<DeintMethod>;

enum class InterpolatorType : int {
	Bilinear = (int)0,
	BicubicBS = (int)1,
	BicubicCR = (int)2,
	BicubicMN = (int)3,
	Spline16 = (int)4,
	Spline36 = (int)5,
	Spline64 = (int)6,
	LanczosFast = (int)7,
	Lanczos2 = (int)8,
	Lanczos3 = (int)9,
	Lanczos4 = (int)10
};

inline bool operator == (InterpolatorType e, int i) { return (int)e == i; }
inline bool operator != (InterpolatorType e, int i) { return (int)e != i; }
inline bool operator == (int i, InterpolatorType e) { return (int)e == i; }
inline bool operator != (int i, InterpolatorType e) { return (int)e != i; }
inline int operator & (InterpolatorType e, int i) { return (int)e & i; }
inline int operator & (int i, InterpolatorType e) { return (int)e & i; }
inline int &operator &= (int &i, InterpolatorType e) { return i &= (int)e; }
inline int operator ~ (InterpolatorType e) { return ~(int)e; }
inline int operator | (InterpolatorType e, int i) { return (int)e | i; }
inline int operator | (int i, InterpolatorType e) { return (int)e | i; }
constexpr inline int operator | (InterpolatorType e1, InterpolatorType e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, InterpolatorType e) { return i |= (int)e; }
inline bool operator > (InterpolatorType e, int i) { return (int)e > i; }
inline bool operator < (InterpolatorType e, int i) { return (int)e < i; }
inline bool operator >= (InterpolatorType e, int i) { return (int)e >= i; }
inline bool operator <= (InterpolatorType e, int i) { return (int)e <= i; }
inline bool operator > (int i, InterpolatorType e) { return i > (int)e; }
inline bool operator < (int i, InterpolatorType e) { return i < (int)e; }
inline bool operator >= (int i, InterpolatorType e) { return i >= (int)e; }
inline bool operator <= (int i, InterpolatorType e) { return i <= (int)e; }

Q_DECLARE_METATYPE(InterpolatorType)

template<>
class EnumInfo<InterpolatorType> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef InterpolatorType Enum;
public:
    typedef InterpolatorType type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 11; }
    static constexpr const char *typeName() { return "InterpolatorType"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Bilinear: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Bilinear"));
		case Enum::BicubicBS: return tr(QT_TRANSLATE_NOOP("EnumInfo", "B-Spline"));
		case Enum::BicubicCR: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Catmull-Rom"));
		case Enum::BicubicMN: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Mitchell-Netravali"));
		case Enum::Spline16: return tr(QT_TRANSLATE_NOOP("EnumInfo", "2-Lobed Spline"));
		case Enum::Spline36: return tr(QT_TRANSLATE_NOOP("EnumInfo", "3-Lobed Spline"));
		case Enum::Spline64: return tr(QT_TRANSLATE_NOOP("EnumInfo", "4-Lobed Spline"));
		case Enum::LanczosFast: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Fast Lanczos"));
		case Enum::Lanczos2: return tr(QT_TRANSLATE_NOOP("EnumInfo", "2-Lobed Lanczos"));
		case Enum::Lanczos3: return tr(QT_TRANSLATE_NOOP("EnumInfo", "3-Lobed Lanczos"));
		case Enum::Lanczos4: return tr(QT_TRANSLATE_NOOP("EnumInfo", "4-Lobed Lanczos"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 11> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return InterpolatorType::Bilinear; }
private:
	static const std::array<Item, 11> info;
};

using InterpolatorTypeInfo = EnumInfo<InterpolatorType>;

enum class AudioDriver : int {
	Auto = (int)0,
	CoreAudio = (int)1,
	PulseAudio = (int)2,
	OSS = (int)3,
	ALSA = (int)4,
	JACK = (int)5,
	PortAudio = (int)6,
	OpenAL = (int)7
};

inline bool operator == (AudioDriver e, int i) { return (int)e == i; }
inline bool operator != (AudioDriver e, int i) { return (int)e != i; }
inline bool operator == (int i, AudioDriver e) { return (int)e == i; }
inline bool operator != (int i, AudioDriver e) { return (int)e != i; }
inline int operator & (AudioDriver e, int i) { return (int)e & i; }
inline int operator & (int i, AudioDriver e) { return (int)e & i; }
inline int &operator &= (int &i, AudioDriver e) { return i &= (int)e; }
inline int operator ~ (AudioDriver e) { return ~(int)e; }
inline int operator | (AudioDriver e, int i) { return (int)e | i; }
inline int operator | (int i, AudioDriver e) { return (int)e | i; }
constexpr inline int operator | (AudioDriver e1, AudioDriver e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, AudioDriver e) { return i |= (int)e; }
inline bool operator > (AudioDriver e, int i) { return (int)e > i; }
inline bool operator < (AudioDriver e, int i) { return (int)e < i; }
inline bool operator >= (AudioDriver e, int i) { return (int)e >= i; }
inline bool operator <= (AudioDriver e, int i) { return (int)e <= i; }
inline bool operator > (int i, AudioDriver e) { return i > (int)e; }
inline bool operator < (int i, AudioDriver e) { return i < (int)e; }
inline bool operator >= (int i, AudioDriver e) { return i >= (int)e; }
inline bool operator <= (int i, AudioDriver e) { return i <= (int)e; }

Q_DECLARE_METATYPE(AudioDriver)

template<>
class EnumInfo<AudioDriver> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef AudioDriver Enum;
public:
    typedef AudioDriver type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 8; }
    static constexpr const char *typeName() { return "AudioDriver"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Auto: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::CoreAudio: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::PulseAudio: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::OSS: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::ALSA: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::JACK: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::PortAudio: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::OpenAL: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 8> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return AudioDriver::Auto; }
private:
	static const std::array<Item, 8> info;
};

using AudioDriverInfo = EnumInfo<AudioDriver>;

enum class ClippingMethod : int {
	Auto = (int)0,
	Soft = (int)1,
	Hard = (int)2
};

inline bool operator == (ClippingMethod e, int i) { return (int)e == i; }
inline bool operator != (ClippingMethod e, int i) { return (int)e != i; }
inline bool operator == (int i, ClippingMethod e) { return (int)e == i; }
inline bool operator != (int i, ClippingMethod e) { return (int)e != i; }
inline int operator & (ClippingMethod e, int i) { return (int)e & i; }
inline int operator & (int i, ClippingMethod e) { return (int)e & i; }
inline int &operator &= (int &i, ClippingMethod e) { return i &= (int)e; }
inline int operator ~ (ClippingMethod e) { return ~(int)e; }
inline int operator | (ClippingMethod e, int i) { return (int)e | i; }
inline int operator | (int i, ClippingMethod e) { return (int)e | i; }
constexpr inline int operator | (ClippingMethod e1, ClippingMethod e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, ClippingMethod e) { return i |= (int)e; }
inline bool operator > (ClippingMethod e, int i) { return (int)e > i; }
inline bool operator < (ClippingMethod e, int i) { return (int)e < i; }
inline bool operator >= (ClippingMethod e, int i) { return (int)e >= i; }
inline bool operator <= (ClippingMethod e, int i) { return (int)e <= i; }
inline bool operator > (int i, ClippingMethod e) { return i > (int)e; }
inline bool operator < (int i, ClippingMethod e) { return i < (int)e; }
inline bool operator >= (int i, ClippingMethod e) { return i >= (int)e; }
inline bool operator <= (int i, ClippingMethod e) { return i <= (int)e; }

Q_DECLARE_METATYPE(ClippingMethod)

template<>
class EnumInfo<ClippingMethod> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef ClippingMethod Enum;
public:
    typedef ClippingMethod type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 3; }
    static constexpr const char *typeName() { return "ClippingMethod"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Auto: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Auto-clipping"));
		case Enum::Soft: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Soft-clipping"));
		case Enum::Hard: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Hard-clipping"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return ClippingMethod::Auto; }
private:
	static const std::array<Item, 3> info;
};

using ClippingMethodInfo = EnumInfo<ClippingMethod>;

enum class StaysOnTop : int {
	None = (int)0,
	Playing = (int)1,
	Always = (int)2
};

inline bool operator == (StaysOnTop e, int i) { return (int)e == i; }
inline bool operator != (StaysOnTop e, int i) { return (int)e != i; }
inline bool operator == (int i, StaysOnTop e) { return (int)e == i; }
inline bool operator != (int i, StaysOnTop e) { return (int)e != i; }
inline int operator & (StaysOnTop e, int i) { return (int)e & i; }
inline int operator & (int i, StaysOnTop e) { return (int)e & i; }
inline int &operator &= (int &i, StaysOnTop e) { return i &= (int)e; }
inline int operator ~ (StaysOnTop e) { return ~(int)e; }
inline int operator | (StaysOnTop e, int i) { return (int)e | i; }
inline int operator | (int i, StaysOnTop e) { return (int)e | i; }
constexpr inline int operator | (StaysOnTop e1, StaysOnTop e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, StaysOnTop e) { return i |= (int)e; }
inline bool operator > (StaysOnTop e, int i) { return (int)e > i; }
inline bool operator < (StaysOnTop e, int i) { return (int)e < i; }
inline bool operator >= (StaysOnTop e, int i) { return (int)e >= i; }
inline bool operator <= (StaysOnTop e, int i) { return (int)e <= i; }
inline bool operator > (int i, StaysOnTop e) { return i > (int)e; }
inline bool operator < (int i, StaysOnTop e) { return i < (int)e; }
inline bool operator >= (int i, StaysOnTop e) { return i >= (int)e; }
inline bool operator <= (int i, StaysOnTop e) { return i <= (int)e; }

Q_DECLARE_METATYPE(StaysOnTop)

template<>
class EnumInfo<StaysOnTop> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef StaysOnTop Enum;
public:
    typedef StaysOnTop type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 3; }
    static constexpr const char *typeName() { return "StaysOnTop"; }
    static constexpr const char *typeKey() { return "stays-on-top"; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "Stays on Top")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::None: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Off"));
		case Enum::Playing: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Playing"));
		case Enum::Always: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Always"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return StaysOnTop::Playing; }
private:
	static const std::array<Item, 3> info;
};

using StaysOnTopInfo = EnumInfo<StaysOnTop>;

enum class SeekingStep : int {
	Step1 = (int)0,
	Step2 = (int)1,
	Step3 = (int)2
};

inline bool operator == (SeekingStep e, int i) { return (int)e == i; }
inline bool operator != (SeekingStep e, int i) { return (int)e != i; }
inline bool operator == (int i, SeekingStep e) { return (int)e == i; }
inline bool operator != (int i, SeekingStep e) { return (int)e != i; }
inline int operator & (SeekingStep e, int i) { return (int)e & i; }
inline int operator & (int i, SeekingStep e) { return (int)e & i; }
inline int &operator &= (int &i, SeekingStep e) { return i &= (int)e; }
inline int operator ~ (SeekingStep e) { return ~(int)e; }
inline int operator | (SeekingStep e, int i) { return (int)e | i; }
inline int operator | (int i, SeekingStep e) { return (int)e | i; }
constexpr inline int operator | (SeekingStep e1, SeekingStep e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, SeekingStep e) { return i |= (int)e; }
inline bool operator > (SeekingStep e, int i) { return (int)e > i; }
inline bool operator < (SeekingStep e, int i) { return (int)e < i; }
inline bool operator >= (SeekingStep e, int i) { return (int)e >= i; }
inline bool operator <= (SeekingStep e, int i) { return (int)e <= i; }
inline bool operator > (int i, SeekingStep e) { return i > (int)e; }
inline bool operator < (int i, SeekingStep e) { return i < (int)e; }
inline bool operator >= (int i, SeekingStep e) { return i >= (int)e; }
inline bool operator <= (int i, SeekingStep e) { return i <= (int)e; }

Q_DECLARE_METATYPE(SeekingStep)

template<>
class EnumInfo<SeekingStep> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef SeekingStep Enum;
public:
    typedef SeekingStep type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 3; }
    static constexpr const char *typeName() { return "SeekingStep"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Step1: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::Step2: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::Step3: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return SeekingStep::Step1; }
private:
	static const std::array<Item, 3> info;
};

using SeekingStepInfo = EnumInfo<SeekingStep>;

enum class GeneratePlaylist : int {
	Similar = (int)0,
	Folder = (int)1
};

inline bool operator == (GeneratePlaylist e, int i) { return (int)e == i; }
inline bool operator != (GeneratePlaylist e, int i) { return (int)e != i; }
inline bool operator == (int i, GeneratePlaylist e) { return (int)e == i; }
inline bool operator != (int i, GeneratePlaylist e) { return (int)e != i; }
inline int operator & (GeneratePlaylist e, int i) { return (int)e & i; }
inline int operator & (int i, GeneratePlaylist e) { return (int)e & i; }
inline int &operator &= (int &i, GeneratePlaylist e) { return i &= (int)e; }
inline int operator ~ (GeneratePlaylist e) { return ~(int)e; }
inline int operator | (GeneratePlaylist e, int i) { return (int)e | i; }
inline int operator | (int i, GeneratePlaylist e) { return (int)e | i; }
constexpr inline int operator | (GeneratePlaylist e1, GeneratePlaylist e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, GeneratePlaylist e) { return i |= (int)e; }
inline bool operator > (GeneratePlaylist e, int i) { return (int)e > i; }
inline bool operator < (GeneratePlaylist e, int i) { return (int)e < i; }
inline bool operator >= (GeneratePlaylist e, int i) { return (int)e >= i; }
inline bool operator <= (GeneratePlaylist e, int i) { return (int)e <= i; }
inline bool operator > (int i, GeneratePlaylist e) { return i > (int)e; }
inline bool operator < (int i, GeneratePlaylist e) { return i < (int)e; }
inline bool operator >= (int i, GeneratePlaylist e) { return i >= (int)e; }
inline bool operator <= (int i, GeneratePlaylist e) { return i <= (int)e; }

Q_DECLARE_METATYPE(GeneratePlaylist)

template<>
class EnumInfo<GeneratePlaylist> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef GeneratePlaylist Enum;
public:
    typedef GeneratePlaylist type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 2; }
    static constexpr const char *typeName() { return "GeneratePlaylist"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Similar: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Add files which have similar names"));
		case Enum::Folder: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Add all files in the same folder"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 2> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return GeneratePlaylist::Similar; }
private:
	static const std::array<Item, 2> info;
};

using GeneratePlaylistInfo = EnumInfo<GeneratePlaylist>;

enum class PlaylistBehaviorWhenOpenMedia : int {
	AppendToPlaylist = (int)0,
	ClearAndAppendToPlaylist = (int)1,
	ClearAndGenerateNewPlaylist = (int)2
};

inline bool operator == (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e == i; }
inline bool operator != (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e != i; }
inline bool operator == (int i, PlaylistBehaviorWhenOpenMedia e) { return (int)e == i; }
inline bool operator != (int i, PlaylistBehaviorWhenOpenMedia e) { return (int)e != i; }
inline int operator & (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e & i; }
inline int operator & (int i, PlaylistBehaviorWhenOpenMedia e) { return (int)e & i; }
inline int &operator &= (int &i, PlaylistBehaviorWhenOpenMedia e) { return i &= (int)e; }
inline int operator ~ (PlaylistBehaviorWhenOpenMedia e) { return ~(int)e; }
inline int operator | (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e | i; }
inline int operator | (int i, PlaylistBehaviorWhenOpenMedia e) { return (int)e | i; }
constexpr inline int operator | (PlaylistBehaviorWhenOpenMedia e1, PlaylistBehaviorWhenOpenMedia e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, PlaylistBehaviorWhenOpenMedia e) { return i |= (int)e; }
inline bool operator > (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e > i; }
inline bool operator < (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e < i; }
inline bool operator >= (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e >= i; }
inline bool operator <= (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e <= i; }
inline bool operator > (int i, PlaylistBehaviorWhenOpenMedia e) { return i > (int)e; }
inline bool operator < (int i, PlaylistBehaviorWhenOpenMedia e) { return i < (int)e; }
inline bool operator >= (int i, PlaylistBehaviorWhenOpenMedia e) { return i >= (int)e; }
inline bool operator <= (int i, PlaylistBehaviorWhenOpenMedia e) { return i <= (int)e; }

Q_DECLARE_METATYPE(PlaylistBehaviorWhenOpenMedia)

template<>
class EnumInfo<PlaylistBehaviorWhenOpenMedia> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef PlaylistBehaviorWhenOpenMedia Enum;
public:
    typedef PlaylistBehaviorWhenOpenMedia type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 3; }
    static constexpr const char *typeName() { return "PlaylistBehaviorWhenOpenMedia"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::AppendToPlaylist: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Append the open media to the playlist"));
		case Enum::ClearAndAppendToPlaylist: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Clear the playlist and append the open media to the playlist"));
		case Enum::ClearAndGenerateNewPlaylist: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Clear the playlist and generate new playlist"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return PlaylistBehaviorWhenOpenMedia::AppendToPlaylist; }
private:
	static const std::array<Item, 3> info;
};

using PlaylistBehaviorWhenOpenMediaInfo = EnumInfo<PlaylistBehaviorWhenOpenMedia>;

enum class SubtitleAutoload : int {
	Matched = (int)0,
	Contain = (int)1,
	Folder = (int)2
};

inline bool operator == (SubtitleAutoload e, int i) { return (int)e == i; }
inline bool operator != (SubtitleAutoload e, int i) { return (int)e != i; }
inline bool operator == (int i, SubtitleAutoload e) { return (int)e == i; }
inline bool operator != (int i, SubtitleAutoload e) { return (int)e != i; }
inline int operator & (SubtitleAutoload e, int i) { return (int)e & i; }
inline int operator & (int i, SubtitleAutoload e) { return (int)e & i; }
inline int &operator &= (int &i, SubtitleAutoload e) { return i &= (int)e; }
inline int operator ~ (SubtitleAutoload e) { return ~(int)e; }
inline int operator | (SubtitleAutoload e, int i) { return (int)e | i; }
inline int operator | (int i, SubtitleAutoload e) { return (int)e | i; }
constexpr inline int operator | (SubtitleAutoload e1, SubtitleAutoload e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, SubtitleAutoload e) { return i |= (int)e; }
inline bool operator > (SubtitleAutoload e, int i) { return (int)e > i; }
inline bool operator < (SubtitleAutoload e, int i) { return (int)e < i; }
inline bool operator >= (SubtitleAutoload e, int i) { return (int)e >= i; }
inline bool operator <= (SubtitleAutoload e, int i) { return (int)e <= i; }
inline bool operator > (int i, SubtitleAutoload e) { return i > (int)e; }
inline bool operator < (int i, SubtitleAutoload e) { return i < (int)e; }
inline bool operator >= (int i, SubtitleAutoload e) { return i >= (int)e; }
inline bool operator <= (int i, SubtitleAutoload e) { return i <= (int)e; }

Q_DECLARE_METATYPE(SubtitleAutoload)

template<>
class EnumInfo<SubtitleAutoload> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef SubtitleAutoload Enum;
public:
    typedef SubtitleAutoload type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 3; }
    static constexpr const char *typeName() { return "SubtitleAutoload"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Matched: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Subtitles which have the same name as that of playing file"));
		case Enum::Contain: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Subtitles whose names contain the name of playing file"));
		case Enum::Folder: return tr(QT_TRANSLATE_NOOP("EnumInfo", "All subtitles in the folder where the playing file is located"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return SubtitleAutoload::Matched; }
private:
	static const std::array<Item, 3> info;
};

using SubtitleAutoloadInfo = EnumInfo<SubtitleAutoload>;

enum class SubtitleAutoselect : int {
	Matched = (int)0,
	First = (int)1,
	All = (int)2,
	EachLanguage = (int)3
};

inline bool operator == (SubtitleAutoselect e, int i) { return (int)e == i; }
inline bool operator != (SubtitleAutoselect e, int i) { return (int)e != i; }
inline bool operator == (int i, SubtitleAutoselect e) { return (int)e == i; }
inline bool operator != (int i, SubtitleAutoselect e) { return (int)e != i; }
inline int operator & (SubtitleAutoselect e, int i) { return (int)e & i; }
inline int operator & (int i, SubtitleAutoselect e) { return (int)e & i; }
inline int &operator &= (int &i, SubtitleAutoselect e) { return i &= (int)e; }
inline int operator ~ (SubtitleAutoselect e) { return ~(int)e; }
inline int operator | (SubtitleAutoselect e, int i) { return (int)e | i; }
inline int operator | (int i, SubtitleAutoselect e) { return (int)e | i; }
constexpr inline int operator | (SubtitleAutoselect e1, SubtitleAutoselect e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, SubtitleAutoselect e) { return i |= (int)e; }
inline bool operator > (SubtitleAutoselect e, int i) { return (int)e > i; }
inline bool operator < (SubtitleAutoselect e, int i) { return (int)e < i; }
inline bool operator >= (SubtitleAutoselect e, int i) { return (int)e >= i; }
inline bool operator <= (SubtitleAutoselect e, int i) { return (int)e <= i; }
inline bool operator > (int i, SubtitleAutoselect e) { return i > (int)e; }
inline bool operator < (int i, SubtitleAutoselect e) { return i < (int)e; }
inline bool operator >= (int i, SubtitleAutoselect e) { return i >= (int)e; }
inline bool operator <= (int i, SubtitleAutoselect e) { return i <= (int)e; }

Q_DECLARE_METATYPE(SubtitleAutoselect)

template<>
class EnumInfo<SubtitleAutoselect> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef SubtitleAutoselect Enum;
public:
    typedef SubtitleAutoselect type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 4; }
    static constexpr const char *typeName() { return "SubtitleAutoselect"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Matched: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Subtitle which has the same name as that of playing file"));
		case Enum::First: return tr(QT_TRANSLATE_NOOP("EnumInfo", "First subtitle from loaded ones"));
		case Enum::All: return tr(QT_TRANSLATE_NOOP("EnumInfo", "All loaded subtitles"));
		case Enum::EachLanguage: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Each language subtitle"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 4> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return SubtitleAutoselect::Matched; }
private:
	static const std::array<Item, 4> info;
};

using SubtitleAutoselectInfo = EnumInfo<SubtitleAutoselect>;

enum class OsdScalePolicy : int {
	Width = (int)0,
	Height = (int)1,
	Diagonal = (int)2
};

inline bool operator == (OsdScalePolicy e, int i) { return (int)e == i; }
inline bool operator != (OsdScalePolicy e, int i) { return (int)e != i; }
inline bool operator == (int i, OsdScalePolicy e) { return (int)e == i; }
inline bool operator != (int i, OsdScalePolicy e) { return (int)e != i; }
inline int operator & (OsdScalePolicy e, int i) { return (int)e & i; }
inline int operator & (int i, OsdScalePolicy e) { return (int)e & i; }
inline int &operator &= (int &i, OsdScalePolicy e) { return i &= (int)e; }
inline int operator ~ (OsdScalePolicy e) { return ~(int)e; }
inline int operator | (OsdScalePolicy e, int i) { return (int)e | i; }
inline int operator | (int i, OsdScalePolicy e) { return (int)e | i; }
constexpr inline int operator | (OsdScalePolicy e1, OsdScalePolicy e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, OsdScalePolicy e) { return i |= (int)e; }
inline bool operator > (OsdScalePolicy e, int i) { return (int)e > i; }
inline bool operator < (OsdScalePolicy e, int i) { return (int)e < i; }
inline bool operator >= (OsdScalePolicy e, int i) { return (int)e >= i; }
inline bool operator <= (OsdScalePolicy e, int i) { return (int)e <= i; }
inline bool operator > (int i, OsdScalePolicy e) { return i > (int)e; }
inline bool operator < (int i, OsdScalePolicy e) { return i < (int)e; }
inline bool operator >= (int i, OsdScalePolicy e) { return i >= (int)e; }
inline bool operator <= (int i, OsdScalePolicy e) { return i <= (int)e; }

Q_DECLARE_METATYPE(OsdScalePolicy)

template<>
class EnumInfo<OsdScalePolicy> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef OsdScalePolicy Enum;
public:
    typedef OsdScalePolicy type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 3; }
    static constexpr const char *typeName() { return "OsdScalePolicy"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Width: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Fit to width of video"));
		case Enum::Height: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Fit to height of video"));
		case Enum::Diagonal: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Fit to diagonal of video"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return OsdScalePolicy::Width; }
private:
	static const std::array<Item, 3> info;
};

using OsdScalePolicyInfo = EnumInfo<OsdScalePolicy>;

enum class ClickAction : int {
	OpenFile = (int)0,
	Fullscreen = (int)1,
	Pause = (int)2,
	Mute = (int)3
};

inline bool operator == (ClickAction e, int i) { return (int)e == i; }
inline bool operator != (ClickAction e, int i) { return (int)e != i; }
inline bool operator == (int i, ClickAction e) { return (int)e == i; }
inline bool operator != (int i, ClickAction e) { return (int)e != i; }
inline int operator & (ClickAction e, int i) { return (int)e & i; }
inline int operator & (int i, ClickAction e) { return (int)e & i; }
inline int &operator &= (int &i, ClickAction e) { return i &= (int)e; }
inline int operator ~ (ClickAction e) { return ~(int)e; }
inline int operator | (ClickAction e, int i) { return (int)e | i; }
inline int operator | (int i, ClickAction e) { return (int)e | i; }
constexpr inline int operator | (ClickAction e1, ClickAction e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, ClickAction e) { return i |= (int)e; }
inline bool operator > (ClickAction e, int i) { return (int)e > i; }
inline bool operator < (ClickAction e, int i) { return (int)e < i; }
inline bool operator >= (ClickAction e, int i) { return (int)e >= i; }
inline bool operator <= (ClickAction e, int i) { return (int)e <= i; }
inline bool operator > (int i, ClickAction e) { return i > (int)e; }
inline bool operator < (int i, ClickAction e) { return i < (int)e; }
inline bool operator >= (int i, ClickAction e) { return i >= (int)e; }
inline bool operator <= (int i, ClickAction e) { return i <= (int)e; }

Q_DECLARE_METATYPE(ClickAction)

template<>
class EnumInfo<ClickAction> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef ClickAction Enum;
public:
    typedef ClickAction type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 4; }
    static constexpr const char *typeName() { return "ClickAction"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::OpenFile: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Open a file"));
		case Enum::Fullscreen: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Toggle fullscreen mode"));
		case Enum::Pause: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Toggle play/pause"));
		case Enum::Mute: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Toggle mute/unmute"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 4> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return ClickAction::OpenFile; }
private:
	static const std::array<Item, 4> info;
};

using ClickActionInfo = EnumInfo<ClickAction>;

enum class WheelAction : int {
	Seek1 = (int)0,
	Seek2 = (int)1,
	Seek3 = (int)2,
	PrevNext = (int)3,
	Volume = (int)4,
	Amp = (int)5
};

inline bool operator == (WheelAction e, int i) { return (int)e == i; }
inline bool operator != (WheelAction e, int i) { return (int)e != i; }
inline bool operator == (int i, WheelAction e) { return (int)e == i; }
inline bool operator != (int i, WheelAction e) { return (int)e != i; }
inline int operator & (WheelAction e, int i) { return (int)e & i; }
inline int operator & (int i, WheelAction e) { return (int)e & i; }
inline int &operator &= (int &i, WheelAction e) { return i &= (int)e; }
inline int operator ~ (WheelAction e) { return ~(int)e; }
inline int operator | (WheelAction e, int i) { return (int)e | i; }
inline int operator | (int i, WheelAction e) { return (int)e | i; }
constexpr inline int operator | (WheelAction e1, WheelAction e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, WheelAction e) { return i |= (int)e; }
inline bool operator > (WheelAction e, int i) { return (int)e > i; }
inline bool operator < (WheelAction e, int i) { return (int)e < i; }
inline bool operator >= (WheelAction e, int i) { return (int)e >= i; }
inline bool operator <= (WheelAction e, int i) { return (int)e <= i; }
inline bool operator > (int i, WheelAction e) { return i > (int)e; }
inline bool operator < (int i, WheelAction e) { return i < (int)e; }
inline bool operator >= (int i, WheelAction e) { return i >= (int)e; }
inline bool operator <= (int i, WheelAction e) { return i <= (int)e; }

Q_DECLARE_METATYPE(WheelAction)

template<>
class EnumInfo<WheelAction> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef WheelAction Enum;
public:
    typedef WheelAction type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 6; }
    static constexpr const char *typeName() { return "WheelAction"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Seek1: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Seek playback for step 1"));
		case Enum::Seek2: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Seek playback for step 2"));
		case Enum::Seek3: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Seek playback for step 3"));
		case Enum::PrevNext: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Play previous/next"));
		case Enum::Volume: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Volumn up/down"));
		case Enum::Amp: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Amp. up/down"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 6> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return WheelAction::Seek1; }
private:
	static const std::array<Item, 6> info;
};

using WheelActionInfo = EnumInfo<WheelAction>;

enum class KeyModifier : int {
	None = (int)Qt::NoModifier,
	Ctrl = (int)Qt::ControlModifier,
	Shift = (int)Qt::ShiftModifier,
	Alt = (int)Qt::AltModifier
};

inline bool operator == (KeyModifier e, int i) { return (int)e == i; }
inline bool operator != (KeyModifier e, int i) { return (int)e != i; }
inline bool operator == (int i, KeyModifier e) { return (int)e == i; }
inline bool operator != (int i, KeyModifier e) { return (int)e != i; }
inline int operator & (KeyModifier e, int i) { return (int)e & i; }
inline int operator & (int i, KeyModifier e) { return (int)e & i; }
inline int &operator &= (int &i, KeyModifier e) { return i &= (int)e; }
inline int operator ~ (KeyModifier e) { return ~(int)e; }
inline int operator | (KeyModifier e, int i) { return (int)e | i; }
inline int operator | (int i, KeyModifier e) { return (int)e | i; }
constexpr inline int operator | (KeyModifier e1, KeyModifier e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, KeyModifier e) { return i |= (int)e; }
inline bool operator > (KeyModifier e, int i) { return (int)e > i; }
inline bool operator < (KeyModifier e, int i) { return (int)e < i; }
inline bool operator >= (KeyModifier e, int i) { return (int)e >= i; }
inline bool operator <= (KeyModifier e, int i) { return (int)e <= i; }
inline bool operator > (int i, KeyModifier e) { return i > (int)e; }
inline bool operator < (int i, KeyModifier e) { return i < (int)e; }
inline bool operator >= (int i, KeyModifier e) { return i >= (int)e; }
inline bool operator <= (int i, KeyModifier e) { return i <= (int)e; }

Q_DECLARE_METATYPE(KeyModifier)

template<>
class EnumInfo<KeyModifier> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef KeyModifier Enum;
public:
    typedef KeyModifier type;
    using Data =  QVariant;
    struct Item { Enum value; QString name, key; QVariant data; };
	static constexpr int size() { return 4; }
    static constexpr const char *typeName() { return "KeyModifier"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        auto it = std::find_if(info.cbegin(), info.cend(), [e](const Item &info) { return info.value == e; }); return it != info.cend() ? &(*it) : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QVariant data(Enum e) { auto i = item(e); return i ? i->data : QVariant(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::None: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::Ctrl: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::Shift: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		case Enum::Alt: return tr(QT_TRANSLATE_NOOP("EnumInfo", ""));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 4> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QVariant &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return KeyModifier::None; }
private:
	static const std::array<Item, 4> info;
};

using KeyModifierInfo = EnumInfo<KeyModifier>;

enum class VerticalAlignment : int {
	Top = (int)0,
	Center = (int)1,
	Bottom = (int)2
};

inline bool operator == (VerticalAlignment e, int i) { return (int)e == i; }
inline bool operator != (VerticalAlignment e, int i) { return (int)e != i; }
inline bool operator == (int i, VerticalAlignment e) { return (int)e == i; }
inline bool operator != (int i, VerticalAlignment e) { return (int)e != i; }
inline int operator & (VerticalAlignment e, int i) { return (int)e & i; }
inline int operator & (int i, VerticalAlignment e) { return (int)e & i; }
inline int &operator &= (int &i, VerticalAlignment e) { return i &= (int)e; }
inline int operator ~ (VerticalAlignment e) { return ~(int)e; }
inline int operator | (VerticalAlignment e, int i) { return (int)e | i; }
inline int operator | (int i, VerticalAlignment e) { return (int)e | i; }
constexpr inline int operator | (VerticalAlignment e1, VerticalAlignment e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, VerticalAlignment e) { return i |= (int)e; }
inline bool operator > (VerticalAlignment e, int i) { return (int)e > i; }
inline bool operator < (VerticalAlignment e, int i) { return (int)e < i; }
inline bool operator >= (VerticalAlignment e, int i) { return (int)e >= i; }
inline bool operator <= (VerticalAlignment e, int i) { return (int)e <= i; }
inline bool operator > (int i, VerticalAlignment e) { return i > (int)e; }
inline bool operator < (int i, VerticalAlignment e) { return i < (int)e; }
inline bool operator >= (int i, VerticalAlignment e) { return i >= (int)e; }
inline bool operator <= (int i, VerticalAlignment e) { return i <= (int)e; }

Q_DECLARE_METATYPE(VerticalAlignment)

template<>
class EnumInfo<VerticalAlignment> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef VerticalAlignment Enum;
public:
    typedef VerticalAlignment type;
    using Data =  Qt::Alignment;
    struct Item { Enum value; QString name, key; Qt::Alignment data; };
	static constexpr int size() { return 3; }
    static constexpr const char *typeName() { return "VerticalAlignment"; }
    static constexpr const char *typeKey() { return "vertical-alignment"; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static Qt::Alignment data(Enum e) { auto i = item(e); return i ? i->data : Qt::Alignment(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Top: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Top"));
		case Enum::Center: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Vertical Center"));
		case Enum::Bottom: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Bottom"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const Qt::Alignment &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return VerticalAlignment::Center; }
private:
	static const std::array<Item, 3> info;
};

using VerticalAlignmentInfo = EnumInfo<VerticalAlignment>;

enum class HorizontalAlignment : int {
	Left = (int)0,
	Center = (int)1,
	Right = (int)2
};

inline bool operator == (HorizontalAlignment e, int i) { return (int)e == i; }
inline bool operator != (HorizontalAlignment e, int i) { return (int)e != i; }
inline bool operator == (int i, HorizontalAlignment e) { return (int)e == i; }
inline bool operator != (int i, HorizontalAlignment e) { return (int)e != i; }
inline int operator & (HorizontalAlignment e, int i) { return (int)e & i; }
inline int operator & (int i, HorizontalAlignment e) { return (int)e & i; }
inline int &operator &= (int &i, HorizontalAlignment e) { return i &= (int)e; }
inline int operator ~ (HorizontalAlignment e) { return ~(int)e; }
inline int operator | (HorizontalAlignment e, int i) { return (int)e | i; }
inline int operator | (int i, HorizontalAlignment e) { return (int)e | i; }
constexpr inline int operator | (HorizontalAlignment e1, HorizontalAlignment e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, HorizontalAlignment e) { return i |= (int)e; }
inline bool operator > (HorizontalAlignment e, int i) { return (int)e > i; }
inline bool operator < (HorizontalAlignment e, int i) { return (int)e < i; }
inline bool operator >= (HorizontalAlignment e, int i) { return (int)e >= i; }
inline bool operator <= (HorizontalAlignment e, int i) { return (int)e <= i; }
inline bool operator > (int i, HorizontalAlignment e) { return i > (int)e; }
inline bool operator < (int i, HorizontalAlignment e) { return i < (int)e; }
inline bool operator >= (int i, HorizontalAlignment e) { return i >= (int)e; }
inline bool operator <= (int i, HorizontalAlignment e) { return i <= (int)e; }

Q_DECLARE_METATYPE(HorizontalAlignment)

template<>
class EnumInfo<HorizontalAlignment> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef HorizontalAlignment Enum;
public:
    typedef HorizontalAlignment type;
    using Data =  Qt::Alignment;
    struct Item { Enum value; QString name, key; Qt::Alignment data; };
	static constexpr int size() { return 3; }
    static constexpr const char *typeName() { return "HorizontalAlignment"; }
    static constexpr const char *typeKey() { return "horizontal-alignment"; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static Qt::Alignment data(Enum e) { auto i = item(e); return i ? i->data : Qt::Alignment(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Left: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Left"));
		case Enum::Center: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Horizontal Center"));
		case Enum::Right: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Right"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const Qt::Alignment &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return HorizontalAlignment::Center; }
private:
	static const std::array<Item, 3> info;
};

using HorizontalAlignmentInfo = EnumInfo<HorizontalAlignment>;

enum class MoveToward : int {
	Reset = (int)0,
	Upward = (int)1,
	Downward = (int)2,
	Leftward = (int)3,
	Rightward = (int)4
};

inline bool operator == (MoveToward e, int i) { return (int)e == i; }
inline bool operator != (MoveToward e, int i) { return (int)e != i; }
inline bool operator == (int i, MoveToward e) { return (int)e == i; }
inline bool operator != (int i, MoveToward e) { return (int)e != i; }
inline int operator & (MoveToward e, int i) { return (int)e & i; }
inline int operator & (int i, MoveToward e) { return (int)e & i; }
inline int &operator &= (int &i, MoveToward e) { return i &= (int)e; }
inline int operator ~ (MoveToward e) { return ~(int)e; }
inline int operator | (MoveToward e, int i) { return (int)e | i; }
inline int operator | (int i, MoveToward e) { return (int)e | i; }
constexpr inline int operator | (MoveToward e1, MoveToward e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, MoveToward e) { return i |= (int)e; }
inline bool operator > (MoveToward e, int i) { return (int)e > i; }
inline bool operator < (MoveToward e, int i) { return (int)e < i; }
inline bool operator >= (MoveToward e, int i) { return (int)e >= i; }
inline bool operator <= (MoveToward e, int i) { return (int)e <= i; }
inline bool operator > (int i, MoveToward e) { return i > (int)e; }
inline bool operator < (int i, MoveToward e) { return i < (int)e; }
inline bool operator >= (int i, MoveToward e) { return i >= (int)e; }
inline bool operator <= (int i, MoveToward e) { return i <= (int)e; }

Q_DECLARE_METATYPE(MoveToward)

template<>
class EnumInfo<MoveToward> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef MoveToward Enum;
public:
    typedef MoveToward type;
    using Data =  QPoint;
    struct Item { Enum value; QString name, key; QPoint data; };
	static constexpr int size() { return 5; }
    static constexpr const char *typeName() { return "MoveToward"; }
    static constexpr const char *typeKey() { return "move"; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static QPoint data(Enum e) { auto i = item(e); return i ? i->data : QPoint(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Reset: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Reset"));
		case Enum::Upward: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Upward"));
		case Enum::Downward: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Downward"));
		case Enum::Leftward: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Leftward"));
		case Enum::Rightward: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Rightward"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 5> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const QPoint &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return MoveToward::Reset; }
private:
	static const std::array<Item, 5> info;
};

using MoveTowardInfo = EnumInfo<MoveToward>;

enum class ChangeValue : int {
	Reset = (int)0,
	Increase = (int)1,
	Decrease = (int)2
};

inline bool operator == (ChangeValue e, int i) { return (int)e == i; }
inline bool operator != (ChangeValue e, int i) { return (int)e != i; }
inline bool operator == (int i, ChangeValue e) { return (int)e == i; }
inline bool operator != (int i, ChangeValue e) { return (int)e != i; }
inline int operator & (ChangeValue e, int i) { return (int)e & i; }
inline int operator & (int i, ChangeValue e) { return (int)e & i; }
inline int &operator &= (int &i, ChangeValue e) { return i &= (int)e; }
inline int operator ~ (ChangeValue e) { return ~(int)e; }
inline int operator | (ChangeValue e, int i) { return (int)e | i; }
inline int operator | (int i, ChangeValue e) { return (int)e | i; }
constexpr inline int operator | (ChangeValue e1, ChangeValue e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, ChangeValue e) { return i |= (int)e; }
inline bool operator > (ChangeValue e, int i) { return (int)e > i; }
inline bool operator < (ChangeValue e, int i) { return (int)e < i; }
inline bool operator >= (ChangeValue e, int i) { return (int)e >= i; }
inline bool operator <= (ChangeValue e, int i) { return (int)e <= i; }
inline bool operator > (int i, ChangeValue e) { return i > (int)e; }
inline bool operator < (int i, ChangeValue e) { return i < (int)e; }
inline bool operator >= (int i, ChangeValue e) { return i >= (int)e; }
inline bool operator <= (int i, ChangeValue e) { return i <= (int)e; }

Q_DECLARE_METATYPE(ChangeValue)

template<>
class EnumInfo<ChangeValue> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef ChangeValue Enum;
public:
    typedef ChangeValue type;
    using Data =  int;
    struct Item { Enum value; QString name, key; int data; };
	static constexpr int size() { return 3; }
    static constexpr const char *typeName() { return "ChangeValue"; }
    static constexpr const char *typeKey() { return ""; }
    static QString typeDescription() { return tr(QT_TRANSLATE_NOOP("EnumInfo", "")); }
    static const Item *item(Enum e) {
        return 0 <= e && e < size() ? &info[(int)e] : nullptr;
    }
    static QString name(Enum e) { auto i = item(e); return i ? i->name : QString(); }
    static QString key(Enum e) { auto i = item(e); return i ? i->key : QString(); }
    static int data(Enum e) { auto i = item(e); return i ? i->data : int(); }
	static QString description(int e) { return description((Enum)e); }
	static QString description(Enum e) {
		switch (e) {
		case Enum::Reset: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Reset"));
		case Enum::Increase: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Increase %1"));
		case Enum::Decrease: return tr(QT_TRANSLATE_NOOP("EnumInfo", "Decrease %1"));
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
    static Enum from(int id, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
    static Enum from(const QString &name, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&name] (const Item &item) { return !name.compare(item.name); });
		return it != info.cend() ? it->value : def;
	}
	static Enum fromData(const int &data, Enum def = default_()) {
		auto it = std::find_if(info.cbegin(), info.cend(), [&data] (const Item &item) { return item.data == data; });
		return it != info.cend() ? it->value : def;
	}
    static constexpr Enum default_() { return ChangeValue::Reset; }
private:
	static const std::array<Item, 3> info;
};

using ChangeValueInfo = EnumInfo<ChangeValue>;
static inline bool _IsEnumTypeId(int userType) {
	return userType == qMetaTypeId<SpeakerId>()
		|| userType == qMetaTypeId<ChannelLayout>()
		|| userType == qMetaTypeId<ColorRange>()
		|| userType == qMetaTypeId<AdjustColor>()
		|| userType == qMetaTypeId<SubtitleDisplay>()
		|| userType == qMetaTypeId<VideoRatio>()
		|| userType == qMetaTypeId<Dithering>()
		|| userType == qMetaTypeId<DecoderDevice>()
		|| userType == qMetaTypeId<DeintMode>()
		|| userType == qMetaTypeId<DeintDevice>()
		|| userType == qMetaTypeId<DeintMethod>()
		|| userType == qMetaTypeId<InterpolatorType>()
		|| userType == qMetaTypeId<AudioDriver>()
		|| userType == qMetaTypeId<ClippingMethod>()
		|| userType == qMetaTypeId<StaysOnTop>()
		|| userType == qMetaTypeId<SeekingStep>()
		|| userType == qMetaTypeId<GeneratePlaylist>()
		|| userType == qMetaTypeId<PlaylistBehaviorWhenOpenMedia>()
		|| userType == qMetaTypeId<SubtitleAutoload>()
		|| userType == qMetaTypeId<SubtitleAutoselect>()
		|| userType == qMetaTypeId<OsdScalePolicy>()
		|| userType == qMetaTypeId<ClickAction>()
		|| userType == qMetaTypeId<WheelAction>()
		|| userType == qMetaTypeId<KeyModifier>()
		|| userType == qMetaTypeId<VerticalAlignment>()
		|| userType == qMetaTypeId<HorizontalAlignment>()
		|| userType == qMetaTypeId<MoveToward>()
		|| userType == qMetaTypeId<ChangeValue>()
		|| false;
}

static inline bool _GetEnumFunctionsForSql(int varType, EnumVariantToSqlFunc &toSql, EnumVariantFromSqlFunc &fromSql) {
	if (varType == qMetaTypeId<SpeakerId>()) {
		toSql = _EnumVariantToSql<SpeakerId>;
		fromSql = _EnumVariantFromSql<SpeakerId>;
	} else	if (varType == qMetaTypeId<ChannelLayout>()) {
		toSql = _EnumVariantToSql<ChannelLayout>;
		fromSql = _EnumVariantFromSql<ChannelLayout>;
	} else	if (varType == qMetaTypeId<ColorRange>()) {
		toSql = _EnumVariantToSql<ColorRange>;
		fromSql = _EnumVariantFromSql<ColorRange>;
	} else	if (varType == qMetaTypeId<AdjustColor>()) {
		toSql = _EnumVariantToSql<AdjustColor>;
		fromSql = _EnumVariantFromSql<AdjustColor>;
	} else	if (varType == qMetaTypeId<SubtitleDisplay>()) {
		toSql = _EnumVariantToSql<SubtitleDisplay>;
		fromSql = _EnumVariantFromSql<SubtitleDisplay>;
	} else	if (varType == qMetaTypeId<VideoRatio>()) {
		toSql = _EnumVariantToSql<VideoRatio>;
		fromSql = _EnumVariantFromSql<VideoRatio>;
	} else	if (varType == qMetaTypeId<Dithering>()) {
		toSql = _EnumVariantToSql<Dithering>;
		fromSql = _EnumVariantFromSql<Dithering>;
	} else	if (varType == qMetaTypeId<DecoderDevice>()) {
		toSql = _EnumVariantToSql<DecoderDevice>;
		fromSql = _EnumVariantFromSql<DecoderDevice>;
	} else	if (varType == qMetaTypeId<DeintMode>()) {
		toSql = _EnumVariantToSql<DeintMode>;
		fromSql = _EnumVariantFromSql<DeintMode>;
	} else	if (varType == qMetaTypeId<DeintDevice>()) {
		toSql = _EnumVariantToSql<DeintDevice>;
		fromSql = _EnumVariantFromSql<DeintDevice>;
	} else	if (varType == qMetaTypeId<DeintMethod>()) {
		toSql = _EnumVariantToSql<DeintMethod>;
		fromSql = _EnumVariantFromSql<DeintMethod>;
	} else	if (varType == qMetaTypeId<InterpolatorType>()) {
		toSql = _EnumVariantToSql<InterpolatorType>;
		fromSql = _EnumVariantFromSql<InterpolatorType>;
	} else	if (varType == qMetaTypeId<AudioDriver>()) {
		toSql = _EnumVariantToSql<AudioDriver>;
		fromSql = _EnumVariantFromSql<AudioDriver>;
	} else	if (varType == qMetaTypeId<ClippingMethod>()) {
		toSql = _EnumVariantToSql<ClippingMethod>;
		fromSql = _EnumVariantFromSql<ClippingMethod>;
	} else	if (varType == qMetaTypeId<StaysOnTop>()) {
		toSql = _EnumVariantToSql<StaysOnTop>;
		fromSql = _EnumVariantFromSql<StaysOnTop>;
	} else	if (varType == qMetaTypeId<SeekingStep>()) {
		toSql = _EnumVariantToSql<SeekingStep>;
		fromSql = _EnumVariantFromSql<SeekingStep>;
	} else	if (varType == qMetaTypeId<GeneratePlaylist>()) {
		toSql = _EnumVariantToSql<GeneratePlaylist>;
		fromSql = _EnumVariantFromSql<GeneratePlaylist>;
	} else	if (varType == qMetaTypeId<PlaylistBehaviorWhenOpenMedia>()) {
		toSql = _EnumVariantToSql<PlaylistBehaviorWhenOpenMedia>;
		fromSql = _EnumVariantFromSql<PlaylistBehaviorWhenOpenMedia>;
	} else	if (varType == qMetaTypeId<SubtitleAutoload>()) {
		toSql = _EnumVariantToSql<SubtitleAutoload>;
		fromSql = _EnumVariantFromSql<SubtitleAutoload>;
	} else	if (varType == qMetaTypeId<SubtitleAutoselect>()) {
		toSql = _EnumVariantToSql<SubtitleAutoselect>;
		fromSql = _EnumVariantFromSql<SubtitleAutoselect>;
	} else	if (varType == qMetaTypeId<OsdScalePolicy>()) {
		toSql = _EnumVariantToSql<OsdScalePolicy>;
		fromSql = _EnumVariantFromSql<OsdScalePolicy>;
	} else	if (varType == qMetaTypeId<ClickAction>()) {
		toSql = _EnumVariantToSql<ClickAction>;
		fromSql = _EnumVariantFromSql<ClickAction>;
	} else	if (varType == qMetaTypeId<WheelAction>()) {
		toSql = _EnumVariantToSql<WheelAction>;
		fromSql = _EnumVariantFromSql<WheelAction>;
	} else	if (varType == qMetaTypeId<KeyModifier>()) {
		toSql = _EnumVariantToSql<KeyModifier>;
		fromSql = _EnumVariantFromSql<KeyModifier>;
	} else	if (varType == qMetaTypeId<VerticalAlignment>()) {
		toSql = _EnumVariantToSql<VerticalAlignment>;
		fromSql = _EnumVariantFromSql<VerticalAlignment>;
	} else	if (varType == qMetaTypeId<HorizontalAlignment>()) {
		toSql = _EnumVariantToSql<HorizontalAlignment>;
		fromSql = _EnumVariantFromSql<HorizontalAlignment>;
	} else	if (varType == qMetaTypeId<MoveToward>()) {
		toSql = _EnumVariantToSql<MoveToward>;
		fromSql = _EnumVariantFromSql<MoveToward>;
	} else	if (varType == qMetaTypeId<ChangeValue>()) {
		toSql = _EnumVariantToSql<ChangeValue>;
		fromSql = _EnumVariantFromSql<ChangeValue>;
	} else
		return false;
	return true;
}
#endif
