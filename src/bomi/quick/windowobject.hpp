#ifndef WINDOWOBJECT_HPP
#define WINDOWOBJECT_HPP

#include <QQuickItem>

class MainWindow;

class MouseObject : public QObject {
    Q_OBJECT
    Q_ENUMS(Cursor)
    Q_PROPERTY(QPointF pos READ pos)
    Q_PROPERTY(Cursor cursor READ cursor NOTIFY cursorChanged)
    Q_PROPERTY(bool hidingCursorBlocked READ isHidingCursorBlocked WRITE setHidingCursorBlocked NOTIFY hidingCursorBlockedChanged)
public:
    enum Cursor { NoCursor, Arrow };
    auto pos() const -> QPointF { return QCursor::pos(); }
    auto cursor() const -> Cursor { return m_cursor; }
    auto updateCursor(Qt::CursorShape shape) -> void;
    auto isHidingCursorBlocked() const -> bool { return m_blocked; }
    auto setHidingCursorBlocked(bool b) -> void
        { if (_Change(m_blocked, b)) emit hidingCursorBlockedChanged(b); }
    Q_INVOKABLE bool isIn(QQuickItem *item);
    Q_INVOKABLE bool isIn(QQuickItem *item, const QRectF &exclude);
    Q_INVOKABLE QPointF posFor(QQuickItem *item);
signals:
    void cursorChanged();
    void hidingCursorBlockedChanged(bool blocked);
private:
    Cursor m_cursor = Arrow;
    bool m_blocked = false;
};


class WindowObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool fullscreen READ fullscreen NOTIFY fullscreenChanged)
    Q_PROPERTY(MouseObject *mouse READ mouse CONSTANT FINAL)
    Q_PROPERTY(QSize size READ size NOTIFY sizeChanged)
    Q_PROPERTY(int width READ width NOTIFY widthChanged)
    Q_PROPERTY(int height READ height NOTIFY heightChanged)
    Q_PROPERTY(QQuickItem *z10 READ z10 CONSTANT FINAL)
public:
    auto set(MainWindow *mw) -> void;
    auto fullscreen() const -> bool;
    auto mouse() const -> MouseObject* { return getMouse(); }
    auto size() const -> QSize;
    auto width() const -> int;
    auto height() const -> int;
    auto z10() -> QQuickItem* { return &m_z10; }
    static auto getMouse() -> MouseObject*;
    Q_INVOKABLE void showToolTip(QQuickItem *item, const QPointF &pos,
                                 const QString &text);
    Q_INVOKABLE void showToolTip(QQuickItem *item, qreal x, qreal y,
                                 const QString &text)
        { showToolTip(item, {x, y}, text); }
    Q_INVOKABLE void hideToolTip();
signals:
    void fullscreenChanged();
    void sizeChanged();
    void widthChanged();
    void heightChanged();
private:
    MainWindow *m = nullptr;
    QQuickItem m_z10;
};

#endif // WINDOWOBJECT_HPP
