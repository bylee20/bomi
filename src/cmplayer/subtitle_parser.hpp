#ifndef SUBTITLE_PARSER_HPP
#define SUBTITLE_PARSER_HPP

#include "stdafx.hpp"
#include "subtitle.hpp"

class SubtitleParser : public RichTextHelper {
public:
	virtual ~SubtitleParser() {}
	static Subtitle parse(const QString &file, const QString &enc);
//	void setEncoding(const QString &enc) {m_enc = enc;}
	static void setMsPerCharactor(int msPerChar) {SubtitleParser::msPerChar = msPerChar;}
	bool save(const Subtitle &sub, const QString &fileName, const QString &enc);
protected:
	virtual bool isParsable() const = 0;
	virtual bool _save(QString &/*save*/, const Subtitle &/*sub*/) {return false;}
	virtual void _parse(Subtitle &sub) = 0;
	const QString &all() const {return m_all;}
	QStringRef getLine() const;
	int pos() const {return m_pos;}
	bool atEnd() const {return m_pos >= m_all.size();}
	ushort at(int i) const {return m_all.at(i).unicode();}
	void seekTo(int pos) const {m_pos = pos;}
	static int predictEndTime(const SubComp::const_iterator &it);
	static int predictEndTime(int start, const QString &text);
	static QString encodeEntity(const QStringRef &str);
	static QStringRef processLine(int &idx, const QString &contents);
	static QList<SubComp> &components(Subtitle &sub) {return sub.m_comp;}
	static const QList<SubComp> &components(const Subtitle &sub) {return sub.m_comp;}
	static void append(SubComp &c, const QString &text, int start) {c[start] += RichTextDocument(text);}
	static void append(SubComp &c, const QString &text, int start, int end) {append(c, text, start); c[end];}
	const QFileInfo &file() const {return m_file;}
	bool skipSeparators() const {return RichTextHelper::skipSeparator(m_pos, m_all);}
	SubComp &append(Subtitle &sub, SubComp::SyncType base = SubComp::Time) {
		sub.m_comp.append(SubComp(m_file.fileName(), base));
		return sub.m_comp.last();
	}
private:
	static int msPerChar;
	QString m_all;
	QFileInfo m_file;
	mutable int m_pos = 0;
};


#endif // SUBTITLE_PARSER_HPP
