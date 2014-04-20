#include "toplevelitem.hpp"

void reg_top_level_item() {
	qmlRegisterType<TopLevelItem>();
}

struct TopLevelItem::Data {
	QColor shade;
	bool shadeChanged = true, geometryChanged = true;
	bool pressed = false;
};

TopLevelItem::TopLevelItem(QQuickItem *parent)
: QQuickItem(parent), d(new Data) {
	setFlag(ItemHasContents);
	setZ(1e10);
	setVisible(true);
	setAcceptedMouseButtons(Qt::AllButtons);
	d->shade = QColor(0, 0, 0, 255*0.4);
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

QColor TopLevelItem::shade() const {
	return d->shade;
}

void TopLevelItem::setShade(const QColor &color) {
	if (_Change(d->shade, color)) {
		d->shadeChanged = true;
		emit shadeChanged();
		update();
	}
}

QSGNode *TopLevelItem::updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) {
	Q_UNUSED(data);
	auto node = static_cast<QSGGeometryNode*>(old);
	if (!old) {
		node = new QSGGeometryNode;
		auto g = new QSGGeometry(QSGGeometry::defaultAttributes_Point2D(), 4);
		g->setDrawingMode(GL_TRIANGLE_STRIP);
		auto m = new QSGFlatColorMaterial;
		m->setColor(d->shade);
		m->setFlag(QSGFlatColorMaterial::Blending);
		node->setGeometry(g);
		node->setMaterial(m);
		node->setFlags(QSGNode::OwnsMaterial | QSGNode::OwnsGeometry);
	}
	if (d->shadeChanged) {
		static_cast<QSGFlatColorMaterial*>(node->material())->setColor(d->shade);
		node->markDirty(QSGNode::DirtyMaterial);
		d->shadeChanged = false;
	}
	if (d->geometryChanged) {
		auto p = node->geometry()->vertexDataAsPoint2D();
		auto set = [&p] (float x, float y) { p->x = x; p->y = y; ++p; };
		set(0, 0);
		set(0, height());
		set(width(), 0);
		set(width(), height());
		node->markDirty(QSGNode::DirtyGeometry);
		d->geometryChanged = false;
	}
	return node;
}

void TopLevelItem::geometryChanged(const QRectF &new_, const QRectF &old) {
	QQuickItem::geometryChanged(new_, old);
	d->geometryChanged = true;
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
