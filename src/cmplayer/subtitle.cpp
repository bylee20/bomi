#include "subtitle.hpp"
#include "subtitle_parser.hpp"
#include "global.hpp"
#include "pref.hpp"
#include "charsetdetector.hpp"
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

QString Subtitle::Component::name() const {
	if (m_lang.id().isEmpty())
		return QFileInfo(m_file).fileName();
	else
		return QFileInfo(m_file).fileName() + " (" + m_lang.id() + ')';
}

Subtitle::Component Subtitle::component(double frameRate) const {
	if (m_comp.isEmpty())
		return Component();
	Component comp;
	for (int i=0; i<m_comp.size(); ++i)
		comp.unite(m_comp[i], frameRate);
	return comp;
}

Subtitle::Component::const_iterator Subtitle::Component::start(int time, double frameRate) const {
	if (isEmpty() || time < 0)
		return end();
	return --end(time, frameRate);
}

Subtitle::Component::const_iterator Subtitle::Component::end(int time, double frameRate) const {
	if (isEmpty() || time < 0)
		return end();
	int key = time;
	if (m_base == Frame) {
		if (frameRate < 0.0)
			return end();
		key = qRound(time*0.001*frameRate);
	}
	return upperBound(key);
}

Subtitle::Component &Subtitle::Component::unite(const Component &other, double frameRate) {
	if (this == &other || other.isEmpty())
		return *this;
	else if (isEmpty()) {
		*this = other;
		return *this;
	}
	ComponentIterator it1(*this);
	ComponentIterator it2(other);
	int k3 = -1;
	while(it1.hasNext()) {
		int k1 = it1.next().key();
		int k2 = it1.hasNext() ? it1.peekNext().key() : -1;
		if (k3 != -1 && it2.hasPrevious())
			(*this)[k1].text = it1.value().text.merged(it2.peekPrevious().value().text);
		while(it2.hasNext()) {
			k3 = convertKeyBase(it2.next().key(), other.base(), m_base, frameRate);
			if (k2 == -1)
				(*this)[k3].text = it1.value().text.merged(it2.value().text);
			else if (k3 >= k2) {
				it2.previous();
				break;
			} else if (k3 == k1)
				(*this)[k1].text = it1.value().text.merged(it2.value().text);
			else if (k3 > k1)
				(*this)[k3].text = it1.value().text.merged(it2.value().text);
			else if (k3 < k1)
				(*this)[k3].text = it2.value().text;
		}
	}
	Component::iterator it = begin();
	for (int idx = 0; it != end(); ++idx, ++it)
		it->index = idx;
	return *this;
}

//int Subtitle::start(int time, double frameRate) const {
//	int s = -1;
//	for (int i=0; i<m_comp.size(); ++i) {
//		const Component::const_iterator it = m_comp[i].start(time, frameRate);
//		if (it != m_comp[i].end())
//			s = qMax(s, m_comp[i].isBasedOnFrame() ? msec(it.key(), frameRate) : it.key());
//	}
//	return s;
//}

//int Subtitle::end(int time, double frameRate) const {
//	int e = -1;
//	for (int i=0; i<m_comp.size(); ++i) {
//		const Component::const_iterator it = m_comp[i].end(time, frameRate);
//		if (it != m_comp[i].end()) {
//			const int t = m_comp[i].isBasedOnFrame() ? msec(it.key(), frameRate) : it.key();
//			e = e == -1 ? t : qMin(e, t);
//		}
//	}
//	return e;
//}

RichString Subtitle::text(int time, double frameRate) const {
	if (m_comp.isEmpty())
		return QString();
	RichString text;
	for (int i=0; i<m_comp.size(); ++i) {
		const Component::const_iterator it = m_comp[i].start(time, frameRate);
		if (it != m_comp[i].end())
			text.merge(it.value().text);
	}
	return text;
}

bool Subtitle::load(const QString &file, const QString &enc) {
	const Pref &p = Pref::get();
	const double acc = p.sub_enc_accuracy*0.01;
	QString encoding;
	if (p.sub_enc_autodetection)
		encoding = CharsetDetector::detect(file, acc);
	if (encoding.isEmpty())
		encoding = enc;
	*this = parse(file, encoding);
	return !isEmpty();
}

Subtitle Subtitle::parse(const QString &file, const QString &enc) {
	Parser *parser = Parser::create(file);
	Subtitle sub;
	if (parser) {
		parser->setEncoding(enc);
		sub = parser->parse(file);
	}
	delete parser;
	return sub;
}

bool Subtitle::isEmpty() const {
	if (m_comp.isEmpty())
		return true;
	for (int i=0; i<m_comp.size(); ++i) {
		if (!m_comp[i].isEmpty())
			return false;
	}
	return true;
}
