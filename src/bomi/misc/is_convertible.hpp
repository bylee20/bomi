#ifndef IS_CONVERTIBLE_HPP
#define IS_CONVERTIBLE_HPP

#include "tmp/type_traits.hpp"

template<class T>
class EnumInfo;

namespace tmp {

#define IS_VALID(exp) decltype(exp, std::true_type())
#define EXISTS(type, func) decltype(func<type>(0))::value
#define DEC_CHECK(func, exp) \
template<class S> \
static auto func(S* p) -> IS_VALID(exp); \
template<class> static auto func(...) -> std::false_type;

template<class T, bool is_enum = tmp::is_enum_class<T>()>
class convert_string {
    DEC_CHECK(_to1, p->toString())
    DEC_CHECK(_from1, S::fromString(QString()))
public:
    SCA available = EXISTS(T, _to1) && EXISTS(T, _from1);
    template<class S = T>
    static auto to(const QString &str) -> tmp::enable_if_t<EXISTS(S, _from1), S>
    { return S::fromString(str); }
    template<class S = T>
    static auto to(S &t, const QString &str) -> tmp::enable_if_t<EXISTS(S, _from1), void>
    { t = S::fromString(str); }
    template<class S = T>
    static auto from(const S &t) -> tmp::enable_if_t<EXISTS(S, _to1), QString>
    { return t.toString(); }
};

template<class T>
class convert_string<T, true> {
public:
    SCA available = true;
    static auto to(const QString &str) -> T { return EnumInfo<T>::from(str); }
    static auto from(T t) -> QString { return EnumInfo<T>::name(t); }
};

template<>
class convert_string<QString> {
public:
    SCA available = true;
    static auto to(QString &t, const QString &str) -> void { t = str; }
    static auto to(const QString &str) -> QString { return str; }
    static auto from(const QString &t) -> QString { return t; }
};

template<class T>
class convert_json {
    DEC_CHECK(_to1, p->toJson())
    DEC_CHECK(_from1, p->setFromJson(QJsonObject()))
    DEC_CHECK(_from2, S::fromJson(QJsonObject()))
    DEC_CHECK(_from3, p->setFromJson(QJsonArray()))
    DEC_CHECK(_fromv, p->setFromJson(QJsonValue()))
//    DEC_CHECK(_io, json_io(p)->toJson(*p))
public:
    SCA value = EXISTS(T, _fromv);
    SCA array = !value && EXISTS(T, _from3);
    SCA object = !value && (EXISTS(T, _from1) || EXISTS(T, _from2));
    SCA available = (EXISTS(T, _to1) && (value || array || object));// || EXISTS(T, _io);
    using JsonType = tmp::conditional_t<array,  QJsonArray,
                     tmp::conditional_t<object, QJsonObject,
                                                QJsonValue>>;
    template<class S = T>
    static auto to(S &t, const JsonType &json) -> tmp::enable_if_t<EXISTS(S, _from1) || array, void>
    { t.setFromJson(json); }
    template<class S = T>
    static auto to(S &t, const JsonType &json) -> tmp::enable_if_t<!EXISTS(S, _from1) && EXISTS(S, _from2), void>
    { t = S::fromJson(json); }
    template<class S = T>
    static auto to(const JsonType &json) -> tmp::enable_if_t<convert_json<S>::available, T>
    { T t; to(t, json); return t; }
    template<class S = T>
    static auto from(const S &t) -> tmp::enable_if_t<EXISTS(S, _to1), JsonType>
    { return t.toJson(); }
//    template<class S = T>
//    static auto to(S &t, const QJsonObject &json) -> tmp::enable_if_t<EXISTS(S, _io), void>
//    { json_io(&t)->fromJson(t, json); }
//    template<class S = T>
//    static auto from(S &t) -> tmp::enable_if_t<EXISTS(S, _io), QJsonObject>
//    { json_io(&t)->toJson(t); }
};
#undef IS_VALID
#undef DEC_CHECK
#undef EXISTS

template<class T>
SCIA is_convertible_to_string() noexcept -> bool
{ return convert_string<T>::available; }

template<class T>
SCIA is_convertible_to_json() noexcept -> bool
{ return convert_json<T>::available; }

}

#endif // IS_CONVERTIBLE_HPP
