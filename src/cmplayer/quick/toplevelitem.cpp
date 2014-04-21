#include "toplevelitem.hpp"

void reg_top_level_item() {
    qmlRegisterType<TopLevelItem>();
}

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
            connect(w, &QQuickWindow::widthChanged, this, &QQuickItem::setWidth);
            connect(w, &QQuickWindow::heightChanged, this, &QQuickItem::setHeight);
            setWidth(w->width());
            setHeight(w->height());
        }
    });
}

TopLevelItem::~TopLevelItem() {
    delete d;
}

void TopLevelItem::updateVertex(Vertex *vertex) {
    Vertex::fillAsTriangleStrip(vertex, {0, 0}, {width(), height()});
}

bool TopLevelItem::filteredMousePressEvent() const {
    return d->pressed;
}

void TopLevelItem::resetMousePressEventFilterState() {
    d->pressed = false;
}

void TopLevelItem::mousePressEvent(QMouseEvent *event) {
    QQuickItem::mousePressEvent(event);
    event->accept();
    d->pressed = true;
}

void TopLevelItem::itemChange(ItemChange change, const ItemChangeData &data) {
    QQuickItem::itemChange(change, data);
    switch (change) {
    case ItemChildAddedChange:
        connect(data.item, &QQuickItem::visibleChanged, this, &TopLevelItem::check);
        check();
        break;
    case ItemChildRemovedChange:
        disconnect(data.item, &QQuickItem::visibleChanged, this, nullptr);
        check();
        break;
    default:
        break;
    }
}

void TopLevelItem::check() {
    const auto items = childItems();
    for (auto item : items) {
        if (item->isVisible()) {
            setVisible(true);
            return;
        }
    }
    setVisible(false);
}
