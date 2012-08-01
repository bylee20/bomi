#include "osdstyle.hpp"
#include <QtGui/QColorDialog>
#include <QtGui/QFontDialog>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include "record.hpp"

OsdStyle::OsdStyle()
	: color_bg(Qt::black), color_fg(Qt::white), shadow_color(Qt::black) {
	border_width = 0.05;
	text_scale = 0.03;
	auto_size = AutoSize::Width;
	has_shadow = false;
	shadow_offset = QPointF(0, 0);
	wrap_mode = QTextOption::WrapAtWordBoundaryOrAnywhere;
	shadow_blur = 3;
}

void OsdStyle::save(Record &r, const QString &group) const {
	r.beginGroup(group);
	r.write("font", font);
	r.write("color_bg", color_bg);
	r.write("color_fg", color_fg);
	r.write("border_width", border_width);
	r.write("text_scale", text_scale);
	r.write("auto_size", auto_size.name());
	r.write("has_shadow", has_shadow);
	r.write("shadow_color", shadow_color);
	r.write("shadow_offset", shadow_offset);
	r.write("shadow_blur", shadow_blur);
	r.endGroup();
}

void OsdStyle::load(Record &r, const QString &group) {
	r.beginGroup(group);
	font = r.read("font", font);
	color_bg = r.read("color_bg", color_bg);
	color_fg = r.read("color_fg", color_fg);
	border_width = r.read("border_width", border_width);
	text_scale = r.read("text_scale", text_scale);
	auto_size = r.readEnum("auto_size", AutoSize::Width);
	has_shadow = r.read("has_shadow", has_shadow);
	shadow_color = r.read("shadow_color", shadow_color);
	shadow_offset = r.read("shadow_offset", shadow_offset);
	shadow_blur = r.read("shadow_blur", shadow_blur);
	r.endGroup();
}
