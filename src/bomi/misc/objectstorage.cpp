#include "objectstorage.hpp"
#include "misc/log.hpp"
#include <QMetaProperty>
#include <QSettings>

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
    QSettings *s = nullptr;
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
    else
        close();
    delete d;
}

auto ObjectStorage::setAutoSave(bool save) -> void
{
    d->autosave = save;
}

auto ObjectStorage::object() const -> QObject*
{
    return d->object;
}

auto ObjectStorage::setObject(QObject *object, const QString &name) -> bool {
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
    return true;
}

auto ObjectStorage::open() -> void
{
    if (!d->s) {
        d->s = new QSettings(file(), QSettings::IniFormat);
        d->s->beginGroup(d->name);
    }
}

auto ObjectStorage::close() -> void
{
    if (d->s)
        d->s->endGroup();
    _Delete(d->s);
}

auto ObjectStorage::write(const char *name, const QVariant &var) -> void
{
    Q_ASSERT(d->s);
    if (d->s)
        d->s->setValue(_L(name), var);
}

auto ObjectStorage::read(const char *name, const QVariant &def) const -> QVariant
{
    Q_ASSERT(d->s);
    if (d->s)
        return d->s->value(_L(name), def);
    return QVariant();
}

auto ObjectStorage::save() -> void
{
    if (!d->object)
        return;
    const bool was = d->s;
    if (!was)
        open();

    if (d->object->isWidgetType()) {
        auto w = static_cast<const QWidget*>(d->object);
        if (!w->parentWidget())
            d->s->setValue(u"_b_geometry"_q, w->saveGeometry());
    }
    for (auto &name : d->properties)
        d->s->setValue(_L(name), d->object->property(name));
    for (auto &alias : d->aliases)
        d->s->setValue(_L(alias.name), alias.property.read(alias.object));
    for (auto &data : d->data)
        d->s->setValue(_L(data.name), data.get());

    if (!was)
        close();
}

auto ObjectStorage::add(const char *property) -> void
{
    d->properties.push_back(property);
}

auto ObjectStorage::restore() -> void
{
    if (!d->object)
        return;
    const bool was = d->s;
    if (!was)
        open();

    if (d->object->isWidgetType()) {
        auto w = static_cast<QWidget*>(d->object);
        if (!w->parentWidget()) {
            auto g = d->s->value(u"_b_geometry"_q).toByteArray();
            if (!g.isEmpty())
                w->restoreGeometry(g);
        }
    }
    for (auto &name : d->properties)
        d->object->setProperty(name, d->s->value(_L(name), d->object->property(name)));
    for (auto &alias : d->aliases)
        alias.property.write(alias.object, d->s->value(_L(alias.name), alias.property.read(alias.object)));
    for (auto &data : d->data)
        data.set(d->s->value(_L(data.name), data.get()));

    if (!was)
        close();
}

auto ObjectStorage::add(QByteArray &&alias, QObject *from, const char *src) -> bool
{
    if (alias.isEmpty()) {
        _Error("Empty alias name.");
        return false;
    }
    auto mo = from->metaObject();
    const int idx = mo->indexOfProperty(src);
    if (idx < 0) {
        _Error("Property '%%' does not exist in '%%'.", src, from->metaObject()->className());
        return false;
    }
    d->aliases.push_back({std::move(alias), from, mo->property(idx)});
    return true;
}

auto ObjectStorage::add(const char *name, std::function<QVariant(void)> &&get,
                        std::function<void(const QVariant &var)> &&set) -> void
{
    d->data.push_back({name, std::move(get), std::move(set)});
}

auto ObjectStorage::add(QLineEdit *le) -> bool
{
    return add(le->objectName().toLatin1(), le, "text");
}

auto ObjectStorage::add(QAbstractButton *cb) -> bool
{
    return add(cb->objectName().toLatin1(), cb, "checked");
}

auto ObjectStorage::file() const -> QString
{
    return _WritablePath(Location::Config) % "/objectstorage.ini"_a;
}

auto ObjectStorage::add(QSpinBox *sb) -> bool
{
    return add(sb->objectName().toLatin1(), sb, "value");
}

auto ObjectStorage::add(QDoubleSpinBox *sb) -> bool
{
    return add(sb->objectName().toLatin1(), sb, "value");
}
