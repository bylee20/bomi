#ifndef SUBTITLE_HPP
#define SUBTITLE_HPP

#include "stdafx.hpp"
#include "richtextdocument.hpp"

struct SubCapt : public RichTextDocument {
	SubCapt() {index = -1;}
	RichTextDocument &doc() {return *this;}
	const RichTextDocument &doc() const {return *this;}
	inline SubCapt &operator += (const SubCapt &rhs) {RichTextDocument::operator += (rhs); return *this;}
	inline SubCapt &operator += (const RichTextDocument &rhs) {RichTextDocument::operator += (rhs); return *this;}
	inline SubCapt &operator += (const QList<RichTextBlock> &rhs) {RichTextDocument::operator += (rhs); return *this;}
	mutable int index;
};

class SubComp {
public:
	using Map = QMap<int, SubCapt>;
	using It = Map::iterator;
	using ConstIt = Map::const_iterator;
	using iterator = Map::iterator;
	using const_iterator = Map::const_iterator;
//	struct Lang {
//		QString id() const {
//			if (!name.isEmpty())
//				return name;
//			if (!locale.isEmpty())
//				return locale;
//			return klass;
//		}
//		QString name, locale, klass;
//	};
	enum SyncType {Time, Frame};
	SubComp(const QString &file = QString(), SyncType base = Time);
	bool operator == (const SubComp &rhs) const {return name() == rhs.name();}
	bool operator != (const SubComp &rhs) const {return !operator==(rhs);}
	SubCapt &operator[] (int key) { return m_capts[key]; }
	SubCapt operator[] (int key) const { return m_capts[key]; }
	SubComp &unite(const SubComp &other, double frameRate);
	SubComp united(const SubComp &other, double frameRate) const;

	bool hasWords() const { for (auto &capt : m_capts) if (capt.hasWords()) return true; return false; }
	bool isEmpty() const { return m_capts.isEmpty(); }
	It begin() { return m_capts.begin(); }
	It end() { return m_capts.end(); }
	ConstIt begin() const { return m_capts.begin(); }
	ConstIt end() const { return m_capts.end(); }
	ConstIt cbegin() const { return m_capts.cbegin(); }
	ConstIt cend() const { return m_capts.cend(); }
	It upperBound(int key) { return m_capts.upperBound(key); }
	It lowerBound(int key) { return m_capts.lowerBound(key); }
	ConstIt upperBound(int key) const { return m_capts.upperBound(key); }
	ConstIt lowerBound(int key) const { return m_capts.lowerBound(key); }
	It insert(int key, const SubCapt &capt) { return m_capts.insert(key, capt); }

	QString name() const;
	const QString &fileName() const {return m_file;}
	SyncType base() const {return m_base;}
	bool isBasedOnFrame() const {return m_base == Frame;}
//	const Language &language() const {return m_lang;}
	QString language() const {return m_klass;}
	const_iterator start(int time, double frameRate) const;
	const_iterator finish(int time, double frameRate) const;
	static int msec(int frame, double frameRate) {return qRound(frame/frameRate*1000.0);}
	static int frame(int msec, double frameRate) {return qRound(msec*0.001*frameRate);}
	int toTime(int key, double fps) const { return m_base == Time ? key : msec(key, fps); }
	const Map &map() const { return m_capts; }
	void setLanguage(const QString &lang) { m_klass = lang; }
	bool selection() const { return m_selection; }
	bool &selection() { return m_selection; }
private:
	friend class Parser;
	QString m_file;
	QString m_klass;
	SyncType m_base;
//	Language m_lang;
	Map m_capts;
	mutable bool m_selection = false;
};

typedef QMapIterator<int, SubCapt> SubtitleComponentIterator;

class Subtitle {
public:
	const SubComp &operator[] (int rhs) const {return m_comp[rhs];}
	Subtitle &operator += (const Subtitle &rhs) {m_comp += rhs.m_comp; return *this;}
	int count() const {return m_comp.size();}
	int size() const {return m_comp.size();}
	bool isEmpty() const;
	SubComp component(double frameRate) const;
//	int start(int time, double frameRate) const;
//	int end(int time, double frameRate) const;
	RichTextDocument caption(int time, double frameRate) const;
	bool load(const QString &file, const QString &enc, double accuracy);
	void clear() {m_comp.clear();}
	void append(const SubComp &comp) {m_comp.append(comp);}
	static Subtitle parse(const QString &fileName, const QString &enc);
private:
	friend class SubtitleParser;
	QList<SubComp> m_comp;
};

#endif // SUBTITLE_HPP


