#include "subtitle.hpp"
#include "subtitle_parser.hpp"
#include "global.hpp"
#include "pref.hpp"
#include "charsetdetector.hpp"
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>


//	if (m_lang.id().isEmpty())
//		return QFileInfo(m_file).fileName();
//	else
//		return QFileInfo(m_file).fileName() + " (" + m_lang.id() + ')';
//}

//Subtitle::Component Subtitle::component(double frameRate) const {
//	if (m_comp.isEmpty())
//		return Component();
//	Component comp;
//	for (int i=0; i<m_comp.size(); ++i)
//		comp.unite(m_comp[i], frameRate);
//	return comp;
//}

//Subtitle::Component::const_iterator Subtitle::Component::start(int time, double frameRate) const {
//	if (isEmpty() || time < 0)
//		return end();
//	return --end(time, frameRate);
//}

//Subtitle::Component::const_iterator Subtitle::Component::end(int time, double frameRate) const {
//	if (isEmpty() || time < 0)
//		return end();
//	int key = time;
//	if (m_base == Frame) {
//		if (frameRate < 0.0)
//			return end();
//		key = qRound(time*0.001*frameRate);
//	}
//	return upperBound(key);
//}

//Subtitle::Component &Subtitle::Component::unite(const Component &other, double frameRate) {
//	if (this == &other || other.isEmpty())
//		return *this;
//	else if (isEmpty()) {
//		*this = other;
//		return *this;
//	}
//	ComponentIterator it1(*this);
//	ComponentIterator it2(other);
//	int k3 = -1;
//	while(it1.hasNext()) {
//		int k1 = it1.next().key();
//		int k2 = it1.hasNext() ? it1.peekNext().key() : -1;
//		if (k3 != -1 && it2.hasPrevious())
//			(*this)[k1].text = it1.value().text.merged(it2.peekPrevious().value().text);
//		while(it2.hasNext()) {
//			k3 = convertKeyBase(it2.next().key(), other.base(), m_base, frameRate);
//			if (k2 == -1)
//				(*this)[k3].text = it1.value().text.merged(it2.value().text);
//			else if (k3 >= k2) {
//				it2.previous();
//				break;
//			} else if (k3 == k1)
//				(*this)[k1].text = it1.value().text.merged(it2.value().text);
//			else if (k3 > k1)
//				(*this)[k3].text = it1.value().text.merged(it2.value().text);
//			else if (k3 < k1)
//				(*this)[k3].text = it2.value().text;
//		}
//	}
//	Component::iterator it = begin();
//	for (int idx = 0; it != end(); ++idx, ++it)
//		it->index = idx;
//	return *this;
//}

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

//RichString Subtitle::text(int time, double frameRate) const {
//	if (m_comp.isEmpty())
//		return QString();
//	RichString text;
//	for (int i=0; i<m_comp.size(); ++i) {
//		const Component::const_iterator it = m_comp[i].start(time, frameRate);
//		if (it != m_comp[i].end())
//			text.merge(it.value().text);
//	}
//	return text;
//}

//bool Subtitle::load(const QString &file, const QString &enc) {
//	const Pref &p = Pref::get();
//	const double acc = p.sub_enc_accuracy*0.01;
//	QString encoding;
//	if (p.sub_enc_autodetection)
//		encoding = CharsetDetector::detect(file, acc);
//	if (encoding.isEmpty())
//		encoding = enc;
//	*this = parse(file, encoding);
//	return !isEmpty();
//}

//Subtitle Subtitle::parse(const QString &file, const QString &enc) {
//	Parser *parser = Parser::create(file);
//	Subtitle sub;
//	if (parser) {
//		parser->setEncoding(enc);
//		sub = parser->parse(file);
//	}
//	delete parser;
//	return sub;
//}

//bool Subtitle::isEmpty() const {
//	if (m_comp.isEmpty())
//		return true;
//	for (int i=0; i<m_comp.size(); ++i) {
//		if (!m_comp[i].isEmpty())
//			return false;
//	}
//	return true;
//}









QString SubtitleComponent::name() const {
	return m_klass;
//	if (m_lang.id().isEmpty())
//		return QFileInfo(m_file).fileName();
//	else
//		return QFileInfo(m_file).fileName() + " (" + m_lang.id() + ')';
}

