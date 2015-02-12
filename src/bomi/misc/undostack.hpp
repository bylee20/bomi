#ifndef UNDOSTACK_HPP
#define UNDOSTACK_HPP

#include <QUndoStack>
#include <QActionGroup>

class UndoStack : public QObject {
    Q_OBJECT
public:
    template<class Func, class T>
    class Command : public QUndoCommand {
    public:
        Command(const T &to, const T &from, const Func &func)
            : to(to), from(from), func(func) { }
        auto redo() -> void { func(to); }
        auto undo() -> void { func(from); }
    private:
        T to, from; Func  func;
    };
    UndoStack();
    template<class T, class Func>
    auto push(const T &to, const T &from, const Func &func) -> QUndoCommand*;
    template<class Func>
    auto connect(QActionGroup *g, Func func) -> void;
    auto registerActionGroup(QActionGroup *g) -> void;
    auto canUndo() const -> bool { return m_stack.canUndo(); }
    auto canRedo() const -> bool { return m_stack.canRedo(); }
    auto setEnabled(bool enabled) -> void { m_enabled = enabled; }
    auto undo() -> void { m_stack.undo(); }
    auto redo() -> void { m_stack.redo(); }
signals:
    void canUndoChanged(bool can);
    void canRedoChanged(bool can);
private:
    QUndoStack m_stack;
    QMap<QActionGroup*, QAction*> m_currentActions;
    bool m_enabled = false;
};

template<class T, class Func>
inline auto UndoStack::push(const T &to, const T &from,
                            const Func &func) -> QUndoCommand*
{
    if (m_enabled) {
        auto cmd = new Command<Func, T>(to, from, func);
        m_stack.push(cmd);
        return cmd;
    } else {
        func(to);
        return nullptr;
    }
}

template<class Func>
inline auto UndoStack::connect(QActionGroup *g, Func func) -> void
{
    QObject::connect(g, &QActionGroup::triggered, [this, g, func] (QAction *a) {
        push(a, m_currentActions[g], [this, func, g] (QAction *a) {
            if (a) {
                func(a);
                a->setChecked(true);
            }
            m_currentActions[g] = a;
        });
    });
}

inline auto UndoStack::registerActionGroup(QActionGroup *group) -> void
    { m_currentActions[group] = group->checkedAction(); }

#endif // UNDOSTACK_HPP
