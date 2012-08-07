#include "osdstyle.hpp"
#include <QtGui/QColorDialog>
#include <QtGui/QFontDialog>
#include <QtCore/QSettings>
#include <QtCore/QDebug>
#include "record.hpp"

void OsdStyle::save(Record &r, const QString &group) const {
	r.beginGroup(group);
#define WRITE(a) r.write(#a, a)
	WRITE(color);
	WRITE(outline_color);
	WRITE(shadow_color);
	WRITE(has_shadow);
	WRITE(shadow_blur);
	WRITE(has_outline);
	r.write("scale", scale.name());
	WRITE(shadow_offset);
	WRITE(font);
	WRITE(line_spacing);
	WRITE(paragraph_spacing);
#undef WRITE
	r.endGroup();
}

void OsdStyle::load(Record &r, const QString &group) {
	r.beginGroup(group);
#define READ(a) a = r.read(#a, a)
	READ(color);
	READ(outline_color);
	READ(shadow_color);
	READ(has_shadow);
	READ(shadow_blur);
	READ(has_outline);
	r.readEnum(scale, "scale");
	READ(shadow_offset);
	READ(font);
	READ(line_spacing);
	READ(paragraph_spacing);
#undef READ
	r.endGroup();

	// for < 0.6.0 compatibility
	shadow_offset.rx() = qBound(-0.01, shadow_offset.x(), 0.01);
	shadow_offset.ry() = qBound(-0.01, shadow_offset.y(), 0.01);
}
