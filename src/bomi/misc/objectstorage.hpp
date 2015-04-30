#ifndef OBJECTSTORAGE_HPP
#define OBJECTSTORAGE_HPP

#include "tmp/type_traits.hpp"
#include "enum/enums.hpp"

class ObjectStorage : public QObject {
public:
    ObjectStorage(QObject *parent = nullptr);
    ~ObjectStorage();
    auto setAutoSave(bool save) -> void;
    auto object() const -> QObject*;
    auto setObject(QObject *object, const QString &name) -> bool;
    auto save() -> void;
    auto restore() -> void;
    auto open() -> void;
    auto close() -> void;
    auto write(const char *name, const QVariant &var) -> void;
    auto read(const char *name, const QVariant &def) const -> QVariant;
    template<class T>
    auto write(const char *name, const T &t) -> void
        { write(name, QVariant::fromValue<T>(t)); }
    template<class T>
    auto read(const char *name, const T &def) const -> T
        { return read(name, QVariant::fromValue<T>(def)).template value<T>(); }
    auto add(const char *property) -> void;
    auto add(QByteArray &&alias, QObject *src, const char *property) -> bool;
    auto add(QAbstractButton *cb) -> bool;
    auto add(QSpinBox *sb) -> bool;
    auto add(QDoubleSpinBox *sb) -> bool;
    auto add(QLineEdit *le) -> bool;
    template<class T>
    auto add(const char *name, T *data) -> tmp::enable_unless_t<tmp::is_enum<T>()>
    {
        add(name, [=] () { return QVariant::fromValue<T>(*data); },
            [=] (auto &var) { *data = var.template value<T>(); });
    }
    template<class T>
    auto add(const char *name, T *data) -> tmp::enable_if_t<tmp::is_enum<T>()>
    {
        add(name, [=] () { return QVariant::fromValue(_EnumName(*data)); },
            [=] (auto &var) { *data = _EnumFrom<T>(var.toString()); });
    }
    auto add(const char *name, std::function<QVariant(void)> &&get,
             std::function<void(const QVariant &var)> &&set) -> void;
    template<class R, class T, class O, class R2, class S = tmp::remove_cref_t<R>>
    auto add(const char *name, R(O::*get)(), R2(O::*set)(T)) -> bool
    {
        static_assert(tmp::is_same<S, tmp::remove_cref_t<T>>(), "!!!");
        if (!checkType(&O::staticMetaObject.className()))
            return false;
        add(name, [=] () {
            auto o = static_cast<O*>(object());
            return QVariant::fromValue<S>((o->*get)());
        }, [=] (auto &var) {
            auto o = static_cast<O*>(object());
            (o->*set)(var.template value<S>());
        });
        return true;
    }
    template<class R, class T, class R2, class S = tmp::remove_cref_t<R>>
    auto add(const char *name, R(*get)(), R2(*set)(T)) -> bool
    {
        static_assert(tmp::is_same<S, tmp::remove_cref_t<T>>(), "!!!");
        add(name, [=] () { return QVariant::fromValue<S>(get()); },
            [=] (auto &var) { set(var.template value<S>()); });
        return true;
    }
    template<class T, class J = tmp::remove_cref_t<decltype(T().toJson())>>
    auto json(const char *name, T *data) -> tmp::enable_if_same_t<J, QJsonObject>
    {
        add(name, [=] () { return data->toJson().toVariantMap(); },
            [=] (auto &var) { data->setFromJson(QJsonObject::fromVariantMap(var.toMap())); });
    }
    template<class T, class J = tmp::remove_cref_t<decltype(T().toJson())>>
    auto json(const char *name, T *data) -> tmp::enable_if_same_t<J, QJsonArray>
    {
        add(name, [=] () { return data->toJson().toVariantList(); },
            [=] (auto &var) { data->setFromJson(QJsonArray::fromVariantList(var.toList())); });
    }
    template<class T, class J = tmp::remove_cref_t<decltype(T().toJson())>>
    auto json(const char *name, T *data) -> tmp::enable_if_same_t<J, QJsonValue>
    {
        add(name, [=] () { return data->toJson().toVariant(); },
            [=] (auto &var) { data->setFromJson(QJsonValue::fromVariant(var)); });
    }
    auto file() const -> QString;
private:
    auto checkType(const QMetaObject *mo) const -> bool;
    struct Data;
    Data *d;
};

#endif // OBJECTSTORAGE_HPP
