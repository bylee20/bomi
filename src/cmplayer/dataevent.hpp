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
    DataEvent(int type, const Args&... args): QEvent(static_cast<Type>(type)), m_data(args...) {}
    void get(Args&... args) { std::tie(args...) = m_data; }
    template<int i>
    const DataType<i> &data() const { return std::get<i>(m_data); }
    const Data &tuple() const { return m_data; }
private:
    Data m_data;
};

template<typename... Args>
static inline void _PostEvent(QObject *obj, int type, const Args&... args) {
    qApp->postEvent(obj, new DataEvent<Args...>(type, args...));
}

template<typename... Args>
static inline void _SendEvent(QObject *obj, int type, const Args&... args) {
    DataEvent<Args...> event(type, args...); qApp->sendEvent(obj, &event);
}

template<typename... Args>
static inline void _GetAllData(QEvent *event, Args&... args) {
    static_cast<DataEvent<Args...>*>(event)->get(args...);
}

template<class T>
static inline const T &_GetData(QEvent *event) {
    return static_cast<DataEvent<T>*>(event)->template data<0>();
}

template<class T, class S, class... Args>
static inline const std::tuple<T, S, Args...> &_GetData(QEvent *event) {
    return static_cast<DataEvent<T, S, Args...>*>(event)->tuple();
}

using std::tie;

#endif // DATAEVENT_HPP
