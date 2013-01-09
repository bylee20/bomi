#ifndef PLAYERITEM_HPP
#define PLAYERITEM_HPP

#include "stdafx.hpp"

class SubtitleRendererItem;
class VideoRendererItem;
class PlayInfoItem;
class AudioController;
class PlayEngine;

class PlayerItem : public QQuickItem {
	Q_OBJECT
	Q_PROPERTY(SubtitleRendererItem *subtitle READ subtitle)
	Q_PROPERTY(VideoRendererItem *video READ video)
	Q_PROPERTY(PlayInfoItem *info READ info NOTIFY infoChanged)
	Q_PROPERTY(QQuickItem *infoView READ infoView WRITE setInfoView NOTIFY infoViewChanged)
	Q_PROPERTY(QString message READ message)
public:
	static void registerItems();
	void plug(PlayEngine *engine);
	PlayEngine *unplug();

	PlayerItem(QQuickItem *parent = nullptr);
	SubtitleRendererItem *subtitle() const {return m_subtitle;}
	VideoRendererItem *video() const {return m_video;}
	AudioController *audio() const {return m_audio;}
	PlayInfoItem *info() const {return m_info;}
	PlayEngine *engine() const {return m_engine;}
	QQuickItem *infoView() const {return m_infoView;}
	void setInfoView(QQuickItem *item) {if (m_infoView != item) emit infoViewChanged(m_infoView = item);}
	void setInfoVisible(bool v) {qDebug() << m_infoView; if (m_infoView) m_infoView->setVisible(v);}
	void requestMessage(const QString &message) {emit messageRequested(m_message = message);}
	void doneSeeking() {emit sought();}
	QString message() const {return m_message;}
	Q_INVOKABLE bool execute(const QString &key);
	Q_INVOKABLE void seek(int time);
	Q_INVOKABLE void setVolume(int volume);
signals:
	void messageRequested(const QString &message);
	void infoViewChanged(QQuickItem *item);
	void sought();
	void infoChanged();
private:
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	SubtitleRendererItem *m_subtitle = nullptr;
	VideoRendererItem *m_video = nullptr;
	PlayInfoItem *m_info = nullptr;
	AudioController *m_audio = nullptr;
	QQuickItem *m_infoView = nullptr;
	PlayEngine *m_engine = nullptr;
	QString m_message;
};

#endif // PLAYERITEM_HPP
