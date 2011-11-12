#ifndef ABREPEATER_HPP
#define ABREPEATER_HPP

#include <QtCore/QObject>

class PlayEngine;		class Subtitle;
class SubtitleRenderer;

class ABRepeater : public QObject {
	Q_OBJECT
public:
	ABRepeater(PlayEngine *engine, const SubtitleRenderer *sub);
	~ABRepeater();
	bool repeat(int a, int b, int times = -1) {m_a = a; m_b = b; return start(times);}
	bool isRepeating() {return m_repeating;}
	int a() const {return m_a;}
	int b() const {return m_b;}
	bool hasA() const {return m_a >= 0;}
	bool hasB() const {return m_b >= 0;}
	int restTimes() const {return m_times - m_nth;}
	int times() const {return m_times;}
public slots:
	void stop();
	bool start(int times = -1);
	int setAToCurrentTime();
	int setBToCurrentTime();
	int setAToSubtitleTime();
	int setBToSubtitleTime();
	void setA(int a) {m_a = a;}
	void setB(int b) {m_b = b;}
signals:
	void repeated(int rest);
	void stopped();
	void started();
private slots:
	void slotTick(int time);
private:
	PlayEngine *m_engine;
	int m_a, m_b;
	bool m_repeating;
	int m_times, m_nth;
	const SubtitleRenderer *m_sub;
};

#endif // ABREPEATER_HPP
