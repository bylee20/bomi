#ifndef GLOBAL_DEF_HPP
#define GLOBAL_DEF_HPP

class QRegularExpression;
class QRegularExpressionMatch;
class QRegularExpressionMatchIterator;

template<class State, class... Args>
using Signal = void(State::*)(Args...);

#define SIGNAL_T(T, name, ...) static_cast<void(T::*)(__VA_ARGS__)>(&T::name)
#define SIGNAL_VT(var, sig, ...) var, \
    SIGNAL_T(std::remove_pointer<std::remove_reference<decltype(var)>::type>::type, sig, ##__VA_ARGS__)
#define SIGNAL_V(var, sig) var, &std::remove_pointer<std::remove_reference<decltype(var)>::type>::type::sig

#define SIA static inline auto
#define SCA static constexpr auto
#define SCIA static constexpr inline auto
#define CIA constexpr inline auto

using QRegEx = QRegularExpression;
using QRegExMatch = QRegularExpressionMatch;
using QRegExMatchIterator = QRegularExpressionMatchIterator;

#define DECL_PLUG_CHANGED_T(Q, from, ...) \
namespace Global { \
template<class T, class... Args> \
SIA _PlugChanged(const Q *q, const T *t, void(T::*sig)(Args...)) -> void \
{ QObject::connect(q, SIGNAL_T(Q, from, ##__VA_ARGS__), t, sig); }}

#define DECL_PLUG_CHANGED(Q, from) \
namespace Global { \
template<class T, class... Args> \
SIA _PlugChanged(const Q *q, const T *t, void(T::*sig)(Args...)) -> void \
{ QObject::connect(q, &Q::from, t, sig); }}

#define PLUG_CHANGED(Q) _PlugChanged(Q, this, signal)

namespace Global {

namespace global_detail {
template<class T>
struct GenericEq {
    static auto eq(const T *, const T *) -> bool { return true; }
    template<class S, class... Args>
    static auto eq(const T *lhs, const T *rhs, S pm, Args... args) -> bool
        { return (lhs->*pm) == (rhs->*pm) && eq(lhs, rhs, args...); }
};
}
#define DECL_EQ(type, ...) \
    using T = type; \
    auto operator == (const T &rhs) const -> bool \
        { return global_detail::GenericEq<T>::eq(this, &rhs, __VA_ARGS__); } \
    auto operator != (const T &rhs) const -> bool \
        { return !operator == (rhs); }

#define _R std::tie

}

#endif // GLOBAL_DEF_HPP

