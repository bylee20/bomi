#include "toplevelitem.hpp"
#include <QQuickWindow>

struct TopLevelItem::Data {
    bool pressed = false;
};

TopLevelItem::TopLevelItem(QQuickItem *parent)
    : SimpleVertexItem(parent)
    , d(new Data)
{
    setZ(1e10);
    setAcceptedMouseButtons(Qt::AllButtons);
    setColor(QColor(0, 0, 0, 255*0.4));

    connect(this, &QQuickItem::windowChanged, this, [this] (QQuickWindow *w) {
        if (w) {
            connect(w, &QQuickWindow::widthChanged,
                    this, &QQuickItem::setWidth);
            connect(w, &QQuickWindow::heightChanged,
                    this, &QQuickItem::setHeight);
            setWidth(w->width());
            setHeight(w->height());
        }
    });
}

TopLevelItem::~TopLevelItem() {
    delete d;
}

auto TopLevelItem::updateVertex(Vertex *vertex) -> void
{
    Vertex::fillAsTriangleStrip(vertex, {0, 0}, {width(), height()});
}

auto TopLevelItem::filteredMousePressEvent() const -> bool
{
    return d->pressed;
}

auto TopLevelItem::resetMousePressEventFilterState() -> void
{
    d->pressed = false;
}

auto TopLevelItem::mousePressEvent(QMouseEvent *event) -> void
{
    QQuickItem::mousePressEvent(event);
    event->accept();
    d->pressed = true;
}

auto TopLevelItem::itemChange(ItemChange change,
                              const ItemChangeData &data) -> void
{
    QQuickItem::itemChange(change, data);
    switch (change) {
    case ItemChildAddedChange:
        connect(data.item, &QQuickItem::visibleChanged,
                this, &TopLevelItem::check);
        check();
        break;
    case ItemChildRemovedChange:
        disconnect(data.item, &QQuickItem::visibleChanged,
                   this, &TopLevelItem::check);
        check();
        break;
    default:
        break;
    }
}

auto TopLevelItem::check() -> void
{
    const auto items = childItems();
    for (auto item : items) {
        if (item->isVisible()) {
            setVisible(true);
            return;
        }
    }
    setVisible(false);
}
