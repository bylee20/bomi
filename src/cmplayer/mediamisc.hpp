#ifndef MEDIAMISC_HPP
#define MEDIAMISC_HPP

#include "stdafx.hpp"
#include "mrl.hpp"

class PlayEngine;

class AvIoFormat : public QObject {
	Q_OBJECT
	Q_PROPERTY(QSize size READ size)
	Q_PROPERTY(QString type READ type)
	Q_PROPERTY(double bps READ bps)
	Q_PROPERTY(double fps READ fps)
	Q_PROPERTY(int bits READ bits)
	Q_PROPERTY(double samplerate READ samplerate)
	Q_PROPERTY(int channels READ channels)
public:
	AvIoFormat(QObject *parent = nullptr): QObject(parent) {}
	QSize size() const {return m_size;}
	double samplerate() const {return m_samplerate;}
	int bits() const {return m_bits;}
	int channels() const {return m_channels;}
	QString type() const {return m_type;}
	double bps() const {return m_bps;}
	double fps() const {return m_fps;}
private:
	friend class AvInfoObject;
	QSize m_size;
	QString m_type;
	double m_bps = 0;
	double m_fps = 0.0;
	double &m_samplerate = m_fps;
	int &m_bits = m_size.rwidth();
	int &m_channels = m_size.rheight();
};

class AvInfoObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString codec READ codec)
	Q_PROPERTY(AvIoFormat *input READ input)
	Q_PROPERTY(AvIoFormat *output READ output)
	Q_PROPERTY(QString audioDriverText READ audioDriverText)
public:
	AvInfoObject(QObject *parent = nullptr);
	QString codec() const {return m_codec;}
	AvIoFormat *input() const {return m_input;}
	AvIoFormat *output() const {return m_output;}
	void setVideo(const PlayEngine *engine);
	void setAudio(const PlayEngine *engine);
	QString audioDriverText() const { return m_audioDriver; }
private:
	void setHwAcc(int acc);
	static QString format(quint32 fmt) { return fmt >= 0x20202020 ? _U((const char*)&fmt, 4) : _L("0x") % _N(fmt, 16); }
	static QString bps(int Bps) {return (Bps ? _N(Bps*8/1000) % _L("kbps") : QString());}
	AvIoFormat *m_input = new AvIoFormat(this);
	AvIoFormat *m_output = new AvIoFormat(this);
	QString m_codec, m_hwAccText, &m_audioDriver = m_hwAccText;
};

class MediaInfoObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString name READ name NOTIFY nameChanged)
public:
	MediaInfoObject(QObject *parent = nullptr): QObject(parent) {}
	QString name() const {return m_name;}
	void setName(const QString &name) { if (_Change(m_name, name)) emit nameChanged(m_name); }
signals:
	void nameChanged(const QString &name);
private:
	QString m_name;
};

struct DvdInfo {
	struct Title {
		QString name() const {return m_name;}
		int id() const {return m_id;}
		int m_id = 0;
		int number = 0, chapters = 0, angles = 0, length = 0;
	private:
		friend class PlayEngine;
		QString m_name;
	};
	void clear() {titles.clear(); titles.clear(); }
	QMap<int, Title> titles;
	QString volume;
};

struct Chapter {
	QString name() const {return m_name;}
	int id() const {return m_id;}
	bool operator == (const Chapter &rhs) const {
		return m_id == rhs.m_id && m_name == rhs.m_name;
	}
private:
	friend class PlayEngine;
	QString m_name;
	int m_id = 0;
};

typedef QVector<Chapter> ChapterList;

#endif // MEDIAMISC_HPP
