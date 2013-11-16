#ifndef SUBTITLESTYLE_H
#define SUBTITLESTYLE_H

#include "stdafx.hpp"
#include "enums.hpp"

class Record;

struct SubtitleStyle {
	struct Font {
		typedef OsdScalePolicy Scale;
		Font() { qfont.setPixelSize(height()); }
		QString family() const {return qfont.family();}
		bool bold() const {return qfont.bold();}
		bool italic() const {return qfont.italic();}
		bool underline() const {return qfont.underline();}
		bool strikeOut() const {return qfont.strikeOut();}
		void setFamily(const QString &family) {qfont.setFamily(family);}
		void setBold(bool bold) {qfont.setBold(bold);}
		void setItalic(bool italic) {qfont.setItalic(italic);}
		void setUnderline(bool underline) {qfont.setUnderline(underline);}
		void setStrikeOut(bool strikeOut) {qfont.setStrikeOut(strikeOut);}
		const QFont &font() const {return qfont;}
		static constexpr int height() {return 20;}
		int weight() const {return qfont.weight();}
		QColor color = {Qt::white};
		double size = 0.03;
		Scale scale = Scale::Width;
		QFont qfont;
	};
	struct BoundingBox { bool enabled = false; QColor color = {0, 0, 0, 127}; QPointF padding = {0.3, 0.1}; };
	struct Shadow { bool enabled = true, blur = false; QColor color = {0, 0, 0, 127}; QPointF offset = {0.1, 0.1}; };
	struct Outline { QColor color = {Qt::black}; double width = 0.05; bool enabled = true; };
	struct Spacing { double line = 0, paragraph = 0; };
	QTextOption::WrapMode wrapMode = QTextOption::WrapAtWordBoundaryOrAnywhere;
	Shadow shadow; Outline outline; Font font; Spacing spacing; BoundingBox bbox;
	void save(Record &r, const QString &group) const;
	void load(Record &r, const QString &group);
};

#endif // SUBTITLESTYLE_H
