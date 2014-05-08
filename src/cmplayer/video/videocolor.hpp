#ifndef COLORPROPERTY_HPP
#define COLORPROPERTY_HPP

#include <QObject>
#include <QGenericMatrix>
#include <QMatrix4x4>
extern "C" {
#include <video/csputils.h>
}

enum class ColorRange;

struct Kernel3x3 {
    Kernel3x3() { mat(0, 0) = mat(2, 2) = 0.f; }
    auto operator = (const Kernel3x3 &rhs) -> Kernel3x3&
        { mat = rhs.mat; return *this; }
    auto operator () (int i, int j) -> float& { return at(i, j); }
    auto operator () (int i, int j) const -> float {return at(i, j); }
    auto operator += (const Kernel3x3 &rhs) -> Kernel3x3&
        { mat += rhs.mat; return *this; }
    auto operator + (const Kernel3x3 &rhs) const -> Kernel3x3
        { Kernel3x3 lhs = *this; return (lhs += rhs); }
    auto operator == (const Kernel3x3 &rhs) const -> bool
        { return mat == rhs.mat; }
    auto operator != (const Kernel3x3 &rhs) const -> bool
        { return !operator == (rhs); }
    auto merged(const Kernel3x3 &other, bool normalize = true) -> Kernel3x3
    {
        Kernel3x3 ret = *this + other;
        if (normalize)
            ret.normalize();
        return ret;
    }
    auto normalize() -> void
    {
        float den = 0.0;
        for (int i=0; i<9; ++i)
            den += mat.data()[i];
        mat /= den;
    }
    auto normalized() const -> Kernel3x3
    {
        Kernel3x3 kernel = *this;
        kernel.normalize();
        return kernel;
    }
    float &at(int i, int j) { return mat(i, j); }
    auto at(int i, int j) const -> float {return mat(i, j); }
    auto setCenter(float v)-> void { mat(1, 1) = v; }
    auto setNeighbor(float v)-> void
        { mat(0, 1) = mat(1, 0) = mat(1, 2) = mat(2, 1) = v; }
    auto setDiagonal(float v)-> void
        { mat(0, 0) = mat(0, 2) = mat(2, 0) = mat(2, 2) = v;; }
    auto center() const -> float {return mat(1, 1);}
    auto neighbor() const -> float {return mat(0, 1);}
    auto diagonal() const -> float {return mat(0, 0);}
    auto matrix() const -> QMatrix3x3 {return mat;}
private:
    QMatrix3x3 mat;
};

class VideoColor {
    Q_DECLARE_TR_FUNCTIONS(VideoColor)
public:
    enum Type {Brightness = 0, Contrast, Saturation, Hue, TypeMax};
    template<class T> using Array = std::array<T, TypeMax>;
    VideoColor(int b, int c, int s, int h): m{{b, c, s, h}} {}
    VideoColor() = default;
    auto operator == (const VideoColor &r) const -> bool { return m == r.m; }
    auto operator != (const VideoColor &r) const -> bool { return m != r.m; }
    auto operator *= (int rhs) -> VideoColor&
        { m[0] *= rhs; m[1] *= rhs; m[2] *= rhs; m[3] *= rhs; return *this; }
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
    auto set(Type p, int val) -> void { m[p] = clip(val); }
    auto setBrightness(int v) -> void { m[Brightness] = clip(v); }
    auto setSaturation(int v) -> void { m[Saturation] = clip(v); }
    auto setContrast(int v) -> void { m[Contrast] = clip(v); }
    auto setHue(int v) -> void { m[Hue] = clip(v); }
    auto isZero() const -> bool
        { return !m[Brightness] && !m[Saturation] && !m[Contrast] && !m[Hue]; }
    auto matrix(QMatrix3x3 &mul, QVector3D &add, mp_csp colorspace,
                ColorRange range, float s = 1.0/255.0) const -> void;
    auto getText(Type type) const -> QString;
    auto packed() const -> qint64;
    static auto fromPacked(qint64 packed) -> VideoColor;
    static auto getType(const char *name) -> Type;
    static auto name(Type type) -> const char*
        { return type < TypeMax ? s_names[type] : ""; }
    template<class F>
    static auto for_type(F func) -> void;
private:
    static auto clip(int v) -> int { return qBound(-100, v, 100); }
    static Array<const char*> s_names;
    Array<int> m{{0, 0, 0, 0}};

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

inline auto VideoColor::operator += (const VideoColor &rhs) -> VideoColor&
{
    m[0] += rhs.m[0]; m[1] += rhs.m[1];
    m[2] += rhs.m[2]; m[3] += rhs.m[3]; return *this;
}

template<class F>
inline auto VideoColor::for_type(F f) -> void
{
    for (int i = 0; i < TypeMax; ++i)
        f(static_cast<Type>(i));
}

inline auto VideoColor::getType(const char *name) -> Type
{
    for (int i = 0; i < TypeMax; ++i) {
        if (!strcmp(name, s_names[i]))
            return static_cast<Type>(i);
    }
    return TypeMax;
}

#endif // COLORPROPERTY_HPP
