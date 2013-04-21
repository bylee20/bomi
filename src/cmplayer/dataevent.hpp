#ifndef DATAEVENT_HPP
#define DATAEVENT_HPP

#include "stdafx.hpp"

template<typename D1 = char, typename D2 = char, typename D3 = char>
class DataEvent : public QEvent {
public:
	using This = DataEvent<D1, D2, D3>;
	DataEvent(int type): QEvent((Type)type) {}
	DataEvent(int type, const D1 &d1): QEvent((Type)type), m_d1(d1) {}
	DataEvent(int type, const D1 &d1, const D2 &d2)
	: QEvent((Type)type), m_d1(d1), m_d2(d2) {}
	DataEvent(int type, const D1 &d1, const D2 &d2, const D3 &d3)
	: QEvent((Type)type), m_d1(d1), m_d2(d2), m_d3(d3) {}
	D1 data() const {return m_d1;}
	D1 data1() const {return m_d1;}
	D2 data2() const {return m_d2;}
	D3 data3() const {return m_d3;}
private:
	D1 m_d1; D2 m_d2; D3 m_d3;
};

template<typename D1 = char, typename D2 = char, typename D3 = char>
static void post(QObject *obj, int type, const D1 &d1 = 0, const D2 &d2 = 0, const D3 &d3 = 0) {
	qApp->postEvent(obj, new DataEvent<D1, D2, D3>(type, d1, d2, d3));
}

template<typename D1, typename D2, typename D3>
static void get(QEvent *event, D1 &d1, D2 &d2, D3 &d3) {
	auto e = static_cast<DataEvent<D1, D2, D3>*>(event);
	d1 = e->data1(); d2 = e->data2(); d3 = e->data3();
}

template<typename D1, typename D2>
static void get(QEvent *event, D1 &d1, D2 &d2) {
	auto e = static_cast<DataEvent<D1, D2>*>(event);
	d1 = e->data1(); d2 = e->data2();
}
template<typename D1>
static void get(QEvent *event, D1 &d1) {
	d1 = static_cast<DataEvent<D1>*>(event)->data();
}

#endif // DATAEVENT_HPP
