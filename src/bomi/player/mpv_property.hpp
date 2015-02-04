#ifndef MPV_PROPERTY_HPP
#define MPV_PROPERTY_HPP

#include <libmpv/client.h>
#include "tmp/type_test.hpp"

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
    static auto get_free(mpv_type) { };
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

template<>
struct mpv_trait<const char*> : detail::mpv_trait_no_alloc {
    using mpv_type = const char*;
    static constexpr mpv_format format = MPV_FORMAT_STRING;
    static auto set(const char *&data, const char *t) { data = t; }
    static auto node_fill(mpv_node &node, const char *t)
    { node.format = format; set((const char*&)node.u.string, t); }
};

template<>
struct mpv_trait<QString> {
    using mpv_type = const char*;
    static constexpr mpv_format format = MPV_FORMAT_STRING;
    static auto get_free(mpv_type &data) { mpv_free((void*)data); }
    static auto get(QString &s, const mpv_type &data)
        { s = QString::fromLocal8Bit(data); }
    static auto set(mpv_type &data, const QString &t)
    {
        const auto buf = t.toLocal8Bit();
        auto str = new char[buf.size() + 1];
        qstrncpy(str, buf.data(), buf.size() + 1);
        data = str;
    }
    static auto set_free(mpv_type &str) { delete[]str; }
private:
    friend class mpv_trait<QStringList>;
    static auto node_fill(mpv_node &node, const QString &t)
    { node.format = format; set((const char*&)node.u.string, t); }
    static auto node_free(mpv_node &node) { set_free((const char*&)node.u.string); }
};

template<>
struct mpv_trait<QStringList> {
    using ST = mpv_trait<QString>;
    using mpv_type = mpv_node;
    SCA format = MPV_FORMAT_NODE;
    static auto get(QStringList &t, const mpv_node &data) {
        if (data.format != MPV_FORMAT_NODE_ARRAY)
            return;
        t.clear();
        auto array = data.u.list;
        t.reserve(array->num);
        for (int i = 0; i < array->num; ++i) {
            if (array->values[i].format == MPV_FORMAT_STRING)
                t.push_back(toString(array->values[i]));
        }
    }
    static auto get_free(mpv_node &data) { mpv_free_node_contents(&data); }
    static auto set(mpv_node &data, const QStringList &t) -> void
    {
        data.format = MPV_FORMAT_NODE_ARRAY;
        data.u.list = new mpv_node_list;
        auto list = data.u.list;
        list->num = t.size();
        list->keys = nullptr;
        list->values = new mpv_node[t.size()];
        for (int i = 0; i < t.size(); ++i)
            mpv_trait<QString>::node_fill(list->values[i], t[i]);
    }
    static auto set_free(mpv_node &data) -> void
    {
        for (int i = 0; i < data.u.list->num; ++i)
            mpv_trait<QString>::node_free(data.u.list->values[i]);
        delete [] data.u.list->values;
        delete data.u.list;
    }
private:
    static auto toString(const mpv_node &node) -> QString
        { QString t; ST::get(t, node.u.string); return t; }
};

template<>
struct mpv_trait<QVariant> {
    using mpv_type = mpv_node;
    static constexpr mpv_format format = MPV_FORMAT_NODE;
    static constexpr bool use_free = true;
    static auto get(QVariant &var, const mpv_type &data) { var = parse(data); }
    static auto get_free(mpv_type &data) { mpv_free_node_contents(&data); }
private:
    static QVariant parse(const mpv_node &node) {
        switch (node.format) {
        case MPV_FORMAT_DOUBLE:
            return node.u.double_;
        case MPV_FORMAT_FLAG:
            return !!node.u.flag;
        case MPV_FORMAT_INT64:
            return QVariant::fromValue<int>(node.u.int64);
        case MPV_FORMAT_STRING:
            return QString::fromLocal8Bit(node.u.string);
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
                map.insert(QString::fromLocal8Bit(list->keys[i]), data);
            }
            return map;
        } default:
            return QVariant();
        }
    }
};

template<class T>
using mpv_t = typename mpv_trait<T>::mpv_type;

#endif // MPV_PROPERTY_HPP

