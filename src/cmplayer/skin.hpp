#ifndef SKIN_HPP
#define SKIN_HPP

#include <QtCore/QObject>
#include "global.hpp"

class QWidget;		class QPoint;
class PlayEngine;	class AudioController;
class VideoRenderer;class Mrl;

enum Placeholder : int;

class Skin : public QObject {
	Q_OBJECT
public:
	Skin(QObject *parent = nullptr);
	~Skin();
	bool load(const QString &path, QWidget *parent = nullptr);
	QWidget *widget() const;
	QWidget *screen() const;
	void setVisible(bool visible);
	bool contains(const QPoint &pos) const;
	void connectTo(PlayEngine *engine, AudioController *audio, VideoRenderer *video);
	void initializePlaceholders();
public slots:
	void setMediaNumber(int number);
	void setTotalMediaCount(int count);
signals:
	void windowTitleChanged(const QString &title);
	void windowFilePathChanged(const QString &filePath);
private slots:
	void onDurationChanged(int duration);
	void onTick(int tick);
	void seek(int time);
	void onSeekableChanged(bool seekable);
	void onVolumeChanged(int volume);
	void setVolume(int volume);
	void onActionChanged();
	void onMrlChanged(const Mrl &mrl);
	void onStateChanged(State state);
private:
	static QString mediaStateText(State state);
	void setPlaceholder(Placeholder ph, const QString &text);
	void addHidableWidgets(QWidget *parent);
	static QWidget *topParentWidget(QWidget *widget);
	struct Data;
	Data *d;
};

#endif // SKIN_HPP
