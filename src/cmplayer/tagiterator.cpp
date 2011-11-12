#include "tagiterator.hpp"

TagIterator::TagIterator(const QStringRef &target, int pos)
: m_text(target), m_pos(pos), m_begin(-1), m_open(false) {}

TagIterator::TagIterator(const QString &target, int pos)
: m_text(target.midRef(0)), m_pos(pos), m_begin(-1), m_open(false) {}

bool TagIterator::opEq(const QStringRef &lhs, const char *rhs) {
	const int len = qstrlen(rhs);
	if (len != lhs.size())
		return false;
	for (int i=0; i<len; ++i) {
		if (lhs.at(i).toLower().unicode() != QChar::toLower((ushort)rhs[i]))
			return false;
	}
	return true;
}

int TagIterator::next() {
	m_begin = -1;
	m_attr.clear();
	m_elem.clear();
	for (; !atEnd() && (ucs() != '<'); ++m_pos) ;
	if (atEnd())
		return m_begin;
	Q_ASSERT(ucs(m_pos) == '<');
	m_open = true;
	m_begin = m_pos;
	++m_pos;
	if (skipSeperator())
		return m_begin;
	if (checkRightBracket()) {
		++m_pos;
		return m_begin;
	}

	int count = 0;
	int from = m_pos;
	for (; !atEnd() && !isSeperator() && !checkRightBracket(); ++m_pos, ++count) ;
	m_elem = midRef(from, count);

	if (!m_open) {
		++m_pos;
		return m_begin;
	} else if (skipSeperator())
		return m_begin;

	bool equal = false;
	int attrIdx = -1;
	while (!atEnd() && m_open) {
		if (skipSeperator() || checkRightBracket())
			break;
		from = m_pos;
		count = 0;
		if (!equal) {
			while (!atEnd()) {
				if (checkRightBracket() || isSeperator())
					break;
				equal = ucs() == '=';
				if (equal)
					break;
				++count;
				++m_pos;
			}
			Attr attr;
			attr.name = midRef(from, count);
			m_attr.push_back(attr);
			++attrIdx;
		} else {
			Attr &attr = m_attr[attrIdx];
			if (isQuotationMark()) {
				attr.q = qchar();
				from = ++m_pos;
				bool bs = false;
				while (!atEnd()) {
					if (bs)
						bs = false;
					else {
						if (qchar() == '\\')
							bs = true;
						else if (qchar() == attr.q)
							break;
					}
					++count;
					++m_pos;
				}
			} else {
				while (!atEnd()) {
					if (isSeperator() || checkRightBracket())
						break;
					++count;
					++m_pos;
				}
			}
			attr.value = midRef(from, count);
			equal = false;
		}
		if (!m_open)
			break;
		++m_pos;
	}
	if (!m_open)
		++m_pos;
	return m_begin;
}

QString &TagIterator::appendTo(QString &text) const {
	if (m_elem.isEmpty())
		return text;
	text += '<';
	text += m_elem;
	for (int i=0; i<m_attr.size(); ++i) {
		const Attr &attr = m_attr[i];
		text += ' ';
		text += attr.name;
		if (!attr.value.isEmpty()) {
			text += '=';
			if (attr.q != '\0') {
				text += attr.q;
				text += attr.value;
				text += attr.q;
			} else
				text += attr.value;
		}
	}
	if (!m_open)
	text += '>';
	return text;
}

int TagIterator::valueIndex(const char *name) const {
	for (int i=0; i<m_attr.size(); ++i) {
		if (opEq(m_attr[i].name, name))
			return i;
	}
	return -1;
}

int TagIterator::valueToInt(const char *name) const {
	const int idx = valueIndex(name);
	if (idx < 0)
		return 0;
	const QStringRef &value = m_attr[idx].value;
	int ret = 0;
	int p = 1;
	for (int ri = value.size()-1; ri >= 0; --ri) {
		const int n = value.at(ri).unicode() - 48;
		if (n < 0 || n > 9)
			return ret;
		ret += n*p;
		p *= 10;
	}
	return ret;
}