SubtitleComponent Subtitle::component(double frameRate) const {
	if (m_comp.isEmpty())
		return SubtitleComponent();
	SubtitleComponent comp;
	for (int i=0; i<m_comp.size(); ++i)
		comp.unite(m_comp[i], frameRate);
	return comp;
}


SubtitleComponent::SubtitleComponent(const QString &file, SyncType base)
: m_file(file), m_base(base) {
	(*this)[0].index = 0;
	m_flag = false;
}

SubtitleComponent SubtitleComponent::united(const SubtitleComponent &other, double frameRate) const {
	return SubtitleComponent(*this).unite(other, frameRate);
}

SubtitleComponent::const_iterator SubtitleComponent::start(int time, double frameRate) const {
	if (isEmpty() || time < 0)
		return end();
	return --finish(time, frameRate);
}

SubtitleComponent::const_iterator SubtitleComponent::finish(int time, double frameRate) const {
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

SubtitleComponent &SubtitleComponent::unite(const SubtitleComponent &rhs, double frameRate) {
	SubtitleComponent &comp = *this;
	if (this == &rhs || rhs.isEmpty())
		return comp;
	else if (isEmpty()) {
		comp = rhs;
		return comp;
	}

	auto it1 = comp.begin();
	auto it2 = rhs.begin();
	if (it2.key() < it1.key()) {
		while (it2.key() < it1.key()) {
			comp.insert(it2.key(), *it2);
			++it2;
		}
	} else if (it2.key() == it1.key()){
		*it1 += *it2;
		++it2;
	} else
		it1 = --comp.lowerBound(it2.key());

	while (it2 != rhs.end()) {
		auto &cap1 = *it1;
		const int ka = it1.key();
		const int kb = ++it1 != comp.end() ? it1.key() : -1;
		Q_ASSERT(ka < it2.key());
		while (it2 != rhs.end() && (kb == -1 || it2.key() < kb)) {
			*comp.insert(it2.key(), cap1) += *it2;
			++it2;
		}
		if (it2 == rhs.end())
			break;
		if (it2.key() == kb)
			*it1 += *it2++;
		else if (it2 != rhs.begin())
			*it1 += *(it2-1);
	}

	auto it = comp.begin();
	for (int idx = 0; it != comp.end(); ++idx, ++it)
		it->index = idx;
	return comp;
}

//SubtitleComponent &SubtitleComponent::unite(const SubtitleComponent &rhs, double frameRate) {
//	SubtitleComponent &comp = *this;
//	if (this == &rhs || rhs.isEmpty())
//		return comp;
//	else if (isEmpty()) {
//		comp = rhs;
//		return comp;
//	}
//	SubtitleComponentIterator it1(comp);
//	SubtitleComponentIterator it2(rhs);
//	int k3 = -1;
//	typedef RichTextDocument Caption;
//	auto setMerged = [&comp] (int key, const Caption &b1, const Caption &b2) {
//		(RichTextDocument&)comp[key] = (RichTextDocument&)b1;
//		(RichTextDocument&)comp[key] += (RichTextDocument&)b2;
//	};
//	while(it1.hasNext()) {
//		int k1 = it1.next().key();
//		int k2 = it1.hasNext() ? it1.peekNext().key() : -1;
//		if (k3 != -1 && it2.hasPrevious())
//			setMerged(k1, it1.value(), it2.peekPrevious().value());
//		while(it2.hasNext()) {
//			k3 = convertKeyBase(it2.next().key(), rhs.base(), m_base, frameRate);
//			if (k2 == -1)
//				setMerged(k3, it1.value(), it2.value());
//			else if (k3 >= k2) {
//				it2.previous();
//				break;
//			} else if (k3 == k1)
//				setMerged(k1, it1.value(), it2.value());
//			else if (k3 > k1)
//				setMerged(k3, it1.value(), it2.value());
//			else if (k3 < k1)
//				(RichTextDocument&)comp[k3] = (RichTextDocument&)it2.value();
//		}
//	}
//	auto it = comp.begin();
//	for (int idx = 0; it != comp.end(); ++idx, ++it)
//		it->index = idx;
//	return comp;
//}

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

RichTextDocument Subtitle::caption(int time, double fps) const {
	if (m_comp.isEmpty())
		return RichTextDocument();
	RichTextDocument caption;
	for (int i=0; i<m_comp.size(); ++i) {
		const auto it = m_comp[i].start(time, fps);
		if (it != m_comp[i].end())
			caption += *it;
	}
	return caption;
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
	return SubtitleParser::parse(file, enc);
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
