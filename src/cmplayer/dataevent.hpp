#ifndef DATAEVENT_HPP
#define DATAEVENT_HPP

#include "stdafx.hpp"
#include <tuple>

template<typename... Args>
class DataEvent : public QEvent {
public:
	using Data = std::tuple<Args...>;
	template<const int i>
	using DataType = typename std::tuple_element<i, Data>::type;
	static constexpr int size = sizeof...(Args);
	DataEvent(int type, Args... args): QEvent(static_cast<Type>(type)), m_data(args...) {}
	void get(Args&... args) { std::tie(args...) = m_data; }
	template<int i>
	const DataType<i> &data() const { return std::get<i>(m_data); }
private:
	Data m_data;
};

template<typename... Args>
static inline void postData(QObject *obj, int type, const Args&... args) {
	qApp->postEvent(obj, new DataEvent<Args...>(type, args...));
}

template<typename... Args>
static inline void getAllData(QEvent *event, Args&... args) {
	static_cast<DataEvent<Args...>*>(event)->get(args...);
}

template<typename... Args, const int i = 0>
static inline const typename DataEvent<Args...>::template DataType<i> &getData(QEvent *event) {
	return static_cast<DataEvent<Args...>*>(event)->template data<i>();
}

#endif // DATAEVENT_HPP
