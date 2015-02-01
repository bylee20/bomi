#ifndef MPV_PROPERTY_HPP
#define MPV_PROPERTY_HPP

#include <libmpv/client.h>
#include "tmp/type_test.hpp"

struct MpvAsyncData {
    ~MpvAsyncData() { free(data); }
    QByteArray name;
    void *data = nullptr;
    void *data_ptr = nullptr;
    void(*free)(void*) = nullptr;
};

namespace detail {
enum MpvTraitType {
    Arithmetic,
    String,
    Extra
};

template<class T>
static constexpr inline auto mpv_trait_type() -> MpvTraitType
{
    return tmp::is_arithmetic<T>() ? Arithmetic :
           tmp::is_one_of<T, QString, QByteArray>() ? String : Extra;
}

template<class T>
struct SimpleAsync {
    template<class mpv_type>
    static auto async(T t) -> MpvAsyncData* {
        auto data = new MpvAsyncData;
        data->data_ptr = data->data = new mpv_type(t);
        data->free = [] (void *data) { delete (mpv_type*)(data); };
        return data;
    }
};

SIA dupToRaw(const QByteArray &t) -> char*
{
    auto str = new char[t.size() + 1];
    memcpy(str, t.data(), t.size());
    str[t.size()] = '\0';
    return str;
}

SIA async(const QByteArray &t) -> MpvAsyncData*
{
    auto data = new MpvAsyncData;
    data->data = dupToRaw(t);
    data->data_ptr = &(char*&)data->data;
    data->free = [] (void *data) { delete[] (char*)data; };
    return data;
}

SIA async(const QString &t) -> MpvAsyncData*
{ return async(t.toLocal8Bit()); }

}

template<class T, detail::MpvTraitType type = detail::mpv_trait_type<T>()>
struct mpv_format_trait { };

template<class T>
struct mpv_format_trait<T, detail::Arithmetic> {
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
    static auto set_free(mpv_type&) { }
    static auto set_async(T t) -> MpvAsyncData*
    { return detail::SimpleAsync<T>::template async<mpv_type>(t); }
};

template<class T>
struct mpv_format_trait<T, detail::String> {
    using mpv_type = const char*;
    static constexpr mpv_format format = MPV_FORMAT_STRING;
    static auto get_free(mpv_type &data) { mpv_free((void*)data); }
    static auto get(QString &s, const mpv_type &data)
        { s = QString::fromLocal8Bit(data); }
    static auto get(QByteArray &a, const mpv_type &data)
        { a = QByteArray(data); }
    static auto set(mpv_type &data, const QByteArray &t) { data = t.constData(); }
    static auto set_free(mpv_type&) { }
    static auto set_async(const T &t) -> MpvAsyncData* { return detail::async(t); }
};

template<>
struct mpv_format_trait<QStringList> {
    using ST = mpv_format_trait<QString>;
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
            fillNode(list->values[i], t[i]);
    }
    static auto set_async(const QStringList &t) -> MpvAsyncData*
    {
        auto data = new MpvAsyncData;
        auto node = new mpv_node;
        set(*node, t);
        data->data_ptr = data->data = node;
        data->free = [] (void *data) {
            auto node = static_cast<mpv_node*>(data);
            set_free(*node);
            delete node;
        };
        return data;
    }
    static auto set_free(mpv_node &data) -> void
    {
        for (int i = 0; i < data.u.list->num; ++i)
            freeStringNode(data.u.list->values[i]);
        delete [] data.u.list->values;
        delete data.u.list;
    }
private:
    static auto toString(const mpv_node &node) -> QString
        { QString t; ST::get(t, node.u.string); return t; }
    static auto fillNode(mpv_node &node, const QByteArray &data) -> void
        { node.format = MPV_FORMAT_STRING; node.u.string = detail::dupToRaw(data); }
    static auto fillNode(mpv_node &node, const QString &data) -> void
        { fillNode(node, data.toLocal8Bit()); }
    static auto freeStringNode(mpv_node &node) -> void
        { delete []node.u.string; }
};

template<>
struct mpv_format_trait<QVariant> {
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
using mpv_t = typename mpv_format_trait<T>::mpv_type;

#endif // MPV_PROPERTY_HPP

