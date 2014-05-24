#ifndef DATAEVENT_HPP
#define DATAEVENT_HPP

#include "stdafx.hpp"

template<class... Args>
class DataEvent : public QEvent {
public:
    using Data = std::tuple<Args...>;
    template<const int i>
    using DataType = typename std::tuple_element<i, Data>::type;
    static constexpr int size = sizeof...(Args);
    DataEvent(int type, const Args&... args)
        : QEvent(static_cast<Type>(type)), m_data(args...) { }
    auto get(Args&... args) -> void { std::tie(args...) = m_data; }
    template<int i>
    auto data() const -> const DataType<i>& { return std::get<i>(m_data); }
    auto tuple() const -> const Data& { return m_data; }
private:
    Data m_data;
};

template<class... Args>
static inline auto _PostEvent(QObject *obj, int type,
                              const Args&... args) -> void {
    qApp->postEvent(obj, new DataEvent<Args...>(type, args...));
}

template<class... Args>
static inline auto _PostEvent(Qt::EventPriority priority,QObject *obj, int type,
                              const Args&... args) -> void {
    qApp->postEvent(obj, new DataEvent<Args...>(type, args...), priority);
}

template<class... Args>
static inline auto _SendEvent(QObject *obj, int type,
                              const Args&... args) -> void {
    DataEvent<Args...> event(type, args...); qApp->sendEvent(obj, &event);
}

template<class... Args>
static inline auto _GetAllData(QEvent *event, Args&... args) -> void {
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
