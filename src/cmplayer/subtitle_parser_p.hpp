#ifndef SUBTITLE_PARSER_P_HPP
#define SUBTITLE_PARSER_P_HPP

#include "subtitle_parser.hpp"
#include <QtCore/QRegExp>
#include <QtGui/QTextCharFormat>
#include "richtexthelper.hpp"

class Tag;
class TagFormat;
class RichTextBlock;

class SubtitleParser::Sami : public SubtitleParser, public RichTextHelper {
public:
	void _parse(Subtitle &sub);
};


//class Subtitle::Parser::TMPlayer : public Subtitle::Parser {
//public:
//	void  _parse(Subtitle &sub);
//private:
//	static QRegExp rxLine;
//	friend class Parser;
//};

//class Subtitle::Parser::MicroDVD : public Subtitle::Parser {
//public:
//	void _parse(Subtitle &sub);
//private:
//	static QRegExp rxLine;
//	friend class Parser;
//};

//class Subtitle::Parser::SubRip : public Subtitle::Parser {
//	void _parse(Subtitle &sub);
//};

#endif // SUBTITLE_PARSER_P_HPP
