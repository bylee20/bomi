#include "buttonboxitem.hpp"
#include "dialogs.hpp"

void reg_button_box_item() {
	qmlRegisterType<ButtonBoxItem>("CMPlayer", 1, 0, "ButtonBox");
}

static const char *code = R"(
import QtQuick 2.2

Rectangle {
	property alias text: textItem.text
	signal clicked
	Text {
		id: textItem
		anchors.fill: parent
		verticalAlignment: Text.AlignVCenter
		horizontalAlignment: Text.AlignHCenter
	}
}

)";

struct ButtonBoxItem::Data {
	ButtonBoxItem *p = nullptr;
	QList<QQuickItem*> items;
	QList<int> buttons;
	QQmlComponent *source = nullptr;
	QQmlComponent *def = nullptr;
	qreal bwidth = -1, gap = 5;
	QQuickItem *clicked = nullptr;

	QQuickItem *make(int button) {
		QQmlComponent *comp = source;
		if (!comp) {
			if (!def) {
				def = new QQmlComponent(qmlEngine(p), p);
				def->setData(::code, QUrl());
			}
			comp = def;
		}
		auto item = static_cast<QQuickItem*>(comp->create(qmlContext(p)));
		if (!item) {
			for (auto &error : comp->errors())
				qDebug() << error.toString();
			return nullptr;
		}
		auto mo = item->metaObject();
		auto clicked = mo->method(mo->indexOfSignal("clicked()"));
		if (!clicked.isValid()) {
			delete item;
			qDebug() << "No 'clicked' signal";
			return nullptr;
		}
		auto text = BBox::buttonText(BBox::Button(button), BBox::SkinLayout);
		item->setProperty("text", text);
		item->setParentItem(p);
		connect(item, clicked, p, emitClicked);
		return item;
	}

	void clear() {
		qDeleteAll(items);
		items.clear();
		create = true;
		p->polish();
	}

	bool create = true;
	QMetaMethod emitClicked;
};

ButtonBoxItem::ButtonBoxItem(QQuickItem *parent)
	: QQuickItem(parent)
	, d(new Data)
{
	d->p = this;
	auto &mo = staticMetaObject;
	d->emitClicked = mo.method(mo.indexOfSlot("emitClicked()"));
	Q_ASSERT(d->emitClicked.isValid());
}

ButtonBoxItem::~ButtonBoxItem() {
	qDeleteAll(d->items);
	delete d;
}

QQuickItem *ButtonBoxItem::clickedButton() const {
	return d->clicked;
}

void ButtonBoxItem::emitClicked() {
	auto clicked = qobject_cast<QQuickItem*>(sender());
	auto idx = d->items.indexOf(clicked);
	emit this->clicked(idx < 0 ? -1 : d->buttons.value(idx));
	if (_Change(d->clicked, clicked))
		emit clickedButtonChanged();
}

QList<int> ButtonBoxItem::buttons() const {
	return d->buttons;
}

QQmlComponent *ButtonBoxItem::source() const {
	return d->source;
}

void ButtonBoxItem::setSource(QQmlComponent *source) {
	if (_Change(d->source, source)) {
		d->clear();
		emit sourceChanged();
	}
}

void ButtonBoxItem::setButtons(QList<int> buttons) {
	if (_Change(d->buttons, buttons)) {
		d->clear();
		emit buttonsChanged();
	}
}

void ButtonBoxItem::geometryChanged(const QRectF &new_, const QRectF &old) {
	QQuickItem::geometryChanged(new_, old);
	polish();
}

qreal ButtonBoxItem::buttonWidth() const {
	return d->bwidth;
}

qreal ButtonBoxItem::gap() const {
	return d->gap;
}

void ButtonBoxItem::setGap(qreal g) {
	if (_Change(d->gap, g)) {
		polish();
		emit gapChanged();
	}
}

void ButtonBoxItem::setButtonWidth(qreal w) {
	if (_Change(d->bwidth, w)) {
		polish();
		emit buttonWidthChanged();
	}
}

void ButtonBoxItem::updatePolish() {
	if (d->buttons.isEmpty())
		return;
	if (d->create) {
		Q_ASSERT(d->items.isEmpty());
		d->items.reserve(d->buttons.size());
		for (int button : d->buttons) {
			if (auto item = d->make(button))
				d->items.append(item);
		}
		d->create = false;
	}

	auto bw = d->bwidth;
	const int num = d->buttons.size();
	if (bw < 0)
		bw = (width() - (num-1)*qMax(0.0, d->gap))/num;
	qreal x = 0;
	for (auto item : d->items) {
		item->setWidth(bw);
		item->setHeight(height());
		item->setX(x);
		item->setY(0);
		x += bw + d->gap;
	}
}

//struct DialogItem::Data {
//	RectangleItem *bg = nullptr;
//	TextItem *title = nullptr;
//	QQuickItem *contentItem = nullptr;
//	Buttons buttons = 0;
//	QList<DialogButtonItem*> buttonItems;
//};

//DialogItem::DialogItem(QQuickItem *parent)
//: QQuickItem(parent), d(new Data) {
//	d->bg = new RectangleItem(this);
//	d->bg->anchorFill(this);
//	d->bg->setColor(Qt::black);
//	d->bg->setRadius(2);

//	d->title = new TextItem(this);
//	d->title->setColor(Qt::white);
//	d->title->setAlignment(Qt::AlignCenter);
//	d->title->setMargins(5);
//	d->title->anchor(QtItem::Top, this, QtItem::Top);
//	d->title->anchor(QtItem::Left, this, QtItem::Left);
//	d->title->anchor(QtItem::Right, this, QtItem::Right);
//	setTitle("Test text");
//}

//DialogItem::~DialogItem() {
//	delete d;
//}

//QString DialogItem::title() const {
//	return d->title->text();
//}

//void DialogItem::setTitle(const QString &title) {
//	if (this->title() != title) {
//		d->title->setText(title);
//		emit titleChanged();
//	}
//}

//QQuickItem *DialogItem::contentItem() const {
//	return d->contentItem;
//}

//void DialogItem::setContentItem(QQuickItem *item) {
//	if (d->contentItem != item) {
//		if (d->contentItem)
//			d->contentItem->setParentItem(nullptr);
//		d->contentItem = item;
//		if (d->contentItem)
//			d->contentItem->setParentItem(this);
//		polish();
//	}
//}

//void DialogItem::updatePolish() {
//}

//void DialogItem::setButtons(Buttons buttons) {
//	if (_Change(d->buttons, buttons)) {

//		emit buttonsChanged();
//	}
//}

//DialogItem::Buttons DialogItem::buttons() const {
//	return d->buttons;
//}
