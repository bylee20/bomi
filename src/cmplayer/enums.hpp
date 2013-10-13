
#ifndef ENUMS_HPP
#define ENUMS_HPP

#include <QCoreApplication>
#include <array>

template<typename T> class EnumInfo { static constexpr int size() { return 0; } double dummy; };

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
inline int operator | (DecoderDevice e1, DecoderDevice e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, DecoderDevice e) { return i |= (int)e; }
inline bool operator > (DecoderDevice e, int i) { return (int)e > i; }
inline bool operator < (DecoderDevice e, int i) { return (int)e < i; }
inline bool operator >= (DecoderDevice e, int i) { return (int)e >= i; }
inline bool operator <= (DecoderDevice e, int i) { return (int)e <= i; }
inline bool operator > (int i, DecoderDevice e) { return i > (int)e; }
inline bool operator < (int i, DecoderDevice e) { return i < (int)e; }
inline bool operator >= (int i, DecoderDevice e) { return i >= (int)e; }
inline bool operator <= (int i, DecoderDevice e) { return i <= (int)e; }

template<>
class EnumInfo<DecoderDevice> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef DecoderDevice Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 3; }
	static const char *name(Enum e) {
		auto it = std::find_if(info.cbegin(), info.cend(), [e](const Item &info) { return info.value == e; }); return it->name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::None: return tr("");
		case Enum::CPU: return tr("");
		case Enum::GPU: return tr("");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
private:
	static const std::array<Item, 3> info;
};

using DecoderDeviceInfo = EnumInfo<DecoderDevice>;

enum class DeintMode : int {
	Off = (int)0,
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
inline int operator | (DeintMode e1, DeintMode e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, DeintMode e) { return i |= (int)e; }
inline bool operator > (DeintMode e, int i) { return (int)e > i; }
inline bool operator < (DeintMode e, int i) { return (int)e < i; }
inline bool operator >= (DeintMode e, int i) { return (int)e >= i; }
inline bool operator <= (DeintMode e, int i) { return (int)e <= i; }
inline bool operator > (int i, DeintMode e) { return i > (int)e; }
inline bool operator < (int i, DeintMode e) { return i < (int)e; }
inline bool operator >= (int i, DeintMode e) { return i >= (int)e; }
inline bool operator <= (int i, DeintMode e) { return i <= (int)e; }

template<>
class EnumInfo<DeintMode> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef DeintMode Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 2; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::Off: return tr("");
		case Enum::Auto: return tr("");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 2> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
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
inline int operator | (DeintDevice e1, DeintDevice e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, DeintDevice e) { return i |= (int)e; }
inline bool operator > (DeintDevice e, int i) { return (int)e > i; }
inline bool operator < (DeintDevice e, int i) { return (int)e < i; }
inline bool operator >= (DeintDevice e, int i) { return (int)e >= i; }
inline bool operator <= (DeintDevice e, int i) { return (int)e <= i; }
inline bool operator > (int i, DeintDevice e) { return i > (int)e; }
inline bool operator < (int i, DeintDevice e) { return i < (int)e; }
inline bool operator >= (int i, DeintDevice e) { return i >= (int)e; }
inline bool operator <= (int i, DeintDevice e) { return i <= (int)e; }

template<>
class EnumInfo<DeintDevice> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef DeintDevice Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 4; }
	static const char *name(Enum e) {
		auto it = std::find_if(info.cbegin(), info.cend(), [e](const Item &info) { return info.value == e; }); return it->name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::None: return tr("");
		case Enum::CPU: return tr("");
		case Enum::GPU: return tr("");
		case Enum::OpenGL: return tr("");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 4> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
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
inline int operator | (DeintMethod e1, DeintMethod e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, DeintMethod e) { return i |= (int)e; }
inline bool operator > (DeintMethod e, int i) { return (int)e > i; }
inline bool operator < (DeintMethod e, int i) { return (int)e < i; }
inline bool operator >= (DeintMethod e, int i) { return (int)e >= i; }
inline bool operator <= (DeintMethod e, int i) { return (int)e <= i; }
inline bool operator > (int i, DeintMethod e) { return i > (int)e; }
inline bool operator < (int i, DeintMethod e) { return i < (int)e; }
inline bool operator >= (int i, DeintMethod e) { return i >= (int)e; }
inline bool operator <= (int i, DeintMethod e) { return i <= (int)e; }

template<>
class EnumInfo<DeintMethod> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef DeintMethod Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 8; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::None: return tr("");
		case Enum::Bob: return tr("");
		case Enum::LinearBob: return tr("");
		case Enum::CubicBob: return tr("");
		case Enum::Median: return tr("");
		case Enum::LinearBlend: return tr("");
		case Enum::Yadif: return tr("");
		case Enum::MotionAdaptive: return tr("");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 8> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
private:
	static const std::array<Item, 8> info;
};

