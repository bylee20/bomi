#ifndef UNDOSTACK_HPP
#define UNDOSTACK_HPP

#include "stdafx.hpp"
#include "actiongroup.hpp"

class UndoStack : public QObject {
    Q_OBJECT
public:
    template<typename Func, typename T>
    class Command : public QUndoCommand {
    public:
        Command(const T &to, const T &from, const Func &func): to(to), from(from), func(func) { }
        void redo() { func(to); }
        void undo() { func(from); }
    private:
        T to, from; Func  func;
    };
    UndoStack() {
        QObject::connect(&m_stack, &QUndoStack::canUndoChanged, this, &UndoStack::canUndoChanged);
        QObject::connect(&m_stack, &QUndoStack::canRedoChanged, this, &UndoStack::canRedoChanged);
    }
    template<typename T, typename Func>
    QUndoCommand *push(const T &to, const T &from, const Func &func) {
        if (m_enabled) {
            auto cmd = new Command<Func, T>(to, from, func);
            m_stack.push(cmd);
            return cmd;
        } else {
            func(to);
            return nullptr;
        }
    }
    template<typename Func>
    void connect(ActionGroup *g, Func func) {
        QObject::connect(g, &ActionGroup::triggered, [this, g, func] (QAction *a) {
            push(a, m_currentActions[g], [this, func, g] (QAction *a) {
                if (a) {
                    func(a);
                    a->setChecked(true);
                }
                m_currentActions[g] = a;
            });
        });
    }
    void registerActionGroup(ActionGroup *g) { m_currentActions[g] = g->checkedAction(); }
    bool canUndo() const { return m_stack.canUndo(); }
    bool canRedo() const { return m_stack.canRedo(); }
    void setEnabled(bool enabled) { m_enabled = enabled; }
public slots:
    void undo() { m_stack.undo(); }
    void redo() { m_stack.redo(); }
signals:
    void canUndoChanged(bool can);
    void canRedoChanged(bool can);
private:
    QUndoStack m_stack;
    QMap<ActionGroup*, QAction*> m_currentActions;
    bool m_enabled = false;
};

#endif // UNDOSTACK_HPP
