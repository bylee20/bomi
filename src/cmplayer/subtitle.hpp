#ifndef SUBTITLE_HPP
#define SUBTITLE_HPP

#include <QtCore/QMap>
#include <QtCore/QList>
#include <QtGui/QTextFormat>
#include <QtGui/QTextLayout>
#include "richtextdocument.hpp"

struct SubtitleCaption : public RichTextDocument {
	SubtitleCaption() {index = -1;}
	RichTextDocument &doc() {return *this;}
	const RichTextDocument &doc() const {return *this;}
	inline SubtitleCaption &operator += (const SubtitleCaption &rhs) {RichTextDocument::operator += (rhs); return *this;}
	inline SubtitleCaption &operator += (const RichTextDocument &rhs) {RichTextDocument::operator += (rhs); return *this;}
	inline SubtitleCaption &operator += (const QList<RichTextBlock> &rhs) {RichTextDocument::operator += (rhs); return *this;}
	mutable int index;
};

class SubtitleComponent : public QMap<int, SubtitleCaption> {
	typedef QMap<int, SubtitleCaption> Super;
public:
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
	SubtitleComponent(const QString &file = QString(), SyncType base = Time);
	SubtitleComponent &unite(const SubtitleComponent &other, double frameRate);
	SubtitleComponent united(const SubtitleComponent &other, double frameRate) const;
	bool operator == (const SubtitleComponent &rhs) const {return name() == rhs.name();}
	bool operator != (const SubtitleComponent &rhs) const {return !operator==(rhs);}
	QString name() const;
	const QString &fileName() const {return m_file;}
	SyncType base() const {return m_base;}
	bool isBasedOnFrame() const {return m_base == Frame;}
//	const Language &language() const {return m_lang;}
	QString language() const {return m_klass;}
	const_iterator start(int time, double frameRate) const;
	const_iterator finish(int time, double frameRate) const;
	static int convertKeyBase(int key, SyncType from, SyncType to, double frameRate) {
		return  (from == to) ? key : ((to == Time) ? msec(key, frameRate) : frame(key, frameRate));
	}
	bool flag() const;
	void setFlag(bool flag) {m_flag = flag;}
	static int msec(int frame, double frameRate) {return qRound(frame/frameRate*1000.0);}
	static int frame(int msec, double frameRate) {return qRound(msec*0.001*frameRate);}

	QString m_klass;
private:
	friend class Parser;
	QString m_file;
	SyncType m_base;
//	Language m_lang;

	mutable bool m_flag;

};

typedef QMapIterator<int, SubtitleCaption> SubtitleComponentIterator;

class Subtitle {
public:
	const SubtitleComponent &operator[] (int rhs) const {return m_comp[rhs];}
	Subtitle &operator += (const Subtitle &rhs) {m_comp += rhs.m_comp; return *this;}
	int count() const {return m_comp.size();}
	int size() const {return m_comp.size();}
	bool isEmpty() const;
	SubtitleComponent component(double frameRate) const;
//	int start(int time, double frameRate) const;
//	int end(int time, double frameRate) const;
	RichTextDocument caption(int time, double frameRate) const;
	bool load(const QString &file, const QString &enc);
	void clear() {m_comp.clear();}
	void append(const SubtitleComponent &comp) {m_comp.append(comp);}
	static Subtitle parse(const QString &fileName, const QString &enc);
private:
	friend class SubtitleParser;
	QList<SubtitleComponent> m_comp;
};

#endif // SUBTITLE_HPP


