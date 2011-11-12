#include "controlwidget.hpp"
#include "widgets.hpp"
#include "squeezedlabel.hpp"
#include "playengine.hpp"
#include "libvlc.hpp"
#include "mrl.hpp"
#include <QtGui/QLinearGradient>
#include <QtGui/QStackedWidget>
#include <QtGui/QPainterPath>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPainter>
#include <QtGui/QAction>
#include <QtGui/QLabel>
#include <QtCore/QDebug>
#include <QtCore/QEvent>
#include <QtCore/QTimer>

class ControlWidget::Lcd : public QFrame {
public:
	Lcd(QWidget *parent = 0): QFrame(parent) {
		setObjectName("lcd");
		setStyleSheet("\
			QFrame#lcd {\
				margin: 0px; padding: 0px 3px;\
				border: 1px solid #aaa; border-radius: 3px;\
				background: qlineargradient(x1:0, y1:0, x2:0, y2:1\
				, stop:0 #111, stop:0.1 #6ad, stop:0.8 #6ad, stop:1 #fff);\
			}\
			QLabel {font-size: 11px;}"
		 );
		prevSecs = 0;
		QHBoxLayout *hbox = new QHBoxLayout(this);
		hbox->setContentsMargins(0, 0, 0, 0);
		hbox->setSpacing(0);

		stack = new QStackedWidget(this);
		time = new QLabel("00:00:00", this);
		QLabel *slash = new QLabel("/", this);
		duration = new QLabel("00:00:00", this);
		hbox->addWidget(stack);
		hbox->addWidget(time);
		hbox->addWidget(slash);
		hbox->addWidget(duration);

		QWidget *w = new QWidget(stack);
		hbox = new QHBoxLayout(w);
		hbox->setContentsMargins(0, 0, 0, 0);
		hbox->setSpacing(0);

		stack->addWidget(w);
		track = new QLabel(w);
		state = new QLabel(w);
		source = new SqueezedLabel(w);
		hbox->addWidget(track);
		hbox->addWidget(state);
		hbox->addWidget(source);
		stack->addWidget(w);

		message = new QLabel(this);
		stack->addWidget(message);

		stack->setCurrentIndex(Info);
		hider.setSingleShot(true);
	}
	static const int Info = 0;
	static const int Message = 1;
	SqueezedLabel *source;
	QStackedWidget *stack;
	QLabel *state, *track, *time, *duration, *message;
	int prevSecs;
	QTimer hider;
};

class ControlWidget::Slider : public QWidget {
public:
	Slider(QWidget *parent = 0)
	: QWidget(parent)
	, muted(":/img/speaker-off.png")
	, unmuted(":/img/speaker.png") {
		mute = new Button(this);
		mute->setBlock(false);
		mute->setFixedWidth(10);
		mute->setIcon(unmuted);
		QHBoxLayout *hbox = new QHBoxLayout(this);
		hbox->setSpacing(2);
		hbox->setContentsMargins(0, 0, 0, 0);
		setFixedHeight(15);
		hbox->addWidget(new SeekSlider(this));
		hbox->addWidget(mute);
		hbox->addWidget(new VolumeSlider(this));
	}
	QIcon muted, unmuted;
	Button *mute;
};

class ControlWidget::Boundary : public QWidget {
public:
	Boundary(QWidget *parent = 0): QWidget(parent) {
		setFixedHeight(5);
		setStyleSheet("background: "
			"qlineargradient(x1:0, y1:0, x2:0, y2:1, "
			"stop:0 #000, stop:0.1 #000, stop:0.5 #888, "
			"stop:0.6 #000, stop:1 #000);");
	}
};

struct ControlWidget::Data {
	Boundary *boundary;
	Lcd *lcd;
	Slider *slider;
	PlayEngine *engine;
	Button *play, *prev, *next, *forward, *backward;
	QBrush bg, light;
	QPainterPath path;
};

