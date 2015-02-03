#include "pref_helper.hpp"
#include "pref.hpp"

static QVector<const PrefFieldInfo*> s_list;

PrefFieldInfo::PrefFieldInfo(QObject *p, const char *property, const char *editor, const char *editorProperty)
    : m_propertyName(property), m_editor(editor), m_editorProperty(editorProperty)
{
    auto mo = p->metaObject();
    const auto idx = mo->indexOfProperty(property);
    Q_ASSERT(idx != -1);
    m_property = mo->property(idx);
    Q_ASSERT(!s_list.contains(this));
    s_list.push_back(this);
}

PrefFieldInfo::PrefFieldInfo(QObject *p, const char *property, const char *editorProperty)
    : PrefFieldInfo(p, property, property, editorProperty)
{

}

auto PrefFieldInfo::getList() -> const QVector<const PrefFieldInfo*>&
{
    return s_list;
}

auto PrefFieldInfo::setFromEditor(QObject *p, const QObject *e) const -> void
{
    Q_ASSERT(e->objectName() == _L(m_editor));
    if (!e->property(m_editorProperty).isValid())
        qDebug() << "setFromEditor" << e->objectName() << m_editorProperty;
    m_property.write(p, e->property(m_editorProperty));
}
auto PrefFieldInfo::setToEditor(const QObject *p, QObject *e) const -> void
{
    Q_ASSERT(e->objectName() == _L(m_editor));
    if (!e->property(m_editorProperty).isValid())
        qDebug() << "setToEditor" << e->objectName() << m_editorProperty;
    e->setProperty(m_editorProperty, m_property.read(p));
}
