#include "undostack.hpp"

UndoStack::UndoStack() {
    QObject::connect(&m_stack, &QUndoStack::canUndoChanged,
                     this, &UndoStack::canUndoChanged);
    QObject::connect(&m_stack, &QUndoStack::canRedoChanged,
                     this, &UndoStack::canRedoChanged);
}
