#ifndef TMP_HPP
#define TMP_HPP

namespace tmp { // simple template meta progamming

template<int... S>
struct index_list {
    template<int n>
    constexpr auto added() const -> decltype(index_list<S+n...>()) {
        return index_list<S+n...>();
    }
    template<int n>
    constexpr auto multiplied() const -> decltype(index_list<S+n...>()) {
        return index_list<S*n...>();
    }
};

namespace detail {

template<int diff, int N, int... S>
struct index_list_generate_interval : public index_list_generate_interval<diff, N-diff, N-diff, S...> { };

template<int diff, int ...S>
struct index_list_generate_interval<diff, 0, S...> { using type = index_list<S...>; };

template<class F, class... Args, int... I>
SIA call_with_tuple_impl(F &&func, std::tuple<Args...> &&tuple, index_list<I...>) -> void {
    func(std::get<I>(tuple)...);
}

template<class F, class... Args, int... I>
SIA call_with_tuple_impl(F &&func, std::tuple<Args...> &tuple, index_list<I...>) -> void {
    func(std::get<I>(tuple)...);
}

template<class F, class... Args, int... I>
SIA call_with_tuple_impl(F &&func, const std::tuple<Args...> &tuple, index_list<I...>) -> void {
    func(std::get<I>(tuple)...);
}

}

template<int N, int interval = 1>
SIA make_tuple_index()
        -> decltype(typename detail::index_list_generate_interval<interval, N%interval == 0 ? N : (N/interval+1)*interval>::type()) {
    return typename detail::index_list_generate_interval<interval, N%interval == 0 ? N : (N/interval+1)*interval>::type();
}

template<int interval = 1, class... Args>
SIA make_tuple_index(const std::tuple<Args...> &) -> decltype(make_tuple_index<sizeof...(Args), interval>()) {
    return make_tuple_index<sizeof...(Args), interval>();
}

template<class... Args, int... I>
SIA extract_tuple(const std::tuple<Args...> &tuple, index_list<I...>) -> decltype(std::tie(std::get<I>(tuple)...)) {
    return std::tie(std::get<I>(tuple)...);
}

template<class F, class... Args, int... I>
SIA call_with_tuple(const F &func, std::tuple<Args...> &&tuple, index_list<I...> index) -> void {
    detail::call_with_tuple_impl(func, tuple, index);
}

template<class F, class... Args, int... I>
SIA call_with_tuple(const F &func, std::tuple<Args...> &tuple, index_list<I...> index) -> void {
    detail::call_with_tuple_impl(func, tuple, index);
}

template<class F, class... Args, int... I>
SIA call_with_tuple(const F &func, const std::tuple<Args...> &tuple, index_list<I...> index) -> void {
    detail::call_with_tuple_impl(func, tuple, index);
}

template<class F, class... Args>
SIA call_with_tuple(const F &func, std::tuple<Args...> &&tuple) -> void {
    detail::call_with_tuple_impl(func, tuple, make_tuple_index(tuple));
}

template<class F, class... Args>
SIA call_with_tuple(const F &func, std::tuple<Args...> &tuple) -> void {
    detail::call_with_tuple_impl(func, tuple, make_tuple_index(tuple));
}

template<class F, class... Args>
SIA call_with_tuple(const F &func, const std::tuple<Args...> &tuple) -> void {
    detail::call_with_tuple_impl(func, tuple, make_tuple_index(tuple));
}

template<class... Args>
static inline constexpr auto tuple_size(const std::tuple<Args...> &) -> int { return sizeof...(Args); }

namespace detail {

template<int n, int size>
struct for_each_tuple_impl {
    template<class F, class... Args>
    SIA run(const std::tuple<Args...> &tuple, F &&func) -> void {
        func(std::get<n>(tuple));
        for_each_tuple_impl<n+1, size>::run(tuple, std::forward<F>(func));
    }
    template<class F, class... Args1, class... Args2>
    SIA run(const std::tuple<Args1...> &tuple1, const std::tuple<Args2...> &tuple2, F &&func) -> void {
        static_assert(sizeof...(Args1) == sizeof...(Args2), "tuple size does not match");
        func(std::get<n>(tuple1), std::get<n>(tuple2));
        for_each_tuple_impl<n+1, size>::run(tuple1, tuple2, std::forward<F>(func));
    }
};

template<int size>
struct for_each_tuple_impl<size, size> {
    template<class... Args> SIA run(Args&... ) -> void {}
    template<class... Args> SIA run(const Args&... ) -> void {}
    template<class... Args> SIA run(Args&&... ) -> void {}
};

struct make_json_impl {
    make_json_impl(QJsonObject &json): m_json(json) {}
    void operator () (const QString &key, const QJsonValue &value) const {
        m_json.insert(key, value);
    }
    void operator () (const char *key, const QJsonValue &value) const {
        m_json.insert(_L(key), value);
    }
    void operator () (const char *key, const char *value) const {
        m_json.insert(_L(key), _L(value));
    }
private:
    QJsonObject &m_json;
};

}

template<class F, class... Args>
auto for_each(const std::tuple<Args...> &tuple, const F &func) -> void {
    detail::for_each_tuple_impl<0, sizeof...(Args)>::run(tuple, func);
}

template<class F, class... Args1, class... Args2>
auto for_each(const std::tuple<Args1...> &tuple1, const std::tuple<Args2...> &tuple2, F &&func) -> void {
    detail::for_each_tuple_impl<0, sizeof...(Args1)>::run(tuple1, tuple2, std::forward<F>(func));
}

template<class... Args>
static auto make_json(const Args &... args) -> QJsonObject {
    const auto params = std::tie(args...);
    const auto keyIdx = tmp::make_tuple_index<2>(params);
    const auto valueIdx = keyIdx.template added<1>();
    const auto keys = tmp::extract_tuple(params, keyIdx);
    const auto values = tmp::extract_tuple(params, valueIdx);
    QJsonObject json;
    for_each(keys, values, detail::make_json_impl(json));
    return json;
}

}


#endif // TMP_HPP
