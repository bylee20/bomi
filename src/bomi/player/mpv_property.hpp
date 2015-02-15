#ifndef MPV_PROPERTY_HPP
#define MPV_PROPERTY_HPP

#include <libmpv/client.h>
#include "tmp/type_traits.hpp"

namespace detail {

enum MpvEnc {
    Utf8, Latin1, Local8Bit
};

template<MpvEnc me>
struct mpv_str {
    static inline auto str_to_mpv(const QString &str) -> QByteArray;
    static inline auto str_from_mpv(const char *data, int len) -> QString;
};

#define DECL_MPV_STR(e) \
template<> \
struct mpv_str<e> { \
    static inline auto str_to_mpv(const QString &str) -> QByteArray \
        { return str.to##e(); } \
    static inline auto str_from_mpv(const char *data, int len) -> QString \
        { return QString::from##e(data, len); } \
};

DECL_MPV_STR(Utf8)
DECL_MPV_STR(Latin1)
DECL_MPV_STR(Local8Bit)

template<MpvEnc me>
struct MpvString {
    using T = MpvString<me>;
    MpvString() = default;
    MpvString(const MpvString &other): data(other.data) { }
    MpvString(MpvString &&other): data(std::move(other.data)) { }
    MpvString(const QString &data): data(data) { }
    MpvString(QString &&data): data(std::move(data)) { }
    operator QString() const { return data; }
    operator QString&() { return data; }
    operator const QString&() const { return data; }
    auto operator = (const T &rhs) -> T&
        { data = rhs.data; return *this; }
    auto operator = (T &&rhs) -> T&
        { data = std::move(rhs.data); return *this; }
    auto operator = (const QString &rhs) -> T&
        { data = rhs; return *this; }
    auto operator = (QString &&rhs) -> T&
        { data = std::move(rhs); return *this; }
    auto operator == (const T &rhs) const -> bool
        { return data == rhs.data; }
    auto operator != (const T &rhs) const -> bool
        { return data != rhs.data; }
    auto operator == (const QString &rhs) const -> bool
        { return data == rhs; }
    auto operator != (const QString &rhs) const -> bool
        { return data != rhs; }
    auto toMpv() const -> QByteArray
        { return mpv_str<me>::str_to_mpv(data); }
    static auto fromMpv(const char *mpv, int len = -1) -> MpvString<me>
        { return mpv_str<me>::str_from_mpv(mpv, len); }
    static auto fromMpv(const QByteArray &data) -> MpvString<me>
        { return fromMpv(data.data(), data.size()); }
    QString data;
};

}

template<detail::MpvEnc me>
auto operator == (const QString &lhs, const detail::MpvString<me> &rhs) -> bool
    { return lhs == rhs.data; }
template<detail::MpvEnc me>
auto operator != (const QString &lhs, const detail::MpvString<me> &rhs) -> bool
    { return lhs != rhs.data; }

using MpvUtf8 = detail::MpvString<detail::Utf8>;
using MpvLocal8Bit = detail::MpvString<detail::Local8Bit>;
using MpvLatin1 = detail::MpvString<detail::Latin1>;

#ifdef Q_OS_LINUX
using MpvFile = MpvLocal8Bit;
#else
using MpvFile = MpvUtf8;
#endif

SIA _ToLog(const MpvFile &file) -> QByteArray { return _ToLog(file.data); }

struct MpvFileList {
    MpvFileList() = default;
    MpvFileList(const QStringList &names): names(names) { }
    MpvFileList(QStringList &&names): names(std::move(names)) { }
    QStringList names;
};

namespace detail {

enum MpvTraitType {
    Arithmetic,
    List,
    Extra
};

template<class T>
struct IsList : std::false_type { };
template<class T>
struct IsList<QList<T>> : std::true_type { };
template<class T>
struct IsList<QVector<T>> : std::true_type { };

template<class T>
static constexpr inline auto mpv_trait_type() -> MpvTraitType
{
    return tmp::is_arithmetic<T>() ? Arithmetic
       : IsList<T>::value ? List : Extra;
}

struct mpv_trait_no_alloc {
    template<class S>
    static auto set_free(S&) { };
    static auto node_free(mpv_node &) { };
};

}

template<class T, detail::MpvTraitType type = detail::mpv_trait_type<T>()>
struct mpv_trait { };

template<class T>
struct mpv_trait<T, detail::Arithmetic> : detail::mpv_trait_no_alloc {
    SCA IsBool = tmp::is_same<T, bool>();
    SCA IsInt = tmp::is_integral<T>();
    using mpv_type = tmp::conditional_t<IsBool, int,
                     tmp::conditional_t<IsInt, qint64, double>>;
    static constexpr mpv_format format = IsBool ? MPV_FORMAT_FLAG
                                       : IsInt ? MPV_FORMAT_INT64
                                               : MPV_FORMAT_DOUBLE;
    static auto get_free(mpv_type&) { };
    static auto get(T &t, mpv_type data) { t = data; }
    static auto set(mpv_type &data, T t) { data = t; }
    static auto node_fill(mpv_node &node, T t)
    {
        node.format = format;
        switch (format) {
        case MPV_FORMAT_FLAG:
            node.u.flag = t;
            break;
        case MPV_FORMAT_INT64:
            node.u.int64 = t;
            break;
        case MPV_FORMAT_DOUBLE:
            node.u.double_ = t;
            break;
        default:
            Q_ASSERT(false);
        }
    }
};

template<>
struct mpv_trait<QByteArray> : detail::mpv_trait_no_alloc {
    using mpv_type = const char*;
    static constexpr mpv_format format = MPV_FORMAT_STRING;
    static auto get_free(mpv_type &data) { mpv_free((void*)data); }
    static auto get(QByteArray &a, const mpv_type &data)
        { a = QByteArray(data); }
    static auto set(mpv_type &data, const QByteArray &t) { data = t.data(); }
    static auto node_fill(mpv_node &node, const QByteArray &t)
    { node.format = format; set((const char*&)node.u.string, t); }
};

namespace detail {

template<MpvEnc me>
struct mpv_trait_string {
    using T = MpvString<me>;
    using mpv_type = const char*;
    static constexpr mpv_format format = MPV_FORMAT_STRING;
    static auto get_free(mpv_type &data) { mpv_free((void*)data); }
    static auto get(T &s, const mpv_type &data) { s = T::fromMpv(data); }
// never set string directly
};

}

template<>
struct mpv_trait<MpvLatin1> : detail::mpv_trait_string<detail::Latin1> { };
template<>
struct mpv_trait<MpvUtf8> : detail::mpv_trait_string<detail::Utf8> { };
template<>
struct mpv_trait<MpvLocal8Bit> : detail::mpv_trait_string<detail::Local8Bit> { };

template<>
struct mpv_trait<MpvFileList> {
    using ST = mpv_trait<MpvFile>;
    using mpv_type = mpv_node;
    SCA format = MPV_FORMAT_NODE;
    static auto get(MpvFileList &t, const mpv_node &data) {
        if (data.format != MPV_FORMAT_NODE_ARRAY)
            return;
        t.names.clear();
        auto array = data.u.list;
        t.names.reserve(array->num);
        for (int i = 0; i < array->num; ++i) {
            if (array->values[i].format == MPV_FORMAT_STRING)
                t.names.push_back(MpvFile::fromMpv(array->values[i].u.string).data);
        }
    }
    static auto get_free(mpv_node &data) { mpv_free_node_contents(&data); }
    static auto set(mpv_node &data, const MpvFileList &t) -> void
    {
        data.format = MPV_FORMAT_NODE_ARRAY;
        data.u.list = new mpv_node_list;
        auto list = data.u.list;
        list->num = t.names.size();
        list->keys = nullptr;
        list->values = new mpv_node[t.names.size()];
        for (int i = 0; i < t.names.size(); ++i)
            node_fill(list->values[i], t.names[i]);
    }
    static auto set_free(mpv_node &data) -> void
    {
        for (int i = 0; i < data.u.list->num; ++i)
            node_free(data.u.list->values[i]);
        delete [] data.u.list->values;
        delete data.u.list;
    }
private:
    static auto node_fill(mpv_node &node, const MpvFile &t) -> void
    {
        node.format = MPV_FORMAT_STRING;
        const auto buf = t.toMpv();
        node.u.string = new char[buf.size() + 1];
        qstrncpy(node.u.string, buf.data(), buf.size() + 1);
    }
    static auto node_free(mpv_node &node) -> void { delete[] node.u.string; }
};

template<>
struct mpv_trait<QVariant> {
    using mpv_type = mpv_node;
    static constexpr mpv_format format = MPV_FORMAT_NODE;
    static auto get(QVariant &var, const mpv_type &data) { var = parse(data); }
    static auto get_free(mpv_type &data) { mpv_free_node_contents(&data); }
    static QVariant parse(const mpv_node &node) {
        switch (node.format) {
        case MPV_FORMAT_DOUBLE:
            return node.u.double_;
        case MPV_FORMAT_FLAG:
            return !!node.u.flag;
        case MPV_FORMAT_INT64:
            return QVariant::fromValue<int>(node.u.int64);
        case MPV_FORMAT_STRING:
            return QString::fromUtf8(node.u.string);
        case MPV_FORMAT_NODE_ARRAY: {
            auto array = node.u.list;
            QVariantList list; list.reserve(array->num);
            for (int i=0; i<array->num; ++i)
                list.append(parse(array->values[i]));
            return list;
        } case MPV_FORMAT_NODE_MAP: {
            auto list = node.u.list; QVariantMap map;
            for (int i=0; i<list->num; ++i) {
                auto data = parse(list->values[i]);
                map.insert(_L(list->keys[i]), data);
            }
            return map;
        } default:
            return QVariant();
        }
    }
};

SIA operator<<(QDebug dbg, const mpv_node &node) -> QDebug
{
    dbg.nospace() << "mpv_node->" << mpv_trait<QVariant>::parse(node);;
    return dbg.space();
}

template<class T>
using mpv_t = typename mpv_trait<T>::mpv_type;

template<class T>
struct MpvScopedData {
    using type = typename mpv_trait<T>::mpv_type;
    MpvScopedData(const MpvScopedData&) = delete;
    ~MpvScopedData() { m_free(m_data); }
    auto operator = (const MpvScopedData&) = delete;
    auto raw() -> type* { return &m_data; }
    auto format() const { return mpv_trait<T>::format; }
    auto set(const T &t) -> void { mpv_trait<T>::set(m_data, t); }
protected:
    MpvScopedData(void(*free)(type&)): m_data(), m_free(free) { }
    type m_data;
private:
    void(*m_free)(type&) = nullptr;
};

template<class T>
struct MpvGetScopedData : MpvScopedData<T> {
    MpvGetScopedData(): MpvScopedData<T>(mpv_trait<T>::get_free) { }
    auto get(T &t) const -> void { mpv_trait<T>::get(t, this->m_data); }
};

template<class T>
struct MpvSetScopedData : MpvScopedData<T> {
    static_assert(!std::is_same<T, QString>::value, "!!!");
    MpvSetScopedData(const T &t): MpvScopedData<T>(mpv_trait<T>::set_free)
        { mpv_trait<T>::set(this->m_data, t); }
};

#endif // MPV_PROPERTY_HPP