using DeintMethodInfo = EnumInfo<DeintMethod>;

enum class InterpolatorType : int {
	Bilinear = (int)0,
	BicubicCR = (int)1,
	BicubicMN = (int)2,
	BicubicBS = (int)3,
	Lanczos2 = (int)4,
	Lanczos3Fast = (int)5
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
inline int operator | (InterpolatorType e1, InterpolatorType e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, InterpolatorType e) { return i |= (int)e; }
inline bool operator > (InterpolatorType e, int i) { return (int)e > i; }
inline bool operator < (InterpolatorType e, int i) { return (int)e < i; }
inline bool operator >= (InterpolatorType e, int i) { return (int)e >= i; }
inline bool operator <= (InterpolatorType e, int i) { return (int)e <= i; }
inline bool operator > (int i, InterpolatorType e) { return i > (int)e; }
inline bool operator < (int i, InterpolatorType e) { return i < (int)e; }
inline bool operator >= (int i, InterpolatorType e) { return i >= (int)e; }
inline bool operator <= (int i, InterpolatorType e) { return i <= (int)e; }

template<>
class EnumInfo<InterpolatorType> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef InterpolatorType Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 6; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::Bilinear: return tr("Bilinear interpolator");
		case Enum::BicubicCR: return tr("Catmull-Rom bicubic interpolator");
		case Enum::BicubicMN: return tr("Mitchell-Netravali bicubic interpolator");
		case Enum::BicubicBS: return tr("B-spline bicubic interpolator");
		case Enum::Lanczos2: return tr("Lanczos 2-lobed interpolator");
		case Enum::Lanczos3Fast: return tr("Lanczos approx. 3-lobed interpolator");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 6> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
private:
	static const std::array<Item, 6> info;
};

using InterpolatorTypeInfo = EnumInfo<InterpolatorType>;

enum class AudioDriver : int {
	Auto = (int)0,
	CoreAudio = (int)1,
	PulseAudio = (int)2,
	ALSA = (int)3,
	JACK = (int)4,
	PortAudio = (int)5,
	OpenAL = (int)6
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
inline int operator | (AudioDriver e1, AudioDriver e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, AudioDriver e) { return i |= (int)e; }
inline bool operator > (AudioDriver e, int i) { return (int)e > i; }
inline bool operator < (AudioDriver e, int i) { return (int)e < i; }
inline bool operator >= (AudioDriver e, int i) { return (int)e >= i; }
inline bool operator <= (AudioDriver e, int i) { return (int)e <= i; }
inline bool operator > (int i, AudioDriver e) { return i > (int)e; }
inline bool operator < (int i, AudioDriver e) { return i < (int)e; }
inline bool operator >= (int i, AudioDriver e) { return i >= (int)e; }
inline bool operator <= (int i, AudioDriver e) { return i <= (int)e; }

template<>
class EnumInfo<AudioDriver> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef AudioDriver Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 7; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::Auto: return tr("");
		case Enum::CoreAudio: return tr("");
		case Enum::PulseAudio: return tr("");
		case Enum::ALSA: return tr("");
		case Enum::JACK: return tr("");
		case Enum::PortAudio: return tr("");
		case Enum::OpenAL: return tr("");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 7> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
private:
	static const std::array<Item, 7> info;
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
inline int operator | (ClippingMethod e1, ClippingMethod e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, ClippingMethod e) { return i |= (int)e; }
inline bool operator > (ClippingMethod e, int i) { return (int)e > i; }
inline bool operator < (ClippingMethod e, int i) { return (int)e < i; }
inline bool operator >= (ClippingMethod e, int i) { return (int)e >= i; }
inline bool operator <= (ClippingMethod e, int i) { return (int)e <= i; }
inline bool operator > (int i, ClippingMethod e) { return i > (int)e; }
inline bool operator < (int i, ClippingMethod e) { return i < (int)e; }
inline bool operator >= (int i, ClippingMethod e) { return i >= (int)e; }
inline bool operator <= (int i, ClippingMethod e) { return i <= (int)e; }

template<>
class EnumInfo<ClippingMethod> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef ClippingMethod Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 3; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::Auto: return tr("Auto-clipping");
		case Enum::Soft: return tr("Soft-clipping");
		case Enum::Hard: return tr("Hard-clipping");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
private:
	static const std::array<Item, 3> info;
};

