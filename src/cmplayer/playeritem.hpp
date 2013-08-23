#ifndef PLAYERITEM_HPP
#define PLAYERITEM_HPP

#include "stdafx.hpp"
#include "playinfoitem.hpp"
#include "skin.hpp"

class VideoRendererItem;
class PlayEngine;

class PlayerItem : public QQuickItem, public Skin {
	Q_OBJECT
	Q_ENUMS(State)
	Q_PROPERTY(QString osdMessage READ osdMessage)
	Q_PROPERTY(QString boxMessage READ boxMessage)
	Q_PROPERTY(MediaInfoObject *media READ media NOTIFY mediaChanged)
	Q_PROPERTY(AvInfoObject *audio READ audio NOTIFY videoChanged)
	Q_PROPERTY(AvInfoObject *video READ video NOTIFY audioChanged)
	Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
	Q_PROPERTY(int time READ position NOTIFY tick)
	Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
	Q_PROPERTY(bool muted READ isMuted NOTIFY mutedChanged)
	Q_PROPERTY(double volumeNormalizer READ volumeNormalizer)
	Q_PROPERTY(double avgsync READ avgsync)
	Q_PROPERTY(double avgfps READ avgfps)
	Q_PROPERTY(State state READ state NOTIFY stateChanged)
	Q_PROPERTY(QString stateText READ stateText NOTIFY stateTextChanged)
	Q_PROPERTY(bool playing READ isPlaying NOTIFY playingChanged)
	Q_PROPERTY(bool running READ isRunning NOTIFY runningChanged)
	Q_PROPERTY(double speed READ speed NOTIFY speedChanged)
//	Q_PROPERTY(bool paused READ isPaused NOTIFY pausedChanged)
//	Q_PROPERTY(bool stopped READ isStopped NOTIFY stoppedCahnged)
public:
	enum State {Stopped = 1, Playing = 2, Paused = 4, Finished = 8, Opening = 16, Buffering = 32, Error = 64, Preparing = 128};
	double avgsync() const;
	double avgfps() const;
	Q_INVOKABLE double bps(double fps) const;
	int duration() const {return m_duration;}
	int position() const {return m_position;}
	AvInfoObject *audio() const {return m_audio;}
	AvInfoObject *video() const {return m_video;}
	MediaInfoObject *media() const {return m_media;}
	State state() const {return m_state;}
	void set(const PlayEngine *engine);
	void setDuration(int duration) {if (m_duration != duration) emit durationChanged(m_duration = duration);}
	void setPosition(int pos) {if (m_position != pos) emit tick(m_position = pos);}
	void setState(State state);
	double volumeNormalizer() const;
	QString stateText() const;
	int volume() const {return m_volume;}
	bool isMuted() const {return m_muted;}
	double speed() const {return m_speed;}
	void setPlaylist(const PlaylistModel *playlist);
	static void registerItems();
	void plugTo(PlayEngine *engine);
	void unplug();
	bool isRunning() const {return m_running;}
	bool isPlaying() const {return m_playing;}
	PlayerItem(QQuickItem *parent = nullptr);
	VideoRendererItem *renderer() const {return m_renderer;}
	PlayEngine *engine() const {return m_engine;}
	void requestMessageOsd(const QString &message) {emit messageOsdRequested(m_omsg = message);}
	void requestMessageBox(const QString &message) {emit messageBoxRequested(m_bmsg = message);}
	void doneSeeking() {emit sought();}
	QString osdMessage() const {return m_omsg;}
	QString boxMessage() const {return m_bmsg;}
	Q_INVOKABLE void seek(int time);
	Q_INVOKABLE void setVolume(int volume);
	~PlayerItem();
signals:
	void stateTextChanged(const QString &stateText);
	void playingChanged(bool playing);
	void messageOsdRequested(const QString &message);
	void messageBoxRequested(const QString &message);
	void sought();
	void mutedChanged(bool muted);
	void nameChanged(const QString &name);
	void durationChanged(int duration);
	void tick(int pos);
	void videoChanged();
	void audioChanged();
	void runningChanged(bool running);
	void stateChanged(State state);
	void mediaChanged();
	void volumeChanged(int volume);
	void speedChanged(double speed);
private slots:
	void updateStateInfo();
private:
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	struct Data;
	Data *d;
	int m_duration = 0, m_position = 0, m_volume = 0;
	State m_state = Stopped;
	AvInfoObject *m_audio = new AvInfoObject(this), *m_video = new AvInfoObject(this);
	MediaInfoObject *m_media = new MediaInfoObject(this);
	VideoRendererItem *m_renderer = nullptr;
	PlayEngine *m_engine = nullptr;
	QString m_omsg, m_bmsg;
	bool m_running = false, m_playing = false, m_muted = false;
	double m_speed = 1.0;
};

#endif // PLAYERITEM_HPP
