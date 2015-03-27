#include "pref_helper.hpp"
#include "pref.hpp"

//static QVector<const PrefFieldInfo*> s_list;

//PrefFieldInfo::PrefFieldInfo(QObject *p, const char *property, const char *editor)
//    : m_editor(editor)
//{
//    auto mo = p->metaObject();
//    const auto idx = mo->indexOfProperty(property);
//    Q_ASSERT(idx != -1);
//    m_property = mo->property(idx);
//    Q_ASSERT(!s_list.contains(this));
//    s_list.push_back(this);
//}

//auto PrefFieldInfo::getList() -> const QVector<const PrefFieldInfo*>&
//{
//    return s_list;
//}

//auto PrefFieldInfo::setFromEditor(QObject *p, const QObject *e) const -> void
//{
//    Q_ASSERT(e->objectName() == _L(m_property.name()));
//    const auto var = e->property(m_editor);
//    if (!var.isValid())
//        qDebug() << "setFromEditor" << e->objectName() << m_editor;
//    m_property.write(p, var);
//}
//auto PrefFieldInfo::setToEditor(const QObject *p, QObject *e) const -> void
//{
//    Q_ASSERT(e->objectName() == _L(m_property.name()));
//    if (!e->property(m_editor).isValid())
//        qDebug() << "setToEditor" << e->objectName() << m_editor;
//    e->setProperty(m_editor, m_property.read(p));
//}
