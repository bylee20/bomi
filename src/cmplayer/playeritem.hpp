#ifndef PLAYERITEM_HPP
#define PLAYERITEM_HPP

#include "stdafx.hpp"
#include "playinfoitem.hpp"
#include "skin.hpp"
#include "resourcemonitor.hpp"

class VideoRendererItem;
class PlayEngine;

class PlayerItem : public QQuickItem, public Skin {
	Q_OBJECT
	Q_ENUMS(State)

//	Q_PROPERTY(PlayInfoItem *info READ info NOTIFY infoChanged)
//	Q_PROPERTY(QQuickItem *infoView READ infoView WRITE setInfoView NOTIFY infoViewChanged)
	Q_PROPERTY(QString message READ message)

	Q_PROPERTY(MediaInfoObject *media READ media NOTIFY mediaChanged)
	Q_PROPERTY(AvInfoObject *audio READ audio NOTIFY videoChanged)
	Q_PROPERTY(AvInfoObject *video READ video NOTIFY audioChanged)
	Q_PROPERTY(int duration READ duration NOTIFY durationChanged)
	Q_PROPERTY(int time READ position NOTIFY tick)
	Q_PROPERTY(int volume READ volume NOTIFY volumeChanged)
	Q_PROPERTY(bool muted READ isMuted NOTIFY mutedChanged)
	Q_PROPERTY(bool volumeNormalized READ isVolumeNormalized NOTIFY volumeNormalizedChanged)
	Q_PROPERTY(bool fullScreen READ isFullScreen NOTIFY fullScreenChanged)
	Q_PROPERTY(double volumeNormalizer READ volumeNormalizer)
	Q_PROPERTY(double avgsync READ avgsync)
	Q_PROPERTY(double avgfps READ avgfps)
	Q_PROPERTY(double totalMemory READ totalMemory)
	Q_PROPERTY(double memory READ memory)
	Q_PROPERTY(double cpu READ cpu)
	Q_PROPERTY(double avgbps READ avgbps)
	Q_PROPERTY(State state READ state NOTIFY stateChanged)
	Q_PROPERTY(QString stateText READ stateText)
public:
	enum State {Stopped = 1, Playing = 2, Paused = 4, Finished = 8, Opening = 16, Buffering = 32, Error = 64, Preparing = 128};
	double avgsync() const {return m_avSync;}
	double avgfps() const {return m_fps;}
	double avgbps() const {return m_bps;}
	int duration() const {return m_duration;}
	int position() const {return m_position;}
	AvInfoObject *audio() const {return m_audio;}
	AvInfoObject *video() const {return m_video;}
	MediaInfoObject *media() const {return m_media;}
	State state() const {return m_state;}
	void set(const PlayEngine *engine);
	void setDuration(int duration) {if (m_duration != duration) emit durationChanged(m_duration = duration);}
	void setPosition(int pos) {if (m_position != pos) emit tick(m_position = pos);}
	void setState(State state) {if (m_state != state) emit stateChanged(m_state = state);}
	int totalMemory() const {return m_totmem;}
	int memory() const {return m_mem;}
	Q_INVOKABLE void collect();
	bool isVolumeNormalized() const {return m_volnorm;}
	double volumeNormalizer() const {return m_norm;}
	double cpu() const {return m_cpu;}
	QString stateText() const;
	int volume() const {return m_volume;}
	bool isFullScreen() const {return m_fullScreen;}
	bool isMuted() const {return m_muted;}
	void setPlaylist(const PlaylistModel *playlist);
	static void registerItems();
	void plugTo(PlayEngine *engine);
	void unplug();
	PlayerItem(QQuickItem *parent = nullptr);
	VideoRendererItem *renderer() const {return m_renderer;}
	PlayEngine *engine() const {return m_engine;}
//	void setInfoView(QQuickItem *item) {if (m_infoView != item) emit infoViewChanged(m_infoView = item);}
//	void setInfoVisible(bool v) {if (m_infoView) m_infoView->setVisible(v);}
	void requestMessage(const QString &message) {emit messageRequested(m_message = message);}
	void doneSeeking() {emit sought();}
	QString message() const {return m_message;}
	Q_INVOKABLE bool execute(const QString &key);
	Q_INVOKABLE void seek(int time);
	Q_INVOKABLE void setVolume(int volume);
	~PlayerItem();
signals:
	void messageRequested(const QString &message);
	void sought();
	void mutedChanged(bool muted);
	void nameChanged(const QString &name);
	void durationChanged(int duration);
	void tick(int pos);
	void videoChanged();
	void audioChanged();
	void runningChanged(bool);
	void stateChanged(State state);
	void mediaChanged();
	void volumeNormalizedChanged(bool volnorm);
	void volumeChanged(int volume);
	void fullScreenChanged(bool full);
private:
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	struct Data;
	Data *d;
	int m_duration = 0, m_position = 0;
	bool m_volnorm = false;
	State m_state = Stopped;
	AvInfoObject *m_audio = new AvInfoObject(this);
	AvInfoObject *m_video = new AvInfoObject(this);
	MediaInfoObject *m_media = new MediaInfoObject(this);
	double m_norm = 1.0, m_fps = 1.0, m_cpu = 0;
	double m_avSync = 0;
	double m_totmem = 1, m_mem = 0; // MB
	double m_bps = 0;
	int m_volume = 0;
	bool m_fullScreen = false, m_muted = false;
	VideoRendererItem *m_renderer = nullptr;
	PlayEngine *m_engine = nullptr;
	QString m_message;
};

#endif // PLAYERITEM_HPP
