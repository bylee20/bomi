#ifndef PLAYINFOITEM_HPP
#define PLAYINFOITEM_HPP

#include "stdafx.hpp"
#include "mrl.hpp"
#include "skin.hpp"

class PlayEngine;			class VideoFormat;
class VideoRendererItem;	class PlaylistModel;

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
	Q_PROPERTY(bool isHardwareAccelerated READ hwaccel)
	Q_PROPERTY(QString codec READ codec)
	Q_PROPERTY(AvIoFormat *input READ input)
	Q_PROPERTY(AvIoFormat *output READ output)
public:
	AvInfoObject(QObject *parent): QObject(parent) {}
	QString codec() const {return m_codec;}
	bool hwaccel() const {return m_hwaccel;}
	AvIoFormat *input() const {return m_input;}
	AvIoFormat *output() const {return m_output;}
	void setVideo(const PlayEngine *engine);
	void setAudio(const PlayEngine *engine);
private:
	static QString format(quint32 fmt) {
		return fmt >= 0x20202020 ? _U((const char*)&fmt, 4) : _L("0x") % _N(fmt, 16);
	}
	static QString bps(int Bps) {return (Bps ? _N(Bps*8/1000) % _L("kbps") : QString());}
	AvIoFormat *m_input = new AvIoFormat(this);
	AvIoFormat *m_output = new AvIoFormat(this);
	bool m_hwaccel = false;
	QString m_codec;
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

//class PlayInfoItem : public QQuickItem, public Skin {
//	Q_OBJECT
//public:
//	Q_ENUMS(State)
//	enum State {Stopped = 1, Playing = 2, Paused = 4, Finished = 8, Opening = 16, Buffering = 32, Error = 64, Preparing = 128};
//private:
//	Q_PROPERTY(MediaInfoObject *media READ media NOTIFY mediaChanged)
//	Q_PROPERTY(AvInfoObject *audio READ audio NOTIFY videoChanged)
//	Q_PROPERTY(AvInfoObject *video READ video NOTIFY audioChanged)
//	Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
//	Q_PROPERTY(int time READ position NOTIFY tick)
//	Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
//	Q_PROPERTY(bool muted READ isMuted NOTIFY mutedChanged)
//	Q_PROPERTY(bool volumeNormalized READ isVolumeNormalized NOTIFY volumeNormalizedChanged)
//	Q_PROPERTY(bool fullScreen READ isFullScreen NOTIFY fullScreenChanged)
//	Q_PROPERTY(double volumeNormalizer READ volumeNormalizer)
//	Q_PROPERTY(double avgsync READ avgsync)
//	Q_PROPERTY(double avgfps READ avgfps)
//	Q_PROPERTY(double totalMemory READ totalMemory)
//	Q_PROPERTY(double memory READ memory)
//	Q_PROPERTY(double cpu READ cpu)
//	Q_PROPERTY(double avgbps READ avgbps)
//	Q_PROPERTY(State state READ state NOTIFY stateChanged)
//	Q_PROPERTY(QString stateText READ stateText)
//	Q_PROPERTY(QString monospace READ monospace)
//public:
//	PlayInfoItem(QQuickItem *parent = nullptr);
//	~PlayInfoItem();
//	double avgsync() const {return m_avSync;}
//	double avgfps() const {return m_fps;}
//	double avgbps() const {return m_bps;}
//	int duration() const {return m_duration;}
//	int position() const {return m_position;}
//	AvInfoObject *audio() const {return m_audio;}
//	AvInfoObject *video() const {return m_video;}
//	MediaInfoObject *media() const {return m_media;}
//	State state() const {return m_state;}
//	void set(const PlayEngine *engine);
//	void setDuration(int duration) {if (m_duration != duration) emit durationChanged(m_duration = duration);}
//	void setPosition(int pos) {if (m_position != pos) emit tick(m_position = pos);}
//	void setState(State state) {if (m_state != state) emit stateChanged(m_state = state);}
//	int totalMemory() const {return m_totmem;}
//	int memory() const {return m_mem;}
//	Q_INVOKABLE void collect();
//	Q_INVOKABLE QString msecToString(int ms) {return Pch::__null_time.addSecs(qRound((double)ms*1e-3)).toString(_L("h:mm:ss"));}
//	bool isVolumeNormalized() const {return m_volnorm;}
//	double volumeNormalizer() const {return m_norm;}
//	double cpu() const {return m_cpu;}
//	QString stateText() const;
//	QString monospace() const;
//	int volume() const {return m_volume;}
//	bool isFullScreen() const {return m_fullScreen;}
//	bool isMuted() const {return m_muted;}
//	void setPlaylist(const PlaylistModel *playlist);
//signals:
//	void mutedChanged(bool muted);
//	void nameChanged(const QString &name);
//	void durationChanged(int duration);
//	void tick(int pos);
//	void videoChanged();
//	void audioChanged();
//	void runningChanged(bool);
//	void stateChanged(State state);
//	void mediaChanged();
//	void volumeNormalizedChanged(bool volnorm);
//	void volumeChanged(int volume);
//	void fullScreenChanged(bool full);
//private:
//	VideoRendererItem *renderer() const;
//	struct Data;
//	Data *d;
//	int m_duration = 0, m_position = 0;
//	bool m_volnorm = false;
//	State m_state = Stopped;
//	AvInfoObject *m_audio = new AvInfoObject(this);
//	AvInfoObject *m_video = new AvInfoObject(this);
//	MediaInfoObject *m_media = new MediaInfoObject(this);
//	double m_norm = 1.0, m_fps = 1.0, m_cpu = 0;
//	double m_avSync = 0;
//	double m_totmem = 1, m_mem = 0; // MB
//	double m_bps = 0;
//	int m_volume = 0;
//	bool m_fullScreen = false, m_muted = false;
//};

#endif // PLAYINFOITEM_HPP
