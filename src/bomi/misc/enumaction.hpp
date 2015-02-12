#ifndef ENUMACTION_HPP
#define ENUMACTION_HPP

#include <QAction>

class BaseEnumAction : public QAction {
    Q_OBJECT
public:
    BaseEnumAction(QObject *parent = nullptr): QAction(parent) { }
};

template<class T>
class EnumInfo;

template<class T>
class EnumAction : public BaseEnumAction {
public:
    using Data = typename EnumInfo<T>::Data;
    using EnumInfo = ::EnumInfo<T>;
    EnumAction(T t, QObject *parent = nullptr);
    auto enum_() const -> T { return m_enum; }
    auto setData(const Data &data) -> void { m_data = data; }
    auto data() const -> const Data& { return m_data; }
    auto description() const -> QString { return EnumInfo::description(m_enum);}
private:
    T m_enum;
    Data m_data;
};

template<class T>
inline EnumAction<T>::EnumAction(T t, QObject *parent)
    : BaseEnumAction(parent)
    , m_enum(t)
    , m_data(EnumInfo::data(m_enum))
{
    QAction::setData(QVariant::fromValue<T>(m_enum));
}

template<class T>
SIA _NewEnumAction(T t) -> EnumAction<T>*
{
    return new EnumAction<T>(t);
}

#endif // ENUMACTION_HPP
