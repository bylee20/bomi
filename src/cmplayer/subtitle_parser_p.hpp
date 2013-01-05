#ifndef SUBTITLE_PARSER_P_HPP
#define SUBTITLE_PARSER_P_HPP

#include "stdafx.hpp"
#include "subtitle_parser.hpp"
#include "richtexthelper.hpp"

class TagFormat;
class RichTextBlock;

class SamiParser : public SubtitleParser {
public:
	void _parse(Subtitle &sub);
	bool isParsable() const;
};

class SubRipParser : public SubtitleParser {
public:
	void _parse(Subtitle &sub);
	bool isParsable() const;
};

class LineParser : public SubtitleParser {
public:
	LineParser(const QString expr): rxLine(expr) {}
	bool isParsable() const {
		if (skipSeparators())
			return false;
		const int pos = this->pos();
		if (rxLine.indexIn(trim(getLine()).toString()) == -1)
			return false;
		seekTo(pos);
		return true;
	}
protected:
	QRegExp rxLine;
};

class TMPlayerParser : public LineParser {
public:
	TMPlayerParser(): LineParser(_L("^\\s*(\\d?\\d)\\s*:\\s*(\\d\\d)\\s*:\\s*(\\d\\d)\\s*:\\s*(.*)$")) {}
	void _parse(Subtitle &sub);
};

class MicroDVDParser : public LineParser {
public:
	MicroDVDParser(): LineParser(_L("^\\{(\\d+)\\}\\{(\\d+)\\}(.*)$")) {}
	void _parse(Subtitle &sub);
};

#endif // SUBTITLE_PARSER_P_HPP
