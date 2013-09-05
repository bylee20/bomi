#ifndef DATAEVENT_HPP
#define DATAEVENT_HPP

#include "stdafx.hpp"

class BaseEvent : public QEvent {
public:
	BaseEvent(int type): QEvent(static_cast<Type>(type)) {}
};

template<typename D1>
class DataEvent1 : public BaseEvent {
public:
	DataEvent1(int type, const D1 &d1): BaseEvent(type), m_d1(d1) {}
	const D1 &data() const {return m_d1;}
	const D1 &data1() const {return m_d1;}
private:
	D1 m_d1;
};

template<typename D1, typename D2>
class DataEvent2 : public DataEvent1<D1> {
public:
	DataEvent2(int type, const D1 &d1, const D2 &d2): DataEvent1<D1>(type, d1), m_d2(d2) {}
	const D2 &data2() const {return m_d2;}
private:
	D2 m_d2;
};

template<typename D1, typename D2, typename D3>
class DataEvent3 : public DataEvent2<D1, D2> {
public:
	DataEvent3(int type, const D1 &d1, const D2 &d2, const D3 &d3): DataEvent2<D1, D2>(type, d1, d2), m_d3(d3) {}
	const D3 &data3() const {return m_d3;}
private:
	D3 m_d3;
};

static inline void postData(QObject *obj, int type) {
	qApp->postEvent(obj, new BaseEvent(type));
}

template<typename D1>
static inline void postData(QObject *obj, int type, const D1 &d1) {
	qApp->postEvent(obj, new DataEvent1<D1>(type, d1));
}

template<typename D1, typename D2>
static inline void postData(QObject *obj, int type, const D1 &d1, const D2 &d2) {
	qApp->postEvent(obj, new DataEvent2<D1, D2>(type, d1, d2));
}

template<typename D1, typename D2, typename D3>
static inline void postData(QObject *obj, int type, const D1 &d1, const D2 &d2, const D3 &d3) {
	qApp->postEvent(obj, new DataEvent3<D1, D2, D3>(type, d1, d2, d3));
}

template<typename D1, typename D2, typename D3>
static inline void getData(QEvent *event, D1 &d1, D2 &d2, D3 &d3) {
	auto e = static_cast<DataEvent3<D1, D2, D3>*>(event);
	d1 = e->data1(); d2 = e->data2(); d3 = e->data3();
}

template<typename D1, typename D2>
static inline void getData(QEvent *event, D1 &d1, D2 &d2) {
	auto e = static_cast<DataEvent2<D1, D2>*>(event);
	d1 = e->data1(); d2 = e->data2();
}
template<typename D1>
static inline void getData(QEvent *event, D1 &d1) {
	d1 = static_cast<DataEvent1<D1>*>(event)->data();
}

template<typename D1> static inline const D1 &getData(QEvent *event) { return static_cast<DataEvent1<D1>*>(event)->data(); }

#endif // DATAEVENT_HPP
