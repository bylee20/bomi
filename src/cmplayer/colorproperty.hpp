#ifndef COLORPROPERTY_HPP
#define COLORPROPERTY_HPP

#include <QGenericMatrix>
extern "C" {
#include <video/csputils.h>
}

class ColorProperty {
public:
	enum Value {Brightness = 0, Saturation, Contrast, Hue, PropMax};
	ColorProperty(double b, double s, double c, double h): m{{b, s, c, h}} {}
	ColorProperty() = default;
	bool operator == (const ColorProperty &rhs) const { return m == rhs.m; }
	bool operator != (const ColorProperty &rhs) const { return m != rhs.m; }
	double &operator [] (Value p) {return m[p];}
	const double &operator [] (Value p) const {return m[p];}
	double value(Value v) const {return m[v];}
	double brightness() const {return m[Brightness];}
	double saturation() const {return m[Saturation];}
	double contrast() const {return m[Contrast];}
	double hue() const {return m[Hue];}

	double &value(Value v) {return m[v];}
	double &brightness() {return m[Brightness];}
	double &saturation() {return m[Saturation];}
	double &contrast() {return m[Contrast];}
	double &hue() {return m[Hue];}

	void setValue(Value p, double val) {m[p] = qFuzzyCompare(val, 0.0) ? 0.0 : val;}
	void setBrightness(double v) {m[Brightness] = v;}
	void setSaturation(double v) {m[Saturation] = v;}
	void setContrast(double v) {m[Contrast] = v;}
	void setHue(double v) {m[Hue] = v;}
#undef clamp
	void clamp() {
		m[Brightness] = qBound(-1.0, m[Brightness], 1.0);
		m[Contrast] = qBound(-1.0, m[Contrast], 1.0);
		m[Saturation] = qBound(-1.0, m[Saturation], 1.0);
		m[Hue] = qBound(-1.0, m[Hue], 1.0);
	}
	bool isZero() const {
		return qFuzzyCompare(m[Brightness], 0.0)
			&& qFuzzyCompare(m[Saturation], 0.0)
			&& qFuzzyCompare(m[Contrast], 0.0)
			&& qFuzzyCompare(m[Hue], 0.0);
	}

/*	m00 m01 m02  v0
 *  m10 m11 m12  v1
 *  m20 m21 m22  v2
 *   o0  o1  o2   x
 */
	QMatrix4x4 matrix(mp_csp colorspace, mp_csp_levels range) const;
private:
	std::array<double, 4> m{{0.0, 0.0, 0.0, 0.0}};
};

#endif // COLORPROPERTY_HPP
