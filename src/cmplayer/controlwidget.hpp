#ifndef CONTROLWIDGET_HPP
#define CONTROLWIDGET_HPP

#ifndef CONTROLWIDGET_H
#define CONTROLWIDGET_H

#include <QtGui/QWidget>
#include "global.hpp"

class PlayEngine;
class QVBoxLayout;			class QHBoxLayout;
class QGridLayout;			class Mrl;

class ControlWidget : public QWidget {
	Q_OBJECT
public:
	ControlWidget(PlayEngine *engine, QWidget *parent = 0);
	~ControlWidget();
	void connectMute(QAction *action);
	void connectPlay(QAction *action);
	void connectPrevious(QAction *action);
	void connectNext(QAction *action);
	void connectForward(QAction *action);
	void connectBackward(QAction *action);
public slots:
	void showMessage(const QString &msg, int time = 3000);
	void setMrl(const Mrl &mrl);
	void setState(MediaState state);
	void setDuration(int duration);
	void setPlayTime(int time);
	void setTrackNumber(int nth, int total);
private slots:
	void hideMessage();
	void updateMuted(bool muted);
private:
	static QVBoxLayout *vbox();
	static QHBoxLayout *hbox();
	static QGridLayout *grid();
	void retranslateUi();
	void changeEvent(QEvent *event);
	void paintEvent(QPaintEvent *event);
	class Slider;
	class Lcd;
	class Boundary;
	struct Data;
	Data *d;
};

#endif

#endif // CONTROLWIDGET_HPP
