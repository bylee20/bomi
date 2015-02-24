#include "videocolor.hpp"
#include "enum/colorrange.hpp"
#include "enum/colorspace.hpp"
#include "misc/json.hpp"
#include "misc/log.hpp"
#include <QVector3D>
#include <QMatrix4x4>

static auto makeNameArray() -> VideoColor::Array<QString>
{
    VideoColor::Array<QString> names;
    names[VideoColor::Brightness] = u"brightness"_q;
    names[VideoColor::Contrast]   = u"contrast"_q;
    names[VideoColor::Saturation] = u"saturation"_q;
    names[VideoColor::Hue]        = u"hue"_q;
    names[VideoColor::Red]        = u"red"_q;
    names[VideoColor::Green]      = u"green"_q;
    names[VideoColor::Blue]       = u"blue"_q;
    return names;
}

VideoColor::Array<QString> VideoColor::s_names = makeNameArray();

struct YCbCrRange {
    auto operator *= (float rhs) -> YCbCrRange&
        { y1 *= rhs; y2 *= rhs;  c1 *= rhs; c2 *= rhs; return *this; }
    auto operator * (float rhs) const -> YCbCrRange
        { return YCbCrRange(*this) *= rhs; }
    float y1, y2, c1, c2;
    auto base() const -> QVector3D
        { return { y1, (c1 + c2)*.5f, (c1 + c2)*.5f }; }
};

//  y1,y2,c1,c2
const YCbCrRange ranges[5] = {
{  0.f/255.f, 255.f/255.f,  0.f/255.f, 255.f/255.f}, // Auto
{ 16.f/255.f, 235.f/255.f, 16.f/255.f, 240.f/255.f}, // Limited
{  0.f/255.f, 255.f/255.f,  0.f/255.f, 255.f/255.f}, // Full
{  0.f/255.f, 255.f/255.f,  0.f/255.f, 255.f/255.f}, // Remap
{  0.f/255.f, 255.f/255.f,  0.f/255.f, 255.f/255.f}  // Extended
};

const float specs[6][2] = {
{0.0000, 0.0000}, // Auto
{0.1140, 0.2990}, // BT601
{0.0722, 0.2126}, // BT709
{0.0870, 0.2120}  // SMPTE240M
};

auto VideoColor::matYCbCrToRgb(ColorSpace c, ColorRange r) const -> QMatrix4x4
{
    QMatrix4x4 scaler;

    const auto kb = specs[(int)c][0], kr = specs[(int)c][1];
    const auto &range = ranges[(int)r];

    const auto dy = 1.f/(range.y2-range.y1);
    const auto dc = 2.f/(range.c2-range.c1);
    const auto kg = 1.f - kb - kr;

    QMatrix4x4 coef;
    const auto m11 = -dc * (1.f - kb) * kb / kg;
    const auto m12 = -dc * (1.f - kr) * kr / kg;
    coef(0, 0) = dy; coef(0, 1) = 0.0;           coef(0, 2) = (1.0 - kr)*dc;
    coef(1, 0) = dy; coef(1, 1) = m11;           coef(1, 2) = m12;
    coef(2, 0) = dy; coef(2, 1) = dc * (1 - kb); coef(2, 2) = 0.0;

    QMatrix4x4 baseSub;
    baseSub.setColumn(3, { -range.base(), 1.0 });

    QMatrix4x4 filter = matBSHC();

    return coef*filter*baseSub*scaler;
}

SIA matRgbToYCbCr(ColorSpace c = ColorSpace::BT601,
                  ColorRange r = ColorRange::Limited) -> QMatrix4x4
{
    const auto kb = specs[(int)c][0], kr = specs[(int)c][1];
    const auto &range = ranges[(int)r];
    const auto dy = (range.y2-range.y1);
    const auto dc = (range.c2-range.c1)/2.0f;
    const auto kg = 1.0f - kb - kr;

    const double m10 = -dc*kr/(1.0 - kb);
    const double m22 = -dc*kb/(1.0 - kr);
    QMatrix4x4 mat;
    mat(0, 0) = dy * kr; mat(0, 1) = dy * kg;         mat(0, 2) = dy * kb;
    mat(1, 0) = m10;     mat(1, 1) = -dc*kg/(1.0-kb); mat(1, 2) = dc;
    mat(2, 0) = dc;      mat(2, 1) = -dc*kg/(1.0-kr); mat(2, 2) = m22;

    QMatrix4x4 add;
    add.setColumn(3, { range.base(), 1.0 });

    return add*mat;
}

