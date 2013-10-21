#ifndef COLORPROPERTY_HPP
#define COLORPROPERTY_HPP

#include <QObject>
#include <QGenericMatrix>
#include <QMatrix4x4>
extern "C" {
#include <video/csputils.h>
}

struct Kernel3x3 {
	Kernel3x3() { mat(0, 0) = mat(2, 2) = 0.f; }
	Kernel3x3(double center, double neighbor, double diagonal) {
		set(center, neighbor, diagonal);
	}
	Kernel3x3 &operator = (const Kernel3x3 &rhs) { mat = rhs.mat; return *this; }
	float &operator () (int i, int j) { return at(i, j); }
	float  operator () (int i, int j) const {return at(i, j); }
	Kernel3x3 &operator += (const Kernel3x3 &rhs) { mat += rhs.mat; return *this; }
	Kernel3x3 operator + (const Kernel3x3 &rhs) { Kernel3x3 lhs = *this; return (lhs += rhs); }
	Kernel3x3 merged(const Kernel3x3 &other, bool normalize = true) {
		Kernel3x3 ret = *this + other;
		if (normalize)
			ret.normalize();
		return ret;
	}
	void normalize() {
		double den = 0.0;
		for (int i=0; i<9; ++i)
			den += mat.data()[i];
		mat /= den;
	}
	Kernel3x3 normalized() const {
		Kernel3x3 kernel = *this;
		kernel.normalize();
		return kernel;
	}
	float &at(int i, int j) { return mat(i, j); }
	float at(int i, int j) const {return mat(i, j); }
	void set(double center, double neighbor, double diagonal) {
		mat(1, 1) = center;
		mat(0, 1) = mat(1, 0) = mat(1, 2) = mat(2, 1) = neighbor;
		mat(0, 0) = mat(0, 2) = mat(2, 0) = mat(2, 2) = diagonal;
	}
	float center() const {return mat(1, 1);}
	float neighbor() const {return mat(0, 1);}
	float diagonal() const {return mat(0, 0);}
	QMatrix3x3 matrix() const {return mat;}
private:
	QMatrix3x3 mat;
};

class VideoColor {
	Q_DECLARE_TR_FUNCTIONS(VideoColor)
public:
	enum Type {Brightness = 0, Contrast, Saturation, Hue, TypeMax};
	VideoColor(int b, int c, int s, int h): m{{b, c, s, h}} {}
	VideoColor() = default;
	bool operator == (const VideoColor &rhs) const { return m == rhs.m; }
	bool operator != (const VideoColor &rhs) const { return m != rhs.m; }
	Type operator & (const VideoColor &rhs) const {
		int count = 0;
		Type type = TypeMax;
		for (uint i=0; i<m.size(); ++i) {
			if (m[i] != rhs.m[i]) {
				++count;
				type = (Type)i;
			}
		}
		return count == 1 ? type : TypeMax;
	}
	VideoColor &operator += (const VideoColor &rhs) {
		m[0] += rhs.m[0]; m[1] += rhs.m[1]; m[2] += rhs.m[2]; m[3] += rhs.m[3]; return *this;
	}
	VideoColor operator + (const VideoColor &rhs) const { return VideoColor(*this) += rhs; }
	int &operator [] (Type p) {return m[p];}
	const int &operator [] (Type p) const {return m[p];}
	int value(Type v) const {return m[v];}
	int brightness() const {return m[Brightness];}
	int saturation() const {return m[Saturation];}
	int contrast() const {return m[Contrast];}
	int hue() const {return m[Hue];}

	int &value(Type v) {return m[v];}
	int &brightness() {return m[Brightness];}
	int &saturation() {return m[Saturation];}
	int &contrast() {return m[Contrast];}
	int &hue() {return m[Hue];}

	void setValue(Type p, int val) {m[p] = qFuzzyCompare(val, 0.0) ? 0.0 : val;}
	void setBrightness(int v) {m[Brightness] = v;}
	void setSaturation(int v) {m[Saturation] = v;}
	void setContrast(int v) {m[Contrast] = v;}
	void setHue(int v) {m[Hue] = v;}
#undef clamp
	void clamp() {
		m[Brightness] = qBound(-100, m[Brightness], 100);
		m[Contrast] = qBound(-100, m[Contrast], 100);
		m[Saturation] = qBound(-100, m[Saturation], 100);
		m[Hue] = qBound(-100, m[Hue], 100);
	}
	bool isZero() const { return !m[Brightness] && !m[Saturation] && !m[Contrast] && !m[Hue]; }
	void matrix(QMatrix3x3 &mul, QVector3D &add, mp_csp colorspace, mp_csp_levels range) const;
	QString getText(Type type) const {
		const QString value = 0 <= type && type < TypeMax ? _NS(m[type]) : QString();
		switch (type) {
		case Brightness:
			return tr("Brightness %1%").arg(value);
		case Saturation:
			return tr("Saturation %1%").arg(value);
		case Contrast:
			return tr("Contrast %1%").arg(value);
		case Hue:
			return tr("Hue %1%").arg(value);
		default:
			return tr("Reset");
		}
	}
private:
	std::array<int, TypeMax> m{{0, 0, 0, 0}};
};

Q_DECLARE_METATYPE(VideoColor)

#endif // COLORPROPERTY_HPP
