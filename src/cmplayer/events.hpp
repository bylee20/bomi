#ifndef EVENTS_HPP
#define EVENTS_HPP

#include "stdafx.hpp"

class VideoFrame;	class VideoFormat;

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



class VideoPrepareEvent : public Event {
public:
	VideoPrepareEvent(): Event(VideoPrepare) {}
};

//class NewVideoFrameEvent : public Event {
//public:
//	NewVideoFrameEvent(double y_min, double y_max)
//		: Event(NewVideoFrame), y_min(y_min), y_max(y_max) {}
//	double y_min, y_max;
//};

class NewVideoFrameEvent : public Event {
public:
	NewVideoFrameEvent(): Event(NewVideoFrame), frame(0) {}
	VideoFrame *frame;
};

#endif // EVENTS_HPP
