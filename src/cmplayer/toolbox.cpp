//#include "toolbox.hpp"
//#include "mainwindow.hpp"
//#include "controls.hpp"
//#include <QtGui/QFrame>
//#include <QtGui/QButtonGroup>
//#include <QtGui/QStackedWidget>
//#include <QtGui/QVBoxLayout>
//#include <QtGui/QHBoxLayout>
//#include <QtGui/QPainter>
//#include <QtCore/QDebug>
//#include <QtGui/QPushButton>

//class ToolBox::Frame : public QFrame {
//public:
//	Frame(QWidget *parent): QFrame(parent) {
//		w = new Widget(this);
//		QVBoxLayout *vbox = new QVBoxLayout(this);
//		vbox->setContentsMargins(0, 0, 0, 0);
//		vbox->addWidget(w);

//		vbox = new QVBoxLayout(w);
//		vbox->setContentsMargins(0, 0, 0, 0);
////		vbox->setContentsMargins(5, 10, 5, 10);
//		vbox->addWidget(stack = new QStackedWidget(w));

//		w->setObjectName("toolBoxBg");
//		setObjectName("toolBoxFrame");
//		setStyleSheet("\
//			QFrame#toolBoxFrame {\
//				margin: 0px; padding: 0px;\
//				border: 0px solid #aaa; border-radius: 5px;\
//			}\
//		");
//	}
//	QStackedWidget *stack;
//private:
//	class Widget : public QWidget {
//	public:
//		Widget(QWidget *parent): QWidget(parent) {
//			bg = palette().color(QPalette::Background);
//			setAutoFillBackground(true);
//		}
//	private:
//		void paintEvent(QPaintEvent */*event*/) {
//			QPainter painter(this);
//			const double top = 1.0/height();
//			const double bottom = 1.0 - 4.0/height();
//			QLinearGradient grad(0, 0, 0, height());
//			grad.setColorAt(0, qRgb(0x11, 0x11, 0x11));
//			grad.setColorAt(top, bg);
//			grad.setColorAt(bottom, bg);
//			grad.setColorAt(1.0, qRgb(0x11, 0x11, 0x11));
//			painter.fillRect(rect(), grad);
//		}
//		QColor bg;
//	};
//	Widget *w;
//};

//class ToolBox::ButtonWidget : public QWidget {
//public:
//	ButtonWidget(QWidget *parent): QWidget(parent) {
//		group = new QButtonGroup(this);
//		m_hbox = new QHBoxLayout(this);
//		m_hbox->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
//		m_hbox->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
//		m_count = 0;

//		QLinearGradient grad(0.5, 0.0, 0.5, 1.0);
//		grad.setColorAt(0.0, qRgb(0, 0, 0));
//		grad.setColorAt(1.0, qRgb(60, 60, 60));
//		bg = QBrush(grad);

//		grad = QLinearGradient(0.5, 0.1, 0.5, 0.9);
//		grad.setColorAt(1.0, Qt::white);
//		grad.setColorAt(0.0, Qt::transparent);
//		light = QBrush(grad);

//		path.moveTo(0.0, 1.0);
//		path.cubicTo(0.05, 0.3, 0.05, 0.3, 0.5, 0.3);
//		path.cubicTo(0.95, 0.3, 0.95, 0.3, 1.0, 1.0);
//		path.closeSubpath();
//	}
//	Button *addButton(int id, const QIcon &icon, const QString &text) {
//		Button *button = new Button(this);
//		button->setCheckable(true);
//		button->setIcon(icon);
//		button->setToolTip(text);
//		button->setText(text);
//		button->setIconSize(25);
//		group->addButton(button, id);
//		m_hbox->insertWidget(++m_count, button);
//		return button;
//	}
//	QButtonGroup *group;
//private:
//	void paintEvent(QPaintEvent */*event*/) {
//		QPainter painter(this);
//		painter.fillRect(rect(), Qt::black);
//		painter.setRenderHint(QPainter::Antialiasing);
//		painter.setPen(Qt::NoPen);
//		QPointF topLeft(0, 0);
//		painter.translate(topLeft);
//		painter.scale(width(), height() - topLeft.y());
//		painter.fillRect(rect(), bg);
//		painter.setOpacity(0.3);
//		painter.setBrush(light);
//		painter.drawPath(path);
//	}
//	int m_count;
//	QHBoxLayout *m_hbox;
//	QBrush bg, light;
//	QPainterPath path;
//};