auto VideoColor::matBSHC() const -> QMatrix4x4
{
    const auto b = qBound(-1.0, brightness() * 1e-2, 1.0);
    const auto s = qBound(0.0, saturation() * 1e-2 + 1.0, 2.0);
    const auto c = qBound(0.0, contrast() * 1e-2 + 1.0, 2.0);
    const auto h = qBound(-M_PI, hue() * 1e-2 * M_PI, M_PI);
    QMatrix4x4 mat;
    mat(0, 0) =   c; mat(0, 1) = 0.0;          mat(0, 2) = 0.0;
    mat(1, 0) = 0.0; mat(1, 1) =  c*s*qCos(h); mat(1, 2) = c*s*qSin(h);
    mat(2, 0) = 0.0; mat(2, 1) = -c*s*qSin(h); mat(2, 2) = c*s*qCos(h);
    mat(0, 3) = b;
    return mat;
}

auto VideoColor::matRgbChannel() const -> QMatrix4x4
{
    const float r = qBound(-1.0, red() * 1e-2, 1.0);
    const float g = qBound(-1.0, green() * 1e-2, 1.0);
    const float b = qBound(-1.0, blue() * 1e-2, 1.0);
    QMatrix4x4 mat;
    mat.setColumn(3, {r, g, b, 1.f});
    return mat;
}

auto VideoColor::matrix() const -> QMatrix4x4
{
    if (isZero())
        return QMatrix4x4();
    const auto rgbFromYCbCr = matYCbCrToRgb(ColorSpace::BT601, ColorRange::Full);
    const auto toYCbCr = matRgbToYCbCr(ColorSpace::BT601, ColorRange::Full);
    return matRgbChannel()*rgbFromYCbCr*toYCbCr;
}

//auto VideoColor::packed() const -> qint64
//{
//#define PACK(p, s) ((0xffu & ((quint32)p() + 100u)) << s)
//    return PACK(brightness, 24) | PACK(contrast, 16)
//           | PACK(saturation, 8) | PACK(hue, 0);
//#undef PACK
//}

//auto VideoColor::fromPacked(qint64 packed) -> VideoColor
//{
//#define UNPACK(s) (((packed >> s) & 0xff) - 100)
//    return VideoColor(UNPACK(24), UNPACK(16), UNPACK(8), UNPACK(0));
//#undef UNPACK
//}

auto VideoColor::formatText(Type type) -> QString
{
    switch (type) {
    case Brightness:
        return qApp->translate("VideoColor", "Brightness %1");
    case Saturation:
        return qApp->translate("VideoColor", "Saturation %1");
    case Contrast:
        return qApp->translate("VideoColor", "Contrast %1");
    case Hue:
        return qApp->translate("VideoColor", "Hue %1");
    case Red:
        return qApp->translate("VideoColor", "Red %1");
    case Green:
        return qApp->translate("VideoColor", "Green %1");
    case Blue:
        return qApp->translate("VideoColor", "Blue %1");
    default:
        return qApp->translate("VideoColor", "Reset");
    }
}

auto VideoColor::description(Type type) -> QString
{
    switch (type) {
    case Brightness:
        return qApp->translate("VideoColor", "Brightness");
    case Saturation:
        return qApp->translate("VideoColor", "Saturation");
    case Contrast:
        return qApp->translate("VideoColor", "Contrast");
    case Hue:
        return qApp->translate("VideoColor", "Hue");
    case Red:
        return qApp->translate("VideoColor", "Red");
    case Green:
        return qApp->translate("VideoColor", "Green");
    case Blue:
        return qApp->translate("VideoColor", "Blue");
    default:
        return qApp->translate("VideoColor", "Reset");
    }
}

auto VideoColor::description() const -> QString
{
    return qApp->translate("VideoColor", "B: %1%, C: %2%, H: %3%, S: %4%, "
                                         "R:%5%, G: %6%, B: %7%")
            .arg(brightness()).arg(contrast()).arg(hue()).arg(saturation())
            .arg(red()).arg(green()).arg(blue());
}

auto VideoColor::getText(Type type) const -> QString
{
    const auto format = formatText(type);
    return _InRange0(type, TypeMax) ? format.arg(_NS(m[type], true)) : format;
}

auto VideoColor::toString() const -> QString
{
    QStringList strs;
    VideoColor::for_type([&] (VideoColor::Type type) {
        strs.append(name(type) % '='_q % _N(get(type)));
    });
    return strs.join('|'_q);
}

auto VideoColor::fromString(const QString &str) -> VideoColor
{
    const auto strs = str.split('|'_q);
    QRegEx regex(uR"((\w+)=(\d+))"_q);
    VideoColor color;
    for (auto &one : strs) {
        auto match = regex.match(one);
        if (!match.hasMatch())
            return VideoColor();
        auto type = getType(match.captured(1));
        if (type == TypeMax)
            return VideoColor();
        color.set(type, match.captured(2).toInt());
    }
    return color;
}

auto VideoColor::toJson() const -> QJsonObject
{
    QJsonObject json;
    for_type([&] (Type type) { json.insert(name(type), get(type)); });
    return json;
}

auto VideoColor::setFromJson(const QJsonObject &json) -> bool
{
    bool ok = true;
    for_type([&] (Type type) {
        const auto it = json.find(name(type));
        if (it == json.end())
            ok = false;
        else
            set(type, (*it).toInt());
    });
    return ok;
}
