#ifndef COLORPROPERTY_HPP
#define COLORPROPERTY_HPP

#include <array>
#include <QString>

enum class ColorRange;                  enum class ColorSpace;
class QMatrix4x4;

class VideoColor {
public:
    enum Type {Brightness = 0, Contrast, Hue, Saturation, Red, Green, Blue, TypeMax};
    template<class T> using Array = std::array<T, TypeMax>;
//    VideoColor(int b, int c, int s, int h): m{{b, c, s, h}} {}
    VideoColor() = default;
    auto operator == (const VideoColor &r) const -> bool { return m == r.m; }
    auto operator != (const VideoColor &r) const -> bool { return m != r.m; }
    auto operator *= (int rhs) -> VideoColor&;
    auto operator * (int rhs) const -> VideoColor
        { return VideoColor(*this) *= rhs; }
    auto operator & (const VideoColor &rhs) const -> Type;
    auto operator += (const VideoColor &rhs) -> VideoColor&;
    auto operator + (const VideoColor &rhs) const -> VideoColor
    { return VideoColor(*this) += rhs; }
    auto operator [] (Type p) const -> int { return m[p]; }
    auto get(Type v) const -> int { return m[v]; }
    auto brightness() const -> int { return m[Brightness]; }
    auto saturation() const -> int { return m[Saturation]; }
    auto contrast() const -> int { return m[Contrast]; }
    auto hue() const -> int { return m[Hue]; }
    auto red() const -> int { return m[Red]; }
    auto green() const -> int { return m[Green]; }
    auto blue() const -> int { return m[Blue]; }
    auto set(Type p, int val) -> void { m[p] = clip(val); }
    auto add(Type p, int diff) -> void { m[p] = clip(m[p] + diff); }
    auto setBrightness(int v) -> void { m[Brightness] = clip(v); }
    auto setSaturation(int v) -> void { m[Saturation] = clip(v); }
    auto setContrast(int v) -> void { m[Contrast] = clip(v); }
    auto setHue(int v) -> void { m[Hue] = clip(v); }
    auto isZero() const -> bool;
    auto matrix() const -> QMatrix4x4;
    auto getText(Type type) const -> QString;
    auto packed() const -> qint64;
    auto toString() const -> QString;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    auto description() const -> QString;
    static auto description(Type type) -> QString;
    static auto fromJson(const QJsonObject &json) -> VideoColor
        { VideoColor c; c.setFromJson(json); return c; }
    static auto fromString(const QString &str) -> VideoColor;
    static auto fromPacked(qint64 packed) -> VideoColor;
    static auto getType(const char *name) -> Type;
    static auto getType(const QString &name) -> Type;
    static auto name(Type type) -> QString
        { return type < TypeMax ? s_names[type] : QString(); }
    static auto formatText(Type type) -> QString;
    template<class F>
    static auto for_type(F func) -> void;
private:
    auto matRgbChannel() const -> QMatrix4x4;
    auto matBSHC() const -> QMatrix4x4;
    auto matYCbCrToRgb(ColorSpace c, ColorRange r) const -> QMatrix4x4;
    static auto clip(int v) -> int { return qBound(-100, v, 100); }
    static Array<QString> s_names;
    Array<int> m{{0, 0, 0, 0, 0, 0, 0}};

};

Q_DECLARE_METATYPE(VideoColor)

inline auto VideoColor::operator & (const VideoColor &rhs) const -> Type
{
    int count = 0;
    Type type = TypeMax;
    for (uint i=0; i<m.size(); ++i) {
        if (m[i] != rhs.m[i]) {
            ++count;
            type = static_cast<Type>(i);
        }
    }
    return count == 1 ? type : TypeMax;
}

inline auto VideoColor::operator *= (int rhs) -> VideoColor&
{
    for (int i = 0; i < TypeMax; ++i)
        m[i] *= rhs;
    return *this;
}

inline auto VideoColor::operator += (const VideoColor &rhs) -> VideoColor&
{
    for (int i = 0; i < TypeMax; ++i)
        m[i] += rhs.m[i];
    return *this;
}

inline auto VideoColor::isZero() const -> bool
{
    for (int i = 0; i < TypeMax; ++i) {
        if (m[i])
            return false;
    }
    return true;
}

template<class F>
inline auto VideoColor::for_type(F f) -> void
{
    for (int i = 0; i < TypeMax; ++i)
        f(static_cast<Type>(i));
}

inline auto VideoColor::getType(const char *name) -> Type
{
    return getType(QString::fromLatin1(name));
}


inline auto VideoColor::getType(const QString &name) -> Type
{
    const auto it = std::find(s_names.begin(), s_names.end(), name);
    return it != s_names.end() ? Type(it - s_names.begin()) : TypeMax;
}

#endif // COLORPROPERTY_HPP
