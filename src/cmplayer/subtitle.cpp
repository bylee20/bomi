#include "subtitle.hpp"
#include "subtitle_parser.hpp"
#include "global.hpp"
#include "pref.hpp"
#include "charsetdetector.hpp"
#include <QtCore/QStringBuilder>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>

QString SubtitleComponent::name() const {
	return m_klass.isEmpty() ? fileName() : fileName() % _L("(") % m_klass % _L(")");
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

SubtitleComponent &SubtitleComponent::unite(const SubtitleComponent &rhs, double fps) {
	SubtitleComponent &comp = *this;
	if (this == &rhs || rhs.isEmpty())
		return comp;
	else if (isEmpty()) {
		comp = rhs;
		return comp;
	}

	auto it1 = comp.begin();
	auto it2 = rhs.begin();
	const auto k2 = convertKeyBase(it2.key(), rhs.base(), m_base, fps);
	if (it2.key() < it1.key()) {
		while (k2 < it1.key()) {
			comp.insert(k2, *it2);
			++it2;
		}
	} else if (k2 == it1.key()){
		*it1 += *it2;
		++it2;
	} else
		it1 = --comp.lowerBound(k2);

	while (it2 != rhs.end()) {
		auto &cap1 = *it1;
//		const int ka = it1.key();
		const int kb = ++it1 != comp.end() ? it1.key() : -1;
//		Q_ASSERT(ka < k2);
		int k2 = 0;
		while (it2 != rhs.end()) {
			k2 = convertKeyBase(it2.key(), rhs.base(), m_base, fps);
			if (!(kb == -1 || k2 < kb))
				break;
			*comp.insert(k2, cap1) += *it2;
			++it2;
		}
		if (it2 == rhs.end())
			break;
		if (k2 == kb)
			*it1 += *it2++;
		else if (it2 != rhs.begin())
			*it1 += *(it2-1);
	}

	auto it = comp.begin();
	for (int idx = 0; it != comp.end(); ++idx, ++it)
		it->index = idx;
	return comp;
}

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
