#ifndef SUBTITLE_PARSER_HPP
#define SUBTITLE_PARSER_HPP

#include "subtitle.hpp"

class Subtitle::Parser {
public:
	virtual ~Parser() {}
	Subtitle parse(const QString &file);
	void setEncoding(const QString &enc) {m_enc = enc;}
	static Parser *create(const QString &ext);
	static void setMsPerCharactor(int msPerChar) {Parser::msPerChar = msPerChar;}
	bool save(const Subtitle &sub, const QString &fileName);
protected:
	virtual bool _save(QString &/*save*/, const Subtitle &/*sub*/) {return false;}
	virtual void _parse(Subtitle &sub) = 0;
	QStringRef getLineRef() const {return trimmed(processLine(m_pos, m_all));}
	QString getLine() const {return getLineRef().toString();}
	const QString &all() const {return m_all;}
	const QString &name() const {return m_name;}
	int pos() const {return m_pos;}
	bool atEnd() const {return m_pos >= m_all.size();}

	static int predictEndTime(const Component::const_iterator &it);
	static QString &replaceEntity(QString &str);
	static QStringRef processLine(int &idx, const QString &contents);
	static QStringRef trimmed(const QStringRef &ref);
private:
	class Sami;
	class SubRip;
	class SubStationAlpha;
	class TMPlayer;
	class MicroDVD;
	static int msPerChar;
	QString m_enc;
	QString m_all;
	QString m_name;
	mutable int m_pos;
};

#endif // SUBTITLE_PARSER_HPP
