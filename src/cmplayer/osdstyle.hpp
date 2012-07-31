#ifndef OSDSTYLE_HPP
#define OSDSTYLE_HPP

#include <QtGui/QWidget>
#include <QtGui/QTextOption>
#include <QtCore/QPointF>
#include "enums.hpp"

class Record;

class OsdStyle {
public:
	typedef Enum::OsdAutoSize AutoSize;
	OsdStyle();
	void save(Record &r, const QString &group) const;
	void load(Record &r, const QString &group);
	QFont font;
	QColor color_bg, color_fg, shadow_color;
	double border_width, text_scale;
	QTextOption::WrapMode wrap_mode;
	AutoSize auto_size;
	bool has_shadow;
	QPointF shadow_offset;
	bool shadow_blur;
};

#endif // OSDSTYLE_HPP