using ClippingMethodInfo = EnumInfo<ClippingMethod>;

enum class StaysOnTop : int {
	Always = (int)0,
	Playing = (int)1,
	Never = (int)2
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
inline int operator | (StaysOnTop e1, StaysOnTop e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, StaysOnTop e) { return i |= (int)e; }
inline bool operator > (StaysOnTop e, int i) { return (int)e > i; }
inline bool operator < (StaysOnTop e, int i) { return (int)e < i; }
inline bool operator >= (StaysOnTop e, int i) { return (int)e >= i; }
inline bool operator <= (StaysOnTop e, int i) { return (int)e <= i; }
inline bool operator > (int i, StaysOnTop e) { return i > (int)e; }
inline bool operator < (int i, StaysOnTop e) { return i < (int)e; }
inline bool operator >= (int i, StaysOnTop e) { return i >= (int)e; }
inline bool operator <= (int i, StaysOnTop e) { return i <= (int)e; }

template<>
class EnumInfo<StaysOnTop> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef StaysOnTop Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 3; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::Always: return tr("");
		case Enum::Playing: return tr("");
		case Enum::Never: return tr("");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
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
inline int operator | (SeekingStep e1, SeekingStep e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, SeekingStep e) { return i |= (int)e; }
inline bool operator > (SeekingStep e, int i) { return (int)e > i; }
inline bool operator < (SeekingStep e, int i) { return (int)e < i; }
inline bool operator >= (SeekingStep e, int i) { return (int)e >= i; }
inline bool operator <= (SeekingStep e, int i) { return (int)e <= i; }
inline bool operator > (int i, SeekingStep e) { return i > (int)e; }
inline bool operator < (int i, SeekingStep e) { return i < (int)e; }
inline bool operator >= (int i, SeekingStep e) { return i >= (int)e; }
inline bool operator <= (int i, SeekingStep e) { return i <= (int)e; }

template<>
class EnumInfo<SeekingStep> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef SeekingStep Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 3; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::Step1: return tr("");
		case Enum::Step2: return tr("");
		case Enum::Step3: return tr("");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
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
inline int operator | (GeneratePlaylist e1, GeneratePlaylist e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, GeneratePlaylist e) { return i |= (int)e; }
inline bool operator > (GeneratePlaylist e, int i) { return (int)e > i; }
inline bool operator < (GeneratePlaylist e, int i) { return (int)e < i; }
inline bool operator >= (GeneratePlaylist e, int i) { return (int)e >= i; }
inline bool operator <= (GeneratePlaylist e, int i) { return (int)e <= i; }
inline bool operator > (int i, GeneratePlaylist e) { return i > (int)e; }
inline bool operator < (int i, GeneratePlaylist e) { return i < (int)e; }
inline bool operator >= (int i, GeneratePlaylist e) { return i >= (int)e; }
inline bool operator <= (int i, GeneratePlaylist e) { return i <= (int)e; }

template<>
class EnumInfo<GeneratePlaylist> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef GeneratePlaylist Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 2; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::Similar: return tr("Add files which have similar names");
		case Enum::Folder: return tr("Add all files in the same folder");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 2> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
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
inline int operator | (PlaylistBehaviorWhenOpenMedia e1, PlaylistBehaviorWhenOpenMedia e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, PlaylistBehaviorWhenOpenMedia e) { return i |= (int)e; }
inline bool operator > (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e > i; }
inline bool operator < (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e < i; }
inline bool operator >= (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e >= i; }
inline bool operator <= (PlaylistBehaviorWhenOpenMedia e, int i) { return (int)e <= i; }
inline bool operator > (int i, PlaylistBehaviorWhenOpenMedia e) { return i > (int)e; }
inline bool operator < (int i, PlaylistBehaviorWhenOpenMedia e) { return i < (int)e; }
inline bool operator >= (int i, PlaylistBehaviorWhenOpenMedia e) { return i >= (int)e; }
inline bool operator <= (int i, PlaylistBehaviorWhenOpenMedia e) { return i <= (int)e; }

template<>
class EnumInfo<PlaylistBehaviorWhenOpenMedia> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef PlaylistBehaviorWhenOpenMedia Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 3; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::AppendToPlaylist: return tr("Append the open media to the playlist");
		case Enum::ClearAndAppendToPlaylist: return tr("Clear the playlist and append the open media to the playlist");
		case Enum::ClearAndGenerateNewPlaylist: return tr("Clear the playlist and generate new playlist");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
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
inline int operator | (SubtitleAutoload e1, SubtitleAutoload e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, SubtitleAutoload e) { return i |= (int)e; }
inline bool operator > (SubtitleAutoload e, int i) { return (int)e > i; }
inline bool operator < (SubtitleAutoload e, int i) { return (int)e < i; }
inline bool operator >= (SubtitleAutoload e, int i) { return (int)e >= i; }
inline bool operator <= (SubtitleAutoload e, int i) { return (int)e <= i; }
inline bool operator > (int i, SubtitleAutoload e) { return i > (int)e; }
inline bool operator < (int i, SubtitleAutoload e) { return i < (int)e; }
inline bool operator >= (int i, SubtitleAutoload e) { return i >= (int)e; }
inline bool operator <= (int i, SubtitleAutoload e) { return i <= (int)e; }

template<>
class EnumInfo<SubtitleAutoload> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef SubtitleAutoload Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 3; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::Matched: return tr("Subtitles which have the same name as that of playing file");
		case Enum::Contain: return tr("Subtitles whose names contain the name of playing file");
		case Enum::Folder: return tr("All subtitles in the folder where the playing file is located");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
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
inline int operator | (SubtitleAutoselect e1, SubtitleAutoselect e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, SubtitleAutoselect e) { return i |= (int)e; }
inline bool operator > (SubtitleAutoselect e, int i) { return (int)e > i; }
inline bool operator < (SubtitleAutoselect e, int i) { return (int)e < i; }
inline bool operator >= (SubtitleAutoselect e, int i) { return (int)e >= i; }
inline bool operator <= (SubtitleAutoselect e, int i) { return (int)e <= i; }
inline bool operator > (int i, SubtitleAutoselect e) { return i > (int)e; }
inline bool operator < (int i, SubtitleAutoselect e) { return i < (int)e; }
inline bool operator >= (int i, SubtitleAutoselect e) { return i >= (int)e; }
inline bool operator <= (int i, SubtitleAutoselect e) { return i <= (int)e; }

template<>
class EnumInfo<SubtitleAutoselect> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef SubtitleAutoselect Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 4; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::Matched: return tr("Subtitle which has the same name as that of playing file");
		case Enum::First: return tr("First subtitle from loaded ones");
		case Enum::All: return tr("All loaded subtitles");
		case Enum::EachLanguage: return tr("Each language subtitle");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 4> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
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
inline int operator | (OsdScalePolicy e1, OsdScalePolicy e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, OsdScalePolicy e) { return i |= (int)e; }
inline bool operator > (OsdScalePolicy e, int i) { return (int)e > i; }
inline bool operator < (OsdScalePolicy e, int i) { return (int)e < i; }
inline bool operator >= (OsdScalePolicy e, int i) { return (int)e >= i; }
inline bool operator <= (OsdScalePolicy e, int i) { return (int)e <= i; }
inline bool operator > (int i, OsdScalePolicy e) { return i > (int)e; }
inline bool operator < (int i, OsdScalePolicy e) { return i < (int)e; }
inline bool operator >= (int i, OsdScalePolicy e) { return i >= (int)e; }
inline bool operator <= (int i, OsdScalePolicy e) { return i <= (int)e; }

template<>
class EnumInfo<OsdScalePolicy> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef OsdScalePolicy Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 3; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::Width: return tr("Fit to width of video");
		case Enum::Height: return tr("Fit to height of video");
		case Enum::Diagonal: return tr("Fit to diagonal of video");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 3> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
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
inline int operator | (ClickAction e1, ClickAction e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, ClickAction e) { return i |= (int)e; }
inline bool operator > (ClickAction e, int i) { return (int)e > i; }
inline bool operator < (ClickAction e, int i) { return (int)e < i; }
inline bool operator >= (ClickAction e, int i) { return (int)e >= i; }
inline bool operator <= (ClickAction e, int i) { return (int)e <= i; }
inline bool operator > (int i, ClickAction e) { return i > (int)e; }
inline bool operator < (int i, ClickAction e) { return i < (int)e; }
inline bool operator >= (int i, ClickAction e) { return i >= (int)e; }
inline bool operator <= (int i, ClickAction e) { return i <= (int)e; }

template<>
class EnumInfo<ClickAction> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef ClickAction Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 4; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::OpenFile: return tr("Open a file");
		case Enum::Fullscreen: return tr("Toggle fullscreen mode");
		case Enum::Pause: return tr("Toggle play/pause");
		case Enum::Mute: return tr("Toggle mute/unmute");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 4> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
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
inline int operator | (WheelAction e1, WheelAction e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, WheelAction e) { return i |= (int)e; }
inline bool operator > (WheelAction e, int i) { return (int)e > i; }
inline bool operator < (WheelAction e, int i) { return (int)e < i; }
inline bool operator >= (WheelAction e, int i) { return (int)e >= i; }
inline bool operator <= (WheelAction e, int i) { return (int)e <= i; }
inline bool operator > (int i, WheelAction e) { return i > (int)e; }
inline bool operator < (int i, WheelAction e) { return i < (int)e; }
inline bool operator >= (int i, WheelAction e) { return i >= (int)e; }
inline bool operator <= (int i, WheelAction e) { return i <= (int)e; }

template<>
class EnumInfo<WheelAction> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef WheelAction Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 6; }
	static const char *name(Enum e) {
		return info[(int)e].name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::Seek1: return tr("Seek playback for step 1");
		case Enum::Seek2: return tr("Seek playback for step 2");
		case Enum::Seek3: return tr("Seek playback for step 3");
		case Enum::PrevNext: return tr("Play previous/next");
		case Enum::Volume: return tr("Volumn up/down");
		case Enum::Amp: return tr("Amp. up/down");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 6> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
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
inline int operator | (KeyModifier e1, KeyModifier e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, KeyModifier e) { return i |= (int)e; }
inline bool operator > (KeyModifier e, int i) { return (int)e > i; }
inline bool operator < (KeyModifier e, int i) { return (int)e < i; }
inline bool operator >= (KeyModifier e, int i) { return (int)e >= i; }
inline bool operator <= (KeyModifier e, int i) { return (int)e <= i; }
inline bool operator > (int i, KeyModifier e) { return i > (int)e; }
inline bool operator < (int i, KeyModifier e) { return i < (int)e; }
inline bool operator >= (int i, KeyModifier e) { return i >= (int)e; }
inline bool operator <= (int i, KeyModifier e) { return i <= (int)e; }

template<>
class EnumInfo<KeyModifier> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef KeyModifier Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 4; }
	static const char *name(Enum e) {
		auto it = std::find_if(info.cbegin(), info.cend(), [e](const Item &info) { return info.value == e; }); return it->name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::None: return tr("");
		case Enum::Ctrl: return tr("");
		case Enum::Shift: return tr("");
		case Enum::Alt: return tr("");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 4> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
private:
	static const std::array<Item, 4> info;
};

