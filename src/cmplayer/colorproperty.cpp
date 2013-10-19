#include "colorproperty.hpp"
#include <QVector3D>
#include <QtMath>

static QMatrix3x3 matYCbCrToRgb(double kb, double kr, double y1, double y2, double c1, double c2) {
	const double dy = 1.0/(y2-y1);
	const double dc = 2.0/(c2-c1);
	const double kg = 1.0 - kb - kr;
	QMatrix3x3 mat;
	mat(0, 0) = dy; mat(0, 1) = 0.0;                mat(0, 2) = (1.0 - kr)*dc;
	mat(1, 0) = dy; mat(1, 1) = -dc*(1.0-kb)*kb/kg; mat(1, 2) = -dc*(1.0-kr)*kr/kg;
	mat(2, 0) = dy; mat(2, 1) = dc*(1-kb);          mat(2, 2) = 0.0;
	return mat;
}

static QMatrix3x3 matRgbToYCbCr(double kb, double kr, double y1, double y2, double c1, double c2) {
	const double dy = (y2-y1);
	const double dc = (c2-c1)/2.0;
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
const float ranges[MP_CSP_LEVELS_COUNT][4] = {
	{	     0.f,         1.f,        0.f,         1.f}, //MP_CSP_LEVELS_AUTO
	{ 16.f/255.f, 235.f/255.f,  16./255.f, 240.f/255.f}, // MP_CSP_LEVELS_TV
	{        0.f,         1.f,        0.f,         1.f}  // MP_CSP_LEVELS_PC
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

void ColorProperty::matrix(QMatrix3x3 &mul, QVector3D &add, mp_csp colorspace, mp_csp_levels levels) const {
	mul.setToIdentity();
	add = {0.f, 0.f, 0.f};
	const float *spec = specs[colorspace];
	const float *range = ranges[levels];
	switch (colorspace) {
	case MP_CSP_RGB:
		spec = specs[MP_CSP_BT_601];
		range = ranges[MP_CSP_LEVELS_TV];
	case MP_CSP_BT_601:
	case MP_CSP_BT_709:
	case MP_CSP_SMPTE_240M:
		break;
	default:
		return;
	}
	switch (levels) {
	case MP_CSP_LEVELS_TV:
	case MP_CSP_LEVELS_PC:
		break;
	default:
		return;
	}
	const float kb = spec[0], kr = spec[1];
	const float y1 = range[0], y2 = range[1], c1 = range[2], c2 = range[3];
	const auto r2y = matRgbToYCbCr(kb, kr, y1, y2, c1, c2);
	const auto y2r = matYCbCrToRgb(kb, kr, y1, y2, c1, c2);
	const auto shc = matSHC(saturation(), hue(), contrast());
	auto bvec = make3x1(qBound(-1.0, brightness(), 1.0), 0);

	mul = y2r*shc;
	if (colorspace == MP_CSP_RGB)
		mul = mul*r2y;
	else
		bvec -= shc*make3x1(y1, (c1 + c2)/2.0f);
	bvec = y2r*bvec;
	add = {bvec(0, 0), bvec(1, 0), bvec(2, 0)};

//	QMatrix3x3 mat = matYCbCrToRgb(kb, kr, y1, y2, c1, c2)*matSHC(saturation(), hue(), contrast());
//	if (colorspace == MP_CSP_RGB) {
//		mat = mat*matRgbToYCbCr(kb, kr, y1, y2, c1, c2);
//		mat(0, 3) = mat(1, 3) = mat(2, 3) = qBound(-1.0, brightness(), 1.0)/(y2 - y1);
//	}
//	if (colorspace != MP_CSP_RGB) {
//		mat(3, 0) = y1;	mat(3, 1) = mat(3, 2) = (c1 + c2)/2.0f;
//	}
}

