#ifndef DATAEVENT_HPP
#define DATAEVENT_HPP

template<class... Args>
class DataEvent : public QEvent {
public:
    using Data = std::tuple<Args...>;
    template<const int i>
    using DataType = typename std::tuple_element<i, Data>::type;
    static constexpr int size = sizeof...(Args);
    DataEvent(int type, const Args&... args)
        : QEvent(static_cast<Type>(type)), m_data(args...) { }
    auto take(Args&... args) -> void { std::tie(args...) = std::move(m_data); }
    template<int i>
    auto data() const -> const DataType<i>& { return std::get<i>(m_data); }
    template<int i>
    auto move() -> DataType<i>&& { return std::get<i>(std::move(m_data)); }
    auto tuple() const -> const Data& { return m_data; }
private:
    Data m_data;
};

template<class... Args>
SIA _PostEvent(QObject *obj, int type,
                              const Args&... args) -> void {
    qApp->postEvent(obj, new DataEvent<Args...>(type, args...));
}

template<class... Args>
SIA _PostEvent(Qt::EventPriority priority,QObject *obj, int type,
                              const Args&... args) -> void {
    qApp->postEvent(obj, new DataEvent<Args...>(type, args...), priority);
}

template<class... Args>
SIA _SendEvent(QObject *obj, int type,
                              const Args&... args) -> void {
    DataEvent<Args...> event(type, args...); qApp->sendEvent(obj, &event);
}

template<class... Args>
SIA _TakeData(QEvent *event, Args&... args) -> void {
    static_cast<DataEvent<Args...>*>(event)->take(args...);
}

template<class T>
static inline const T &_GetData(QEvent *event) {
    return static_cast<DataEvent<T>*>(event)->template data<0>();
}

template<class T>
static inline T &&_MoveData(QEvent *event) {
    return static_cast<DataEvent<T>*>(event)->template move<0>();
}

using std::tie;

#endif // DATAEVENT_HPP
