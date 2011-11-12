#ifndef EVENTS_HPP
#define EVENTS_HPP

#include <QtCore/QEvent>

class Event : public QEvent {
public:
	enum Type {Reopen = QEvent::User + 1, NewVideoFrame, VideoPrepare};
	Event(Type type): QEvent(static_cast<QEvent::Type>(type)) {}
	Type type() const {return static_cast<Type>(QEvent::type());}
};

class ReopenEvent: public Event {
public:
	ReopenEvent(): Event(Reopen) {}
};

class VideoFrame;	class VideoFormat;

class VideoPrepareEvent : public Event {
public:
	VideoPrepareEvent(): Event(VideoPrepare) {}
};

class NewVideoFrameEvent : public Event {
public:
	NewVideoFrameEvent(double y_min, double y_max)
		: Event(NewVideoFrame), y_min(y_min), y_max(y_max) {}
	double y_min, y_max;
};

//class VideoFrameEvent : public Event {
//public:
//	VideoFrameEvent(const ::VideoFrame &frame, uchar *buffer);
//	uchar *data() const {return m_data;}
//	int length() const {return m_length;}
//private:
//	int m_length;
//	uchar *m_data;
//};

#endif // EVENTS_HPP
