#ifndef OSDSTYLE_HPP
#define OSDSTYLE_HPP

#include <QtGui/QWidget>
#include <QtGui/QTextOption>
#include <QtCore/QPointF>
#include "enums.hpp"

class Record;

struct OsdStyle {
	typedef Enum::OsdScalePolicy Scale;
	void save(Record &r, const QString &group) const;
	void load(Record &r, const QString &group);

	// shapes
	QColor color{Qt::white}, outline_color{Qt::black}, shadow_color{Qt::black};
	bool has_shadow{false}, shadow_blur{true}, has_outline{true};
	double size{0.03}, outline_width{0.001};
	Scale scale{Scale::Width};
	QPointF shadow_offset{0.0, 0.0};

	// text
	QFont font;
	QTextOption::WrapMode wrap_mode{QTextOption::WrapAtWordBoundaryOrAnywhere};
	double line_spacing{0.0};
	double paragraph_spacing{0.0};
};

#endif // OSDSTYLE_HPP