ControlWidget::ControlWidget(PlayEngine *engine, QWidget *parent)
: QWidget(parent), d(new Data) {
	QLinearGradient grad(0.5, 0.0, 0.5, 1.0);
	grad.setColorAt(0.0, qRgb(0, 0, 0));
	grad.setColorAt(1.0, qRgb(60, 60, 60));
	d->bg = QBrush(grad);

	grad = QLinearGradient(0.5, 0.1, 0.5, 0.9);
	grad.setColorAt(1.0, Qt::white);
	grad.setColorAt(0.0, Qt::transparent);
	d->light = QBrush(grad);

	d->path.moveTo(0.0, 1.0);
	d->path.cubicTo(0.05, 0.3, 0.05, 0.3, 0.5, 0.3);
	d->path.cubicTo(0.95, 0.3, 0.95, 0.3, 1.0, 1.0);
	d->path.closeSubpath();

	d->engine = engine;
	d->boundary = new Boundary(this);
	d->lcd = new Lcd(this);
	d->slider = new Slider(this);
	d->play = new Button(this);
	d->prev = new Button(this);
	d->next = new Button(this);
	d->forward = new Button(this);
	d->backward = new Button(this);
	d->forward->setBlock(false);
	d->backward->setBlock(false);
	d->play->setBlock(false);
	d->prev->setBlock(false);
	d->next->setBlock(false);
	setState(StoppedState);
	connect(&d->lcd->hider, SIGNAL(timeout()), this, SLOT(hideMessage()));
	connect(d->engine, SIGNAL(stateChanged(MediaState, MediaState))
		, this, SLOT(setState(MediaState)));
	connect(d->engine, SIGNAL(mrlChanged(Mrl)), this, SLOT(setMrl(Mrl)));
	connect(d->engine, SIGNAL(durationChanged(int)), this, SLOT(setDuration(int)));
	connect(d->engine, SIGNAL(tick(int)), this, SLOT(setPlayTime(int)));

	retranslateUi();

	setMinimumHeight(0);
	setMaximumHeight(100000);
	d->play->setIcon(QIcon(":/img/media-playback-start.png"));
	d->backward->setIcon(QIcon(":/img/arrow-left-double-gray.png"));
	d->prev->setIcon(QIcon(":/img/go-first-view-gray.png"));
	d->next->setIcon(QIcon(":/img/go-last-view-gray.png"));
	d->forward->setIcon(QIcon(":/img/arrow-right-double-gray.png"));

	const int big = d->lcd->sizeHint().height() + d->slider->height() - 2;
	const int small = big/2-4;
	d->play->setIconSize(big);
	d->prev->setIconSize(small);
	d->next->setIconSize(small);
	d->forward->setIconSize(small);
	d->backward->setIconSize(small);

	QGridLayout *left = new QGridLayout;
	left->setContentsMargins(0, 0, 0, 0);
	left->addWidget(d->play, 0, 0, 2, 1);
	left->addWidget(d->backward, 0, 1, 1, 1);
	left->addWidget(d->forward, 0, 2, 1, 1);
	left->addWidget(d->prev, 1, 1, 1, 1);
	left->addWidget(d->next, 1, 2, 1, 1);

	QVBoxLayout *middle = new QVBoxLayout;
	middle->addWidget(d->lcd);
	middle->addWidget(d->slider);

	QHBoxLayout *hbox = new QHBoxLayout;
	hbox->setContentsMargins(2, 0, 2, 0);
	hbox->addLayout(left);
	hbox->addLayout(middle);

	QVBoxLayout *layout = new QVBoxLayout(this);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(0);
	layout->addWidget(d->boundary);
	layout->addLayout(hbox);
	adjustSize();
	setFixedHeight(sizeHint().height());

}

ControlWidget::~ControlWidget() {
	delete d;
}

void ControlWidget::paintEvent(QPaintEvent */*event*/) {
	QPainter painter(this);
	painter.fillRect(rect(), Qt::black);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setPen(Qt::NoPen);
	QPointF topLeft(0, 0);
	painter.translate(topLeft);
	painter.scale(width(), height() - topLeft.y());
	painter.fillRect(rect(), d->bg);
	painter.setOpacity(0.3);
	painter.setBrush(d->light);
	painter.drawPath(d->path);
}

void ControlWidget::updateMuted(bool muted) {
	d->slider->mute->setIcon(muted ? d->slider->muted : d->slider->unmuted);
}

void ControlWidget::connectMute(QAction *action) {
	d->slider->mute->setAction(action, false);
	connect(action, SIGNAL(toggled(bool)), this, SLOT(updateMuted(bool)));
}

void ControlWidget::connectPlay(QAction *action) {
	d->play->setAction(action, false);
}

void ControlWidget::connectPrevious(QAction *action) {
	d->prev->setAction(action, false);
}

void ControlWidget::connectNext(QAction *action) {
	d->next->setAction(action, false);
}

void ControlWidget::connectForward(QAction *action) {
	d->forward->setAction(action, false);
}

void ControlWidget::connectBackward(QAction *action) {
	d->backward->setAction(action, false);
}

void ControlWidget::showMessage(const QString &msg, int time) {
	d->lcd->hider.stop();
	if (time > 0) {
		d->lcd->message->setText(msg);
		d->lcd->stack->setCurrentIndex(Lcd::Message);
		d->lcd->hider.start(time);
	}
}

void ControlWidget::setMrl(const Mrl &mrl) {
	d->lcd->source->setText(mrl.displayName());
	hideMessage();
}

void ControlWidget::setState(MediaState state) {
	QString text;
	if (state == PlayingState) {
		text += tr("Playing");
		d->play->setIcon(QIcon(":/img/media-playback-pause.png"));
	} else {
		if (state == StoppedState)
			text += tr("Stopped");
		else if (state == FinishedState)
			text += tr("Finished");
		else
			text += tr("Paused");
		d->play->setIcon(QIcon(":/img/media-playback-start.png"));
	}
	d->lcd->state->setText("(" + text + ") ");
	hideMessage();
}

void ControlWidget::setDuration(int duration) {
	d->lcd->duration->setText(msecToString(duration));
}

void ControlWidget::setPlayTime(int time) {
	const int secs = time/1000;
	if (d->lcd->prevSecs != secs) {
		d->lcd->time->setText(secToString(secs));
		d->lcd->prevSecs = secs;
	}
}

void ControlWidget::setTrackNumber(int nth, int total) {
	if (nth < 1 || total < 2)
		d->lcd->track->clear();
	else
		d->lcd->track->setText(QString("[%1/%2] ").arg(nth).arg(total));
	hideMessage();
}

void ControlWidget::hideMessage() {
	d->lcd->stack->setCurrentIndex(Lcd::Info);
}

void ControlWidget::retranslateUi() {
	d->backward->setToolTip(tr("Backward"));
	d->forward->setToolTip(tr("Forward"));
	d->next->setToolTip(tr("Next"));
	d->prev->setToolTip(tr("Previous"));
}

void ControlWidget::changeEvent(QEvent *event) {
	if (event->type() == QEvent::LanguageChange)
		retranslateUi();
	else
		QWidget::changeEvent(event);
}

