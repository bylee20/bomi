#ifndef OBJECTSTORAGE_HPP
#define OBJECTSTORAGE_HPP

class ObjectStorage : public QObject {
public:
    ObjectStorage(QObject *parent = nullptr);
    ~ObjectStorage();
    auto setAutoSave(bool save) -> void;
    auto setObject(QObject *object, const QString &name) -> bool;
    auto setObject(QObject *object, const QString &name, bool autosave) -> bool;
    auto save() -> void;
    auto restore() -> void;
    auto add(const char *property) -> void;
    auto add(const char *alias, QObject *src, const char *property) -> bool;
    template<class T>
    auto add(const char *name, T *data) -> void
    {
        add(name, [=] () { return QVariant::fromValue<T>(*data); },
            [=] (auto &var) { *data = var.template value<T>(); });
    }
    auto add(const char *name, std::function<QVariant(void)> &&get,
             std::function<void(const QVariant &var)> &&set) -> void;
private:
    struct Data;
    Data *d;
};
#endif // OBJECTSTORAGE_HPP
