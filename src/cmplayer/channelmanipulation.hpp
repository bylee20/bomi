#ifndef CHANNELMANIPULATION_HPP
#define CHANNELMANIPULATION_HPP

#include "stdafx.hpp"
extern "C" {
#include <audio/chmap.h>
}

enum class ChannelLayout;
enum class SpeakerId;

class ChannelManipulation {
public:
	using SourceArray = QVector<mp_speaker_id>;
	const SourceArray &sources(mp_speaker_id out) const { return m_mix[out]; }
	void set(mp_speaker_id dest, const SourceArray &src) { m_mix[dest] = src; }
	void set(mp_speaker_id dest, mp_speaker_id src) { m_mix[dest].resize(1); m_mix[dest][0] = src; }
	void add(mp_speaker_id dest, mp_speaker_id src) { m_mix[dest].append(src); }
	QString toString() const;
	static ChannelManipulation fromString(const QString &text);
	bool hasSources(mp_speaker_id dest) const { return !m_mix[dest].isEmpty(); }
private:
	friend class ChannelLayoutMap;
	std::array<SourceArray, MP_SPEAKER_ID_COUNT> m_mix;
};

class ChannelLayoutMap {
public:
	ChannelManipulation &operator () (ChannelLayout src, ChannelLayout dest) {
		return m_map[src][dest];
	}
	ChannelManipulation &operator () (const mp_chmap &src, const mp_chmap &dest);
	QString toString() const;
	static ChannelLayoutMap fromString(const QString &text);
	static ChannelLayoutMap default_();
	bool isEmpty() const { return m_map.isEmpty(); }
private:
	QMap<ChannelLayout, QMap<ChannelLayout, ChannelManipulation>> m_map;
};

class ChannelManipulationWidget : public QWidget {
	Q_OBJECT
public:
	ChannelManipulationWidget(QWidget *parent = nullptr);
	~ChannelManipulationWidget();
	void setMap(const ChannelLayoutMap &map);
	ChannelLayoutMap map() const;
	void setCurrentLayouts(ChannelLayout src, ChannelLayout dst);
private:
	struct Data;
	Data *d;
};

#endif // CHANNELMANIPULATION_HPP
