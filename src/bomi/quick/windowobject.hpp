#ifndef WINDOWOBJECT_HPP
#define WINDOWOBJECT_HPP

#include <QQuickItem>

class MainWindow;

class MouseObject : public QObject {
    Q_OBJECT
    Q_ENUMS(Cursor)
    Q_PROPERTY(QPointF pos READ pos)
    Q_PROPERTY(Cursor cursor READ cursor NOTIFY cursorChanged)
public:
    enum Cursor { NoCursor, Arrow };
    auto pos() const -> QPointF { return QCursor::pos(); }
    auto cursor() const -> Cursor { return m_cursor; }
    auto updateCursor(Qt::CursorShape shape) -> void;
    Q_INVOKABLE bool isIn(QQuickItem *item);
    Q_INVOKABLE QPointF posFor(QQuickItem *item);
signals:
    void cursorChanged();
private:
    Cursor m_cursor = Arrow;
};


class WindowObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool fullscreen READ fullscreen NOTIFY fullscreenChanged)
    Q_PROPERTY(MouseObject *mouse READ mouse CONSTANT FINAL)
public:
    auto set(MainWindow *mw) -> void;
    auto fullscreen() const -> bool;
    auto mouse() const -> MouseObject* { return &m_mouse; }
    Q_INVOKABLE void showToolTip(QQuickItem *item, const QPointF &pos,
                                 const QString &text);
    Q_INVOKABLE void showToolTip(QQuickItem *item, qreal x, qreal y,
                                 const QString &text)
        { showToolTip(item, {x, y}, text); }
    Q_INVOKABLE void hideToolTip();
signals:
    void fullscreenChanged();
private:
    MainWindow *m = nullptr;
    mutable MouseObject m_mouse;
};

#endif // WINDOWOBJECT_HPP
