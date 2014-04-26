#include "videocolor.hpp"
#include "enums.hpp"
#include <QVector3D>
#include <QtMath>

struct YCbCrRange {
    YCbCrRange &operator *= (float rhs) { y1 *= rhs; y2 *= rhs;  c1 *= rhs; c2 *= rhs; return *this; }
    YCbCrRange operator * (float rhs) const { return YCbCrRange(*this) *= rhs; }
    float y1, y2, c1, c2;
};

static QMatrix3x3 matYCbCrToRgb(double kb, double kr, const YCbCrRange &range) {
    const double dy = 1.0/(range.y2-range.y1);
    const double dc = 2.0/(range.c2-range.c1);
    const double kg = 1.0 - kb - kr;
    QMatrix3x3 mat;
    mat(0, 0) = dy; mat(0, 1) = 0.0;                mat(0, 2) = (1.0 - kr)*dc;
    mat(1, 0) = dy; mat(1, 1) = -dc*(1.0-kb)*kb/kg; mat(1, 2) = -dc*(1.0-kr)*kr/kg;
    mat(2, 0) = dy; mat(2, 1) = dc*(1-kb);          mat(2, 2) = 0.0;
    return mat;
}

static QMatrix3x3 matRgbToYCbCr(double kb, double kr, const YCbCrRange &range) {
    const double dy = (range.y2-range.y1);
    const double dc = (range.c2-range.c1)/2.0;
    const double kg = 1.0 - kb - kr;
    QMatrix3x3 mat;
    mat(0, 0) = dy*kr;              mat(0, 1) = dy*kg;              mat(0, 2) = dy*kb;
    mat(1, 0) = -dc*kr/(1.0 - kb);  mat(1, 1) = -dc*kg/(1.0-kb);    mat(1, 2) = dc;
    mat(2, 0) = dc;                 mat(2, 1) = -dc*kg/(1.0-kr);    mat(2, 2) = -dc*kb/(1.0 - kr);
    return mat;
}

static QMatrix3x3 matSHC(double s, double h, double c) {
    s = qBound(0.0, s + 1.0, 2.0);
    c = qBound(0.0, c + 1.0, 2.0);
    h = qBound(-M_PI, h*M_PI, M_PI);
    QMatrix3x3 mat;
    mat(0, 0) = 1.0;    mat(0, 1) = 0.0;        mat(0, 2) = 0.0;
    mat(1, 0) = 0.0;    mat(1, 1) = s*qCos(h);  mat(1, 2) = s*qSin(h);
    mat(2, 0) = 0.0;    mat(2, 1) = -s*qSin(h); mat(2, 2) = s*qCos(h);
    mat *= c;
    return mat;
}

                        //  y1,y2,c1,c2
const YCbCrRange ranges[5] = {
    {  0.f, 255.f,  0.f, 255.f}, // Auto
    { 16.f, 235.f, 16.f, 240.f}, // Limited
    {  0.f, 255.f,  0.f, 255.f}, // Full
    {  0.f, 255.f,  0.f, 255.f}, // Remap
    {  0.f, 255.f,  0.f, 255.f}  // Extended
};

const float specs[MP_CSP_COUNT][2] = {
    {0.0000, 0.0000}, // MP_CSP_AUTO,
    {0.1140, 0.2990}, // MP_CSP_BT_601,
    {0.0722, 0.2126}, // MP_CSP_BT_709,
    {0.0870, 0.2120}  // MP_CSP_SMPTE_240M,
};

using ColumnVector3 = QGenericMatrix<1, 3, float>;

static ColumnVector3 make3x1(float v1, float v2, float v3) {
    ColumnVector3 vec;
    vec(0, 0) = v1; vec(1, 0) = v2; vec(2, 0) = v3;
    return vec;
}

static ColumnVector3 make3x1(float v1, float v23) {
    return make3x1(v1, v23, v23);
}

void VideoColor::matrix(QMatrix3x3 &mul, QVector3D &add, mp_csp colorspace, ColorRange cr, float s) const {
    mul.setToIdentity();
    add = {0.f, 0.f, 0.f};
    const float *spec = specs[colorspace];
    auto range = ranges[(int)cr];
    switch (colorspace) {
    case MP_CSP_RGB:
        spec = specs[MP_CSP_BT_601];
        range = ranges[(int)ColorRange::Full];
    case MP_CSP_BT_601:
    case MP_CSP_BT_709:
    case MP_CSP_SMPTE_240M:
        break;
    default:
        return;
    }
    range *= s;
    const float kb = spec[0], kr = spec[1];
    const auto ycbcrFromRgb = matRgbToYCbCr(kb, kr, range);
    const auto rgbFromYCbCr = matYCbCrToRgb(kb, kr, range);
    const auto shc = matSHC(saturation()*1e-2, hue()*1e-2, contrast()*1e-2);
    auto bvec = make3x1(qBound(-1.0, brightness()*1e-2, 1.0), 0);

    mul = rgbFromYCbCr*shc;
    if (colorspace == MP_CSP_RGB) {
        mul = mul*ycbcrFromRgb;
    } else {
        auto sub = make3x1(range.y1, (range.c1 + range.c2)/2.0f);
        const auto tv = ranges[(int)ColorRange::Limited]*s;
        const auto pc = ranges[(int)ColorRange::Full]*s;
        if (cr == ColorRange::Remap) {
            QMatrix3x3 scaler;
            scaler(0, 0) = (pc.y2 - pc.y1)/(tv.y2 - tv.y1);
            scaler(1, 1) = scaler(2, 2) = (pc.c2 - pc.c1)/(tv.c2 - tv.c1);
            mul = mul*scaler;
            sub += scaler*make3x1(tv.y1, tv.c1);
        } else if (cr == ColorRange::Extended) {
            QMatrix3x3 scaler;
            scaler(0, 0) = scaler(1, 1) = scaler(2, 2) = (pc.y2 - pc.y1)/(tv.y2 - tv.y1);
            mul = mul*scaler;
            sub += scaler*make3x1(tv.y1, tv.y1);
        }
        bvec -= shc*sub;
    }
    bvec = rgbFromYCbCr*bvec;
    add = {bvec(0, 0), bvec(1, 0), bvec(2, 0)};
}

qint64 VideoColor::packed() const {
#define PACK(p, s) ((0xffu & ((quint32)p() + 100u)) << s)
    return PACK(brightness, 24) | PACK(contrast, 16) | PACK(saturation, 8) | PACK(hue, 0);
#undef PACK
}

VideoColor VideoColor::fromPacked(qint64 packed) {
#define UNPACK(s) (((packed >> s) & 0xff) - 100)
    return VideoColor(UNPACK(24), UNPACK(16), UNPACK(8), UNPACK(0));
#undef UNPACK
}