using KeyModifierInfo = EnumInfo<KeyModifier>;

enum class Position : int {
	CC = (int)Qt::AlignVCenter|Qt::AlignHCenter,
	TL = (int)Qt::AlignTop|Qt::AlignLeft,
	TC = (int)Qt::AlignTop|Qt::AlignHCenter,
	TR = (int)Qt::AlignTop|Qt::AlignRight,
	CL = (int)Qt::AlignVCenter|Qt::AlignLeft,
	CR = (int)Qt::AlignVCenter|Qt::AlignRight,
	BL = (int)Qt::AlignBottom|Qt::AlignLeft,
	BC = (int)Qt::AlignBottom|Qt::AlignHCenter,
	BR = (int)Qt::AlignBottom|Qt::AlignRight
};

inline bool operator == (Position e, int i) { return (int)e == i; }
inline bool operator != (Position e, int i) { return (int)e != i; }
inline bool operator == (int i, Position e) { return (int)e == i; }
inline bool operator != (int i, Position e) { return (int)e != i; }
inline int operator & (Position e, int i) { return (int)e & i; }
inline int operator & (int i, Position e) { return (int)e & i; }
inline int &operator &= (int &i, Position e) { return i &= (int)e; }
inline int operator ~ (Position e) { return ~(int)e; }
inline int operator | (Position e, int i) { return (int)e | i; }
inline int operator | (int i, Position e) { return (int)e | i; }
inline int operator | (Position e1, Position e2) { return (int)e1 | (int)e2; }
inline int &operator |= (int &i, Position e) { return i |= (int)e; }
inline bool operator > (Position e, int i) { return (int)e > i; }
inline bool operator < (Position e, int i) { return (int)e < i; }
inline bool operator >= (Position e, int i) { return (int)e >= i; }
inline bool operator <= (Position e, int i) { return (int)e <= i; }
inline bool operator > (int i, Position e) { return i > (int)e; }
inline bool operator < (int i, Position e) { return i < (int)e; }
inline bool operator >= (int i, Position e) { return i >= (int)e; }
inline bool operator <= (int i, Position e) { return i <= (int)e; }

