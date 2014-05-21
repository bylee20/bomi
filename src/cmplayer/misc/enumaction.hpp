#ifndef ENUMACTION_HPP
#define ENUMACTION_HPP

#include "stdafx.hpp"
#include "enum/enums.hpp"

template<class T>
class EnumAction : public QAction {
public:
    using Data = typename EnumInfo<T>::Data;
    using EnumInfo = ::EnumInfo<T>;
    EnumAction(T t, QObject *parent = nullptr)
    : QAction(parent), m_enum(t), m_data(EnumInfo::data(m_enum)) {
        QAction::setData(QVariant::fromValue<T>(m_enum));
    }
    auto enum_() const -> T { return m_enum; }
    auto setData(const Data &data) -> void { m_data = data; }
    const Data &data() const { return m_data; }
    auto description() const -> QString { return EnumInfo::description(m_enum); }
private:
    T m_enum;
    Data m_data;
};

template<class T> static inline EnumAction<T> *_NewEnumAction(T t) { return new EnumAction<T>(t); }

#endif // ENUMACTION_HPP