//struct ToolBox::Data {
//	PlaylistView *playlist;
//	HistoryView *history;
//	SubtitleView *subtitle;
//	FavoritesView *favorite;
//	Frame *frame;
//	ButtonWidget *button;
//};

//ToolBox::ToolBox(MainWindow *mw)
//: QDialog(mw, Qt::Tool), d(new Data) {
//	setFocusPolicy(Qt::NoFocus);
//	d->frame = new Frame(this);
//	d->button = new ButtonWidget(this);
//	d->playlist = new PlaylistView(mw->engine(), d->frame);
//	d->favorite = new FavoritesView(mw->engine(), d->frame);
//	d->history = new HistoryView(mw->engine(), d->frame);
//	d->subtitle = new SubtitleView(d->frame);

//	addPage(d->playlist, tr("Playlist"), ":/img/view-media-playlist-%1.png");
//	addPage(d->favorite, tr("Favorites"), ":/img/favorites-%1.png");
//	addPage(d->history, tr("History"), ":/img/history-%1.png");
//	addPage(d->subtitle, tr("Subtitle"), ":/img/subtitle-view-icon-%1.png");

//	QVBoxLayout *vbox = new QVBoxLayout(this);
//	vbox->addWidget(d->frame);
//	vbox->addWidget(d->button);
//	vbox->setContentsMargins(3, 3, 3, 3);
//	vbox->setSpacing(0);

//	connect(d->button->group, SIGNAL(buttonClicked(int)), this, SLOT(changeWidget(int)));

//	d->button->group->button(0)->setChecked(true);
//	changeWidget(0);
//}


//ToolBox::~ToolBox() {
//	delete d;
//}

//void ToolBox::addPage(QWidget *widget, const QString &name, const QString &iconHolder) {
//	QIcon icon(iconHolder.arg("gray"));
//	icon.addFile(iconHolder.arg("color"), QSize(), QIcon::Normal, QIcon::On);
//	icon.addFile(iconHolder.arg("color"), QSize(), QIcon::Active);
//	d->button->addButton(d->frame->stack->count(), icon, name);
//	d->frame->stack->addWidget(widget);
//}

//void ToolBox::paintEvent(QPaintEvent */*event*/) {
//	QPainter painter(this);
//	painter.save();
//	painter.fillRect(rect(), Qt::black);
//	painter.setPen(qRgb(0x88, 0x88, 0x88));
//	painter.drawRect(1, 1, width() - 3, height() - 3);
//	painter.restore();

//	const QRectF box = boxRect();
//// 	const double frameBound = d->frame->geometry().bottom();
//	QLinearGradient grad(box.topLeft(), box.bottomLeft());
//	grad.setColorAt(0, Qt::black);
//// 	grad.setColorAt(frameBound/box.height(), qRgb(100, 100, 100));
//	grad.setColorAt(1, qRgb(200, 200, 200));
//	painter.fillRect(box, grad);
//}

//void ToolBox::resizeEvent(QResizeEvent *event) {
//	QDialog::resizeEvent(event);
//}

//void ToolBox::changeWidget(int id) {
//	d->frame->stack->setCurrentIndex(id);
//	const QString title = tr("TOOL BOX") + " - " + d->button->group->button(id)->toolTip();
//	setWindowTitle(title);
//}

//PlaylistView *ToolBox::playlist() const {
//	return d->playlist;
//}

//SubtitleView *ToolBox::subtitle() const {
//	return d->subtitle;
//}

//HistoryView *ToolBox::history() const {
//	return d->history;
//}
