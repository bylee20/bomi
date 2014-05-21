#ifndef ENUMACTION_HPP
#define ENUMACTION_HPP

#include "stdafx.hpp"
#include "enum/enums.hpp"

template<class T>
class EnumAction : public QAction {
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
    : QAction(parent)
    , m_enum(t)
    , m_data(EnumInfo::data(m_enum))
{
    QAction::setData(QVariant::fromValue<T>(m_enum));
}

template<class T>
static inline auto _NewEnumAction(T t) -> EnumAction<T>*
{
    return new EnumAction<T>(t);
}

#endif // ENUMACTION_HPP
