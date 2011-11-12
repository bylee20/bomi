#ifndef TAGITERATOR_HPP
#define TAGITERATOR_HPP

#include <QtCore/QVector>
#include <QtCore/QStringRef>
#include "richstring.hpp"

class TagIterator {
public:
	static bool opEq(const QStringRef &lhs, const char *rhs);

	struct Attr {
		Attr(): q('\0') {}
		void clear() {name.clear(); value.clear();}
		QStringRef name, value;
		QChar q;
	};

	TagIterator(const QStringRef &target, int pos = 0);
	TagIterator(const QString &target, int pos = 0);
	inline const QStringRef &element() const {return m_elem;}
	inline bool atEnd() const {return m_pos >= m_text.size();}
	inline QString toString() const {QString ret; return appendTo(ret);}
	inline bool elementIs(const char *elem) const {return opEq(m_elem, elem);}
	inline const Attr &attribute(int i) const {return m_attr[i];}
	QString &appendTo(QString &text) const;
	int valueIndex(const char *name) const;
	int valueToInt(const char *name) const;
	inline QStringRef value(const char *name) const {
		const int idx = valueIndex(name);
		return idx < 0 ? QStringRef() : m_attr[idx].value;
	}
	inline int attributeCount() const {return m_attr.size();}
	inline bool isOpen() const {return m_open;}
	inline bool isComment() const {return elementIs("!--");}
	inline int pos() const {return m_pos;}
	inline int begin() const {return m_begin;}
	int next();
	void setPos(int pos) {if (m_pos != pos) {m_pos = pos; m_begin = -1;}}
private:
	inline QStringRef midRef(int from, int count = -1) const {
		return RichString::midRef(m_text, from, count);
	}
	inline ushort ucs(int pos) const {return m_text.at(pos).unicode();}
	inline QChar qchar(int pos) const {return m_text.at(pos);}
	inline ushort ucs() const {return m_text.at(m_pos).unicode();}
	inline QChar qchar() const {return m_text.at(m_pos);}
	inline bool isRightBracket() const {return ucs() == '>';}
	inline bool isSeperator() const {return RichString::isSeperator(ucs());}
	inline bool isQuotationMark() const {const ushort c = ucs(); return c == '\'' || c == '"';}
	inline bool checkRightBracket() {return !(m_open = !isRightBracket());}
	inline bool skipSeperator() {return RichString::skipSeperator(m_pos, m_text);}

	QStringRef m_text;
	int m_pos, m_begin;
	QStringRef m_elem;
	QVector<Attr> m_attr;
	bool m_open;
};



#endif // TAGITERATOR_HPP
