#ifndef PLAYERITEM_HPP
#define PLAYERITEM_HPP

#include "stdafx.hpp"
#include "skin.hpp"

class VideoRendererItem;
class PlayInfoItem;				class PlayEngine;

class PlayerItem : public QQuickItem, public Skin {
	Q_OBJECT
	Q_PROPERTY(PlayInfoItem *info READ info NOTIFY infoChanged)
	Q_PROPERTY(QQuickItem *infoView READ infoView WRITE setInfoView NOTIFY infoViewChanged)
	Q_PROPERTY(QString message READ message)
public:
	static void registerItems();
	void plugTo(PlayEngine *engine);
	void unplug();
	PlayerItem(QQuickItem *parent = nullptr);
	VideoRendererItem *renderer() const {return m_renderer;}
	PlayInfoItem *info() const {return m_info;}
	PlayEngine *engine() const {return m_engine;}
	QQuickItem *infoView() const {return m_infoView;}
	void setInfoView(QQuickItem *item) {if (m_infoView != item) emit infoViewChanged(m_infoView = item);}
	void setInfoVisible(bool v) {if (m_infoView) m_infoView->setVisible(v);}
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
	VideoRendererItem *m_renderer = nullptr;
	PlayInfoItem *m_info = nullptr;
	QQuickItem *m_infoView = nullptr;
	PlayEngine *m_engine = nullptr;
	QString m_message;
};

#endif // PLAYERITEM_HPP
