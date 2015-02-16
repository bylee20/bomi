#include "objectstorage.hpp"
#include "misc/log.hpp"

DECLARE_LOG_CONTEXT(ObjectStorage)

struct Alias {
    QByteArray name;
    QObject *object;
    QMetaProperty property;
};

struct RawData {
    QByteArray name;
    std::function<QVariant(void)> get;
    std::function<void(const QVariant &)> set;
};

struct ObjectStorage::Data {
    QObject *object = nullptr;
    QString name;
    QList<QByteArray> properties;
    QList<Alias> aliases;
    QList<RawData> data;
    bool autosave = false;
};

ObjectStorage::ObjectStorage(QObject *parent)
    : QObject(parent), d(new Data)
{
}

ObjectStorage::~ObjectStorage()
{
    if (d->autosave)
        save();
    delete d;
}

auto ObjectStorage::setAutoSave(bool save) -> void
{
    d->autosave = save;
}

auto ObjectStorage::setObject(QObject *object, const QString &name, bool autosave) -> bool {
    if (d->object && d->object != object) {
        _Error("Cannot change target object.");
        return false;
    }
    if (name.isEmpty()) {
        _Error("Cannot set empty name.");
        return false;
    }
    d->object = object;
    d->name = name;
    d->autosave = autosave;
    return true;
}

auto ObjectStorage::save() -> void
{
    if (!d->object)
        return;
    QSettings s;
    s.beginGroup(u"object-states"_q);
    s.beginGroup(d->name);

    if (d->object->isWidgetType())
        s.setValue(u"_b_geometry"_q, static_cast<QWidget*>(d->object)->saveGeometry());
    for (auto &name : d->properties)
        s.setValue(_L(name), d->object->property(name));
    for (auto &alias : d->aliases)
        s.setValue(_L(alias.name), alias.property.read(alias.object));
    for (auto &data : d->data)
        s.setValue(_L(data.name), data.get());

    s.endGroup();
    s.endGroup();

}

auto ObjectStorage::restore() -> void
{
    if (!d->object)
        return;
    QSettings s;
    s.beginGroup(u"object-states"_q);
    s.beginGroup(d->name);

    if (d->object->isWidgetType())
        static_cast<QWidget*>(d->object)->restoreGeometry(s.value(u"_b_geometry"_q).toByteArray());
    for (auto &name : d->properties)
        d->object->setProperty(name, s.value(_L(name), d->object->property(name)));
    for (auto &alias : d->aliases)
        alias.property.write(alias.object, s.value(_L(alias.name), alias.property.read(alias.object)));
    for (auto &data : d->data)
        data.set(s.value(_L(data.name), data.get()));

    s.endGroup();
    s.endGroup();
}

auto ObjectStorage::add(const char *name) -> void
{
    d->properties.push_back(name);
}

auto ObjectStorage::add(const char *alias, QObject *from, const char *src) -> bool
{
    auto mo = from->metaObject();
    const int idx = mo->indexOfProperty(src);
    if (idx < 0) {
        _Error("Property '%%' does not exist in '%%'.", src, from->metaObject()->className());
        return false;
    }
    d->aliases.push_back({alias, from, mo->property(idx)});
    return true;
}

auto ObjectStorage::add(const char *name, std::function<QVariant(void)> &&get,
                        std::function<void(const QVariant &var)> &&set) -> void
{
    d->data.push_back({name, std::move(get), std::move(set)});
}
