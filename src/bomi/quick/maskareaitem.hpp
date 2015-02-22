#ifndef ICONBUTTONITEM_HPP
#define ICONBUTTONITEM_HPP

#include "simpletextureitem.hpp"

class MouseEventObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal x READ x)
    Q_PROPERTY(qreal y READ y)
    Q_PROPERTY(bool accepted READ isAccepted WRITE setAccepted)
    Q_PROPERTY(int button READ button)
    Q_PROPERTY(Qt::MouseButtons buttons READ buttons)
    Q_PROPERTY(Qt::KeyboardModifiers modifiers READ modifiers)
public:
    MouseEventObject(QMouseEvent *event): m_mouse(event) { }
    auto x() const -> qreal { return m_mouse->localPos().x(); }
    auto y() const -> qreal { return m_mouse->localPos().y(); }
    auto isAccepted() const -> bool { return m_mouse->isAccepted(); }
    auto setAccepted(bool a) -> void { m_mouse->setAccepted(a); }
    auto button() const -> Qt::MouseButton { return m_mouse->button(); }
    auto buttons() const -> Qt::MouseButtons { return m_mouse->buttons(); }
    auto modifiers() const -> Qt::KeyboardModifiers { return m_mouse->modifiers(); }
private:
    QMouseEvent *m_mouse = nullptr;
};

class MaskAreaItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QUrl source READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(QUrl mask READ source WRITE setSource NOTIFY sourceChanged)
    Q_PROPERTY(qreal alpha READ alpha WRITE setAlpha NOTIFY maskAlphaChanged)
    Q_PROPERTY(bool pressed READ isPressed NOTIFY pressedChanged)
    Q_PROPERTY(bool hovered READ isHovered NOTIFY hoveredChanged)
    Q_PROPERTY(bool containsMouse READ isHovered NOTIFY hoveredChanged)
    Q_PROPERTY(Qt::MouseButtons acceptedButtons READ acceptedMouseButtons WRITE setAcceptedMouseButtons)
    Q_PROPERTY(bool hoverEnabled READ acceptHoverEvents WRITE setAcceptHoverEvents)
    Q_PROPERTY(qreal mouseX READ mouseX NOTIFY mouseXChanged)
    Q_PROPERTY(qreal mouseY READ mouseY NOTIFY mouseYChanged)
public:
    MaskAreaItem(QQuickItem *parent = nullptr);
    ~MaskAreaItem();
    auto source() const -> QUrl;
    auto setSource(const QUrl &url) -> void;
    auto alpha() const -> qreal;
    auto setAlpha(qreal alpha) -> void;
    auto isPressed() const -> bool;
    auto isHovered() const -> bool;
    auto mouseX() const -> qreal;
    auto mouseY() const -> qreal;
    Q_INVOKABLE auto contains(const QPointF &p) const -> bool { return contains(p.x(), p.y()); }
    Q_INVOKABLE auto contains(qreal x, qreal y) const -> bool;
signals:
    void pressed(MouseEventObject *mouse);
    void released(MouseEventObject *mouse);
    void sourceChanged();
    void exited();
    void entered();
    void canceled();
    void clicked(MouseEventObject *mouse);
    void doubleClicked();
    void maskAlphaChanged();
    void pressedChanged(bool pressed);
    void hoveredChanged(bool hovered);
    void mouseXChanged();
    void mouseYChanged();
private:
    auto mouseUngrabEvent() -> void override;
    auto mousePressEvent(QMouseEvent *event) -> void override;
    auto mouseReleaseEvent(QMouseEvent *event) -> void override;
    auto mouseMoveEvent(QMouseEvent *event) -> void override;
    auto mouseDoubleClickEvent(QMouseEvent *event) -> void override;
    auto hoverEnterEvent(QHoverEvent *event) -> void override;
    auto hoverLeaveEvent(QHoverEvent *event) -> void override;
    auto hoverMoveEvent(QHoverEvent *event) -> void override;
    struct Data;
    Data *d;
};

#endif // ICONBUTTONITEM_HPP
