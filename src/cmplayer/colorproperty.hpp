#ifndef COLORPROPERTY_HPP
#define COLORPROPERTY_HPP

#include <QtGlobal>

class ColorProperty {
public:
	enum Value {Brightness = 0, Saturation, Contrast, Hue, PropMax};
	ColorProperty(double b, double s, double c, double h) {
		m_value[Brightness] = b;
		m_value[Saturation] = s;
		m_value[Contrast] = c;
		m_value[Hue] = h;
	}
	ColorProperty() {
		m_value[Brightness] = m_value[Saturation]
			= m_value[Contrast] = m_value[Hue] = 0.0;
	}
	ColorProperty(const ColorProperty &other) {
		m_value[Brightness] = other.m_value[Brightness];
		m_value[Saturation] = other.m_value[Saturation];
		m_value[Contrast] = other.m_value[Contrast];
		m_value[Hue] = other.m_value[Hue];
	}
	~ColorProperty() {}
	ColorProperty &operator = (const ColorProperty &rhs) {
		if (this != &rhs) {
			m_value[Brightness] = rhs.m_value[Brightness];
			m_value[Saturation] = rhs.m_value[Saturation];
			m_value[Contrast] = rhs.m_value[Contrast];
			m_value[Hue] = rhs.m_value[Hue];
		}
		return *this;
	}
	bool operator == (const ColorProperty &rhs) const {
		return qFuzzyCompare(m_value[Brightness], rhs.m_value[Brightness])
			&& qFuzzyCompare(m_value[Saturation], rhs.m_value[Saturation])
			&& qFuzzyCompare(m_value[Contrast], rhs.m_value[Contrast])
			&& qFuzzyCompare(m_value[Hue], rhs.m_value[Hue]);
	}
	bool operator != (const ColorProperty &rhs) const {
		return !operator==(rhs);
	}
	double &operator [] (Value p) {return m_value[p];}
	const double &operator [] (Value p) const {return m_value[p];}
	double value(Value v) const {return m_value[v];}
	double brightness() const {return m_value[Brightness];}
	double saturation() const {return m_value[Saturation];}
	double contrast() const {return m_value[Contrast];}
	double hue() const {return m_value[Hue];}
	void setValue(Value p, double val) {m_value[p] = qFuzzyCompare(val, 0.0) ? 0.0 : val;}
	void setBrightness(double v) {m_value[Brightness] = v;}
	void setSaturation(double v) {m_value[Saturation] = v;}
	void setContrast(double v) {m_value[Contrast] = v;}
	void setHue(double v) {m_value[Hue] = v;}
	void clamp() {
		m_value[Brightness] = qBound(-1.0, m_value[Brightness], 1.0);
		m_value[Contrast] = qBound(-1.0, m_value[Contrast], 1.0);
		m_value[Saturation] = qBound(-1.0, m_value[Saturation], 1.0);
		m_value[Hue] = qBound(-1.0, m_value[Hue], 1.0);
	}
	bool isZero() const {
		return qFuzzyCompare(m_value[Brightness], 0.0)
			&& qFuzzyCompare(m_value[Saturation], 0.0)
			&& qFuzzyCompare(m_value[Contrast], 0.0)
			&& qFuzzyCompare(m_value[Hue], 0.0);
	}
private:
	double m_value[4];
};

#endif // COLORPROPERTY_HPP