template<>
class EnumInfo<Position> {
	Q_DECLARE_TR_FUNCTIONS(EnumInfo)
	typedef Position Enum;
public:
	struct Item { Enum value; const char *name; };
	static constexpr int size() { return 9; }
	static const char *name(Enum e) {
		auto it = std::find_if(info.cbegin(), info.cend(), [e](const Item &info) { return info.value == e; }); return it->name;
	}
	static QString description(Enum e) {
		switch (e) {
		case Enum::CC: return tr("");
		case Enum::TL: return tr("");
		case Enum::TC: return tr("");
		case Enum::TR: return tr("");
		case Enum::CL: return tr("");
		case Enum::CR: return tr("");
		case Enum::BL: return tr("");
		case Enum::BC: return tr("");
		case Enum::BR: return tr("");
		default: return tr("");
		};
	}
	static constexpr const std::array<Item, 9> &items() { return info; }
	static Enum from(int id, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [id] (const Item &item) { return item.value == id; });
		return it != info.cend() ? it->value : def;
	}
	static Enum from(const QString &name, Enum def = info[0].value) {
		auto it = std::find_if(info.cbegin(), info.cend(), [name] (const Item &item) { return !name.compare(QLatin1String(item.name));});
		return it != info.cend() ? it->value : def;
	}
private:
	static const std::array<Item, 9> info;
};

using PositionInfo = EnumInfo<Position>;

#endif
