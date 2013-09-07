#ifndef PLAYINFOITEM_HPP
#define PLAYINFOITEM_HPP

#include "stdafx.hpp"
#include "mrl.hpp"
#include "playeritem.hpp"

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
	AvIoFormat(QObject *parent): QObject(parent) {}
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
	Q_PROPERTY(PlayerItem::HwAcc hardwareAcceleration READ hwAcc)
	Q_PROPERTY(QString codec READ codec)
	Q_PROPERTY(AvIoFormat *input READ input)
	Q_PROPERTY(AvIoFormat *output READ output)
	Q_PROPERTY(QString hardwareAccelerationText READ hwAccText)
	Q_PROPERTY(QString audioDriverText READ audioDriverText)
public:
	AvInfoObject(QObject *parent): QObject(parent) {setHwAcc(PlayerItem::Unavailable);}
	QString codec() const {return m_codec;}
	PlayerItem::HwAcc hwAcc() const {return m_hwAcc;}
	AvIoFormat *input() const {return m_input;}
	AvIoFormat *output() const {return m_output;}
	void setVideo(const PlayEngine *engine);
	void setAudio(const PlayEngine *engine);
	QString hwAccText() const { return m_hwAccText; }
	QString audioDriverText() const { return m_audioDriver; }
private:
	void setHwAcc(PlayerItem::HwAcc acc);
	static QString format(quint32 fmt) { return fmt >= 0x20202020 ? _U((const char*)&fmt, 4) : _L("0x") % _N(fmt, 16); }
	static QString bps(int Bps) {return (Bps ? _N(Bps*8/1000) % _L("kbps") : QString());}
	AvIoFormat *m_input = new AvIoFormat(this);
	AvIoFormat *m_output = new AvIoFormat(this);
	PlayerItem::HwAcc m_hwAcc = PlayerItem::Unavailable;
	QString m_codec, m_hwAccText, &m_audioDriver = m_hwAccText;
};

class MediaInfoObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(QString name READ name NOTIFY nameChanged)
public:
	MediaInfoObject(QObject *parent): QObject(parent) {}
	void setMrl (const Mrl &mrl) {m_mrl = mrl;}
	QString name() const {return m_name;}
	QString display() const {return m_mrl.displayName();}
	void setName(const QString &name) { if (_Change(m_name, name)) emit nameChanged(m_name); }
signals:
	void nameChanged(const QString &name);
private:
	friend class PlayerItem;
	QString m_name;
	Mrl m_mrl;
};

#endif // PLAYINFOITEM_HPP
