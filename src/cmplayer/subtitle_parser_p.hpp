#ifndef SUBTITLE_PARSER_P_HPP
#define SUBTITLE_PARSER_P_HPP

#include "subtitle_parser.hpp"
#include <QtCore/QRegExp>

class TagIterator;

class Subtitle::Parser::Sami : public Subtitle::Parser {
public:
	void _parse(Subtitle &sub);
private:
	static QString &appendTo(QString &rich, const TagIterator &tag);
	static bool needToAddSharp(const QStringRef &name, const QStringRef &value);
};

class Subtitle::Parser::TMPlayer : public Subtitle::Parser {
public:
	void  _parse(Subtitle &sub);
private:
	static QRegExp rxLine;
	friend class Parser;
};

class Subtitle::Parser::MicroDVD : public Subtitle::Parser {
public:
	void _parse(Subtitle &sub);
private:
	static QRegExp rxLine;
	friend class Parser;
};

class Subtitle::Parser::SubRip : public Subtitle::Parser {
	void _parse(Subtitle &sub);
};

#endif // SUBTITLE_PARSER_P_HPP
