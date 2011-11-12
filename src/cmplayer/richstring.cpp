#include "richstring.hpp"
#include "tagiterator.hpp"
#include <QtCore/QRegExp>
#include <QtCore/QDebug>
#include <QtCore/QMap>

class RichStringUtil {
public:
	RichStringUtil() {
		int from = 0;
		int count = 0;

#define INSERT_ENTITY(e, c) \
		m_fullEntity += (e);\
		count = qstrlen((e));\
		m_entityMap.insert(m_fullEntity.midRef(from, count), c);\
		from += count;
		INSERT_ENTITY("nbsp", ' ' );
		INSERT_ENTITY("amp", '&');
		INSERT_ENTITY("lt", '<');
		INSERT_ENTITY("gt", '>');
#undef INSERT_ENTITY
	}
	void appendEntityTo(QString &target, const QStringRef &entity) const {
		QMap<QStringRef, QChar>::const_iterator it = m_entityMap.find(entity);
		if (it == m_entityMap.end())
			target += entity;
		else
			target += *it;
	}
private:
	QString m_fullEntity;
	QMap<QStringRef, QChar> m_entityMap;
};

static const RichStringUtil util;

RichString::RichString() {}

RichString::RichString(const QString &rich, const QString &plain)
: m_rich(rich), m_plain(plain) {}


RichString::RichString(const QString &string) {
	cache(string.midRef(0));
}

RichString::RichString(const QStringRef &ref) {
	cache(ref);
}

bool RichString::skipSeperator(int &idx, const QStringRef &text) {
	for (; idx < text.size() && isSeperator(text.at(idx).unicode()); ++idx) ;
	return idx >= text.size();
}

QStringRef RichString::processEntity(int &idx, const QStringRef &ref) {
	Q_ASSERT(ref.at(idx) == '&');
	const int max = qMin(idx + 10, ref.size());
	for (int i=idx+1; i<max; ++i) {
		const ushort ucs = ref.at(i).unicode();
		if (isNumber(ucs) || isAlphabet(ucs) || ucs == '#')
			continue;
		if (ucs != ';')
			return QStringRef();
		const int from = idx + 1;
		idx = i + 1;
		return midRef(ref, from, i - from);
	}
	return QStringRef();
}

QStringRef RichString::trimmed(const QStringRef &ref) {
	int from = 0;
	TagIterator tag(ref);
	while (from < ref.size()) {
		const ushort ucs = ref.at(from).unicode();
		if (ucs == '<') {
			tag.setPos(from);
			if (tag.next() < 0 || !tag.elementIs("br"))
				break;
			from = tag.pos();
		} else if (ucs == '&') {
			int idx = from;
			const QStringRef entity = processEntity(idx, ref);
			if (entity.isEmpty()) {
				++from;
				continue;
			} else if (!TagIterator::opEq(entity, "nbsp"))
				break;
			from = idx;
		} else {
			if (!isSeperator(ucs))
				break;
			++from;
		}
	}
	int to = -1;
	int p = from;
	while (p < ref.size()) {
		const ushort ucs = ref.at(p).unicode();
		if (ucs == '<') {
			tag.setPos(p);
			if (tag.next() < 0) {
				to = -1;
				break;
			} else if (tag.elementIs("br")) {
				if (to < 0)
					to = p;
			} else
				to = -1;
			p = tag.pos();
		} else if (ucs == '&') {
			int idx = p;
			const QStringRef entity = processEntity(idx, ref);
			if (entity.isEmpty()) {
				to = -1;
				++p;
				continue;
			} else if (TagIterator::opEq(entity, "nbsp")) {
				if (to < 0)
					to = p;
			} else
				to = -1;
			p = idx;
		} else {
			if (isSeperator(ucs)) {
				if (to < 0)
					to = p;
			} else
				to = -1;
			++p;
		}
	}
	if (to < 0)
		return midRef(ref, from);
	return midRef(ref, from, to - from);
}

void RichString::process(const QStringRef &ref, QString &rich, QString &plain, bool hasTag) {
	int idx = 0;
	while (idx < ref.size()) {
		const QChar c = ref.at(idx);
		if (hasTag && c.unicode() == '<') {
			TagIterator tag(ref, idx);
			if (tag.next() < 0)
				break;
			idx = tag.pos();
			tag.appendTo(rich);
			if (tag.elementIs("br"))
				plain += '\n';
			continue;
		}
		if (isSeperator(c.unicode())) {
			plain += ' ';
			rich += ' ';
			if (skipSeperator(idx, ref))
				break;
			continue;
		}
		if (c.unicode() == '&') {
			const QStringRef entity = processEntity(idx, ref);
			if (!entity.isEmpty()) {
				rich += '&';
				rich += entity;
				rich += ';';
				util.appendEntityTo(plain, entity);
				continue;
			}
		}
		rich += c;
		plain += c;
		++idx;
	}
}

void RichString::cache(const QStringRef &ref) {
	m_rich.clear();
	m_plain.clear();
	m_rich.reserve(ref.size());
	m_plain.reserve(ref.size());
	if (!ref.isEmpty())
		process(ref, m_rich, m_plain, true);
}

RichString::RichString(const RichString &other)
: m_rich(other.m_rich), m_plain(other.m_plain) {}

RichString::~RichString() {

}

RichString &RichString::operator = (const RichString &rhs) {
	if (this != &rhs) {
		m_rich = rhs.m_rich;
		m_plain = rhs.m_plain;
	}
	return *this;
}

bool RichString::hasWords() const {
	for (int i=0; i<m_plain.size(); ++i) {
		if (!isSeperator(m_plain[i].unicode()))
			return true;
	}
	return false;
}

bool RichString::isEmpty() const {
	return m_plain.isEmpty();
}

int RichString::size() const {
	return m_plain.size();
}

RichString &RichString::merge(const RichString &other) {
	if (!other.hasWords())
		return *this;
	if (!hasWords())
		return (*this = other);
	m_rich += "<br>";
	m_rich += other.m_rich;
	m_plain += '\n';
	m_plain += other.m_plain;
	return *this;
}

RichString RichString::merged(const RichString &other) const {
	return RichString(*this).merge(other);
}

void RichString::clear() {
	m_rich.clear();
	m_plain.clear();
}

