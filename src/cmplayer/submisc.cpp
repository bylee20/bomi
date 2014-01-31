#include "submisc.hpp"
#include "subtitle.hpp"

static const QString sep1("[::1::]"), sep2("[::2::]"), sep3("[::3::]");

template<typename List, typename F>
QString _Join(const List &list, F toString, const QString &sep) {
	QStringList l; l.reserve(list.size());
	for (auto &one : list)
		l.append(toString(one));
	return l.join(sep);
}

template<typename T, typename F>
QList<T> _Split(const QString &text, F fromString, const QString &sep, QString::SplitBehavior b = QString::KeepEmptyParts) {
	auto strs = text.split(sep, b);
	QList<T> list; list.reserve(strs.size());
	for (int i=0; i<strs.size(); ++i) {
		list.append(fromString(strs[i]));
	}
	return list;
}

QString SubtitleStateInfo::toString() const {
	QStringList list;
	list.append(_N(m_track));
	list.append(_Join(m_mpv, [] (const SubtitleFileInfo &info) { return info.toString(); }, sep2));
	for (auto it = m_cmplayer.begin(); it != m_cmplayer.end(); ++it) {
		QStringList item;
		item.append(it.key().toString());
		for (auto &comp : *it) {
			item.append(_N(comp.id));
			item.append(_N(comp.selected));
		}
		list.append(item.join(sep2));
	}
	return list.join(sep1);
}

SubtitleStateInfo SubtitleStateInfo::fromString(const QString &string) {
	if (string.isEmpty())
		return SubtitleStateInfo();
	auto list = string.split(sep1, QString::KeepEmptyParts);
	if (list.size() < 2)
		return SubtitleStateInfo();
	SubtitleStateInfo info;
	info.m_track = list[0].toInt();
	auto str2sfi = [] (const QString &s) { return SubtitleFileInfo::fromString(s); };
	info.m_mpv = _Split<SubtitleFileInfo>(list[1], str2sfi, sep2, QString::SkipEmptyParts);
	for (int i=2; i<list.size(); ++i) {
		auto item = list[i].split(sep2, QString::KeepEmptyParts);
		if (item.size() % 2 != 1)
			return SubtitleStateInfo();
		auto &map = info.m_cmplayer[str2sfi(item[0])];
		map.reserve(item.size()/2);
		Comp comp;
		for (int j=1; j<item.size();) {
			comp.id = item[j++].toInt();
			comp.selected = item[j++].toInt();
			map.append(comp);
		}
	}
	return info;
}

void SubtitleStateInfo::append(const SubComp &c) {
	m_cmplayer[c.fileInfo()].append({c.id(), c.selection()});
}

QList<SubComp> SubtitleStateInfo::load() const {
	auto selected = [] (const QList<Comp> &list, int id) {
		for (auto &c : list) { if (c.id == id) return c.selected; }
		return false;
	};
	QList<SubComp> loaded; 		Subtitle sub;
	for (auto it = m_cmplayer.begin(); it != m_cmplayer.end(); ++it) {
		if (!sub.load(it.key().path, it.key().encoding, -1))
			continue;
		if (sub.size() != it->size())
			continue;
		for (int i=0; i<sub.size(); ++i) {
			loaded.append(sub[i]);
			loaded.last().selection() = selected(*it, loaded.last().id());
		}
	}
	return loaded;
}
