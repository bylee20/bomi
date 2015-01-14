#ifndef TYPE_INFO_HPP
#define TYPE_INFO_HPP

namespace tmp {

template<class T>
using remove_const_t = typename std::remove_const<T>::type;

template<class T>
using remove_reference_t = typename std::remove_reference<T>::type;

template<class T>
using remove_all_t = remove_const_t<remove_reference_t<T>>;

}

#endif // TYPE_INFO_HPP
