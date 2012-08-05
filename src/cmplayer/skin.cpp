#include "skin.hpp"
#include "info.hpp"
#include <QtCore/QMetaProperty>
#include <QtGui/QLayout>
#include "rootmenu.hpp"
#include "playengine.hpp"
#include "audiocontroller.hpp"
#include "videorenderer.hpp"
#include <QtGui/QSlider>
#include <QtUiTools/QUiLoader>
#include <QtGui/QAbstractButton>
#include <QtGui/QLabel>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtGui/QWidget>
#include <QtCore/QDebug>
#include "../cmplayer_widgets/widgets.hpp"
#include <QtPlugin>

Q_IMPORT_PLUGIN(cmplayer_widgets)

template<typename T1RandomAccessIterator, typename T2, typename T1LessThanT2, typename T2LessThanT1>
T1RandomAccessIterator binarySearch(T1RandomAccessIterator begin, T1RandomAccessIterator end, const T2 &value, T1LessThanT2 t1t2, T2LessThanT1 t2t1) {
	const auto it = qLowerBound(begin, end, value, t1t2);	return (it == end || t2t1(value, *it)) ? end : it;
}

template<typename T1Container, typename T2>
auto binarySearch(const T1Container &c, const T2 &value) -> decltype(c.begin()) {
	auto t1t2 = [](const typename T1Container::value_type &t1, const T2 &t2) {return t1 < t2;};
	auto t2t1 = [](const T2 &t2, const typename T1Container::value_type &t1) {return t2 < t1;};
	return binarySearch(c.begin(), c.end(), value, t1t2, t2t1);
}

static bool operator < (const QStringRef &lhs, const QLatin1String &rhs) {return lhs.compare(rhs) < 0;}
static bool operator < (const QLatin1String &lhs, const QStringRef &rhs) {return rhs.compare(lhs) > 0;}

// should be or
enum Placeholder : int {
	app_name,
	app_version,
	media_duration,
	media_name,
	media_number,
	media_position,
	media_state,
	total_media_count,
	PlaceholderMax
};

typedef QLatin1String _L;

struct PlaceholderInfo {
	PlaceholderInfo(Placeholder ph, const char *text): ph(ph), text(text) {}
	Placeholder ph;
	const char *text;
};

static bool operator < (const PlaceholderInfo &lhs, const QStringRef &rhs) {return _L(lhs.text) < rhs;}
static bool operator < (const QStringRef &lhs, const PlaceholderInfo &rhs) {return lhs < _L(rhs.text);}

static QList<PlaceholderInfo> makePlaceholders() {
	QList<PlaceholderInfo> list;
#define add(a) {list << PlaceholderInfo(a, "[:" #a ":]");}
	add(app_name);
	add(app_version);
	add(media_duration);
	add(media_name);
	add(media_number);
	add(media_position);
	add(media_state);
	add(total_media_count);
#undef add
	qSort(list.begin(), list.end(), [](const PlaceholderInfo &lhs, const PlaceholderInfo &rhs) -> bool {return strcmp(lhs.text, rhs.text) < 0;});
	return list;
}

static auto placeholders = makePlaceholders();

struct PlaceholderLabel {
	void update() {
		int size = 0;
		for (const auto &token : m_tokens)
			size += token.size();
		QString text;
		text.reserve(size);
		for (const auto &token : m_tokens)
			text += token;
		m_label->setText(text);
	}
	void set(Placeholder ph, const QString &text) {
		auto it = m_phIdx.find(ph);
		if (it != m_phIdx.end() && !it->isEmpty()) {
			for (int idx : *it)
				m_tokens[idx] = text;
			update();
		}
	}
	static PlaceholderLabel *make(Label *label) {
		QStringList tokens;
		QMap<Placeholder, QList<int>> phIdx;
		const QString text = label->text();
		int begin = 0, end = 0;
		auto addToken = [&tokens, &text] (int begin, int end) {
			const QStringRef ref = text.midRef(begin, end - begin);
			if (!ref.isEmpty())
				tokens.append(ref.toString());
		};
		forever {
			begin = text.indexOf(_L("[:"), end);
			if (begin < 0) {
				addToken(end, text.size());
				break;
			}
			addToken(end, begin);
			end = text.indexOf(_L(":]"), begin + 2);
			if (end < 0)
				break;
			end += 2;
			const QStringRef ref = text.midRef(begin, end - begin);
			if (!ref.isEmpty()) {
				auto it = binarySearch(::placeholders, ref);
				if (it != ::placeholders.end())
					phIdx[it->ph] << tokens.size();
				tokens.append(ref.toString());
			}
		}
		if (phIdx.isEmpty())
			return nullptr;
		auto ph = new PlaceholderLabel;
		ph->m_label = label;
		ph->m_tokens = tokens;
		ph->m_phIdx = phIdx;
		return ph;
	}
	const QMap<Placeholder, QList<int> > placeholders() const {return m_phIdx;}
	const Label *label() const {return m_label;}
private:
	QMap<Placeholder, QList<int> > m_phIdx;
	QStringList m_tokens;
	Label *m_label = nullptr;
};

struct Skin::Data {
	QWidget *w = nullptr;
	QWidget *screen = nullptr;
	QString path;
	QList<QWidget*> hidable;
	QSlider *seek = nullptr;
	QSlider *volume = nullptr;
	bool ticking = false;
	PlayEngine *engine = nullptr;
	AudioController *audio = nullptr;
	VideoRenderer *video = nullptr;
	QMap<QAction*, QList<QAbstractButton*> > buttons;
	QList<PlaceholderLabel*> uniqueLabelList;
	Label *titleProxy;
	QVector<QList<PlaceholderLabel*> > labels = decltype(labels)(PlaceholderMax);
	int prevPos = -1, mediaNumber = 0, mediaCount = 0;
};

Skin::Skin(QObject *parent)
: QObject (parent), d(new Data) {
	d->titleProxy = new Label;
	connect(d->titleProxy, SIGNAL(textChanged(QString)), this, SIGNAL(windowTitleChanged(QString)));
}

Skin::~Skin() {
	delete d->titleProxy;
	delete d->w;
	delete d;
}

void Skin::seek(int time) {
	if (!d->ticking && d->engine)
		d->engine->seek(time);
}

void Skin::onSeekableChanged(bool seekable) {
	if (d->seek)
		d->seek->setEnabled(seekable);
}

void Skin::connectTo(PlayEngine *engine, AudioController *audio, VideoRenderer *video) {
	if (d->engine != engine) {
		if (d->engine)
			disconnect(d->engine, 0, this, 0);
		d->engine = engine;
		if (d->engine) {
			if (d->seek) {
				d->seek->setRange(0, d->engine->duration());
				d->seek->setEnabled(d->engine->isSeekable());
				d->seek->setValue(d->engine->position());
			}
			connect(engine, SIGNAL(durationChanged(int)), this, SLOT(onDurationChanged(int)));
			connect(engine, SIGNAL(tick(int)), this, SLOT(onTick(int)));
			connect(engine, SIGNAL(seekableChanged(bool)), this, SLOT(onSeekableChanged(bool)));
			connect(engine, SIGNAL(stateChanged(State,State)), this, SLOT(onStateChanged(State)));
			connect(engine, SIGNAL(mrlChanged(Mrl)), this, SLOT(onMrlChanged(Mrl)));
		}
	}
	if (d->audio != audio) {
		if (d->audio)
			disconnect(d->audio, 0, this, 0);
		d->audio = audio;
		if (d->audio) {
			if (d->volume)
				d->volume->setValue(audio->volume());
			connect(audio, SIGNAL(volumeChanged(int)), this, SLOT(onVolumeChanged(int)));
		}
	}
	if (d->video != video) {
		if (d->video)
			disconnect(d->video, 0, this, 0);
		d->video = video;
	}
}

void Skin::initializePlaceholders() {
	if (d->engine) {
		setPlaceholder(media_duration, msecToString(d->engine->duration()));
		setPlaceholder(media_position, msecToString(d->engine->position()));
		setPlaceholder(media_name, d->engine->mrl().displayName());
		setPlaceholder(media_state, mediaStateText(d->engine->state()));
	} else {
		setPlaceholder(media_duration, msecToString(0));
		setPlaceholder(media_position, msecToString(0));
		setPlaceholder(media_name, tr("No media"));
		setPlaceholder(media_state, tr("Unusable"));
	}
	setPlaceholder(app_name, Info::name());
	setPlaceholder(app_version, Info::version());
	setPlaceholder(media_number, QString::number(d->mediaNumber));
	setPlaceholder(total_media_count, QString::number(d->mediaCount));
}

void Skin::onVolumeChanged(int volume) {
	if (d->volume)
		d->volume->setValue(volume);
}

void Skin::setVolume(int volume) {
	if (d->audio)
		d->audio->setVolume(volume);
}

void Skin::onDurationChanged(int duration) {
	if (d->seek) {
		d->ticking = true;
		d->seek->setRange(0, duration);
		d->ticking = false;
	}
	setPlaceholder(media_duration, msecToString(duration));
}

void Skin::onTick(int tick) {
	if (d->seek) {
		d->ticking = true;
		d->seek->setValue(tick);
		d->ticking = false;
	}
	const int secs = tick/1000;
	if (d->prevPos != secs) {
		d->prevPos = secs;
		setPlaceholder(media_position, secToString(secs));
	}
}

bool Skin::load(const QString &path, QWidget *parent) {
	if (d->w) {
		delete d->w;
		d->w = nullptr;
		d->screen = nullptr;
		d->hidable.clear();
		d->volume = d->seek = nullptr;
		for (auto it = d->buttons.begin(); it != d->buttons.end(); ++it)
			disconnect(it.key(), 0, this, 0);
		d->buttons.clear();
		qDeleteAll(d->uniqueLabelList);
		d->labels.clear();
		d->uniqueLabelList.clear();
	}
	QFile file(path);
	QFileInfo info(file);
	if (!file.open(QFile::ReadOnly))
		return false;
	QUiLoader ui;
	ui.setWorkingDirectory(info.absoluteDir());
	if ((d->w = ui.load(&file, parent))) {
		addHidableWidgets(d->w);
		QWidget *parent = d->screen;
		while (parent) {
			parent->setMouseTracking(true);
			parent = parent->parentWidget();
		}
		if ((d->seek = d->w->findChild<QSlider*>("seek_slider"))) {
			connect(d->seek, SIGNAL(valueChanged(int)), this, SLOT(seek(int)));
			if (d->engine) {
				d->ticking = true;
				d->seek->setRange(0, d->engine->duration());
				d->seek->setEnabled(d->engine->isSeekable());
				d->seek->setValue(d->engine->position());
				d->ticking = false;
			}
		}
		if ((d->volume = d->w->findChild<QSlider*>("volume_slider"))) {
			d->volume->setRange(0, 100);
			if (d->audio)
				d->volume->setValue(d->audio->volume());
			connect(d->volume, SIGNAL(valueChanged(int)), this, SLOT(setVolume(int)));
		}

		QList<QAbstractButton*> buttons = d->w->findChildren<QAbstractButton*>();
		auto &root = RootMenu::get();
		for (QAbstractButton *button : buttons) {
			const auto id = button->property("action").toString();
			if (!id.isEmpty()) {
				QAction *action = root.action(id);
				if (action) {
					d->buttons[action].append(button);
					if (action->isCheckable()) {
						button->setCheckable(true);
						button->setChecked(action->isChecked());
						connect(action, SIGNAL(toggled(bool)), button, SLOT(setChecked(bool)));
						connect(button, SIGNAL(clicked(bool)), action, SLOT(setChecked(bool)));
					} else {
						connect(button, SIGNAL(clicked()), action, SLOT(trigger()));
					}
					button->setToolTip(action->text());
					connect(action, SIGNAL(changed()), this, SLOT(onActionChanged()));
				} else
					qDebug() << "Cannot bind the button" << button->objectName() << "to" << id;
			}
		}

		auto labels = d->w->findChildren<Label*>();
		labels << d->titleProxy;
		d->titleProxy->setText(d->w->windowTitle());
		for (auto label : labels) {
			auto ph = PlaceholderLabel::make(label);
			if (!ph)
				continue;
			d->uniqueLabelList << ph;
			auto phs = ph->placeholders();
			for (auto it = phs.begin(); it != phs.end(); ++it)
				d->labels[it.key()] << ph;
		}
	}
	return d->w && d->screen;
}

void Skin::onActionChanged() {
	auto action = qobject_cast<QAction*>(sender());
	if (action) {
		auto buttons = d->buttons.value(action);
		for (auto button : buttons) {
			button->setEnabled(action->isEnabled());
			button->setToolTip(action->text());
		}
	}
}

void Skin::addHidableWidgets(QWidget *parent) {
	auto objs = parent->children();
	parent = nullptr;
	for (auto obj : objs) {
		if (!obj->isWidgetType())
			continue;
		auto w = static_cast<QWidget*>(obj);
		if (w->objectName() == "screen") {
			d->screen = w;
			continue;
		}
		if (!parent) {
			if (w->findChild<QWidget*>("screen"))
				parent = w;
		}
		if (w != parent)
			d->hidable.append(w);
	}
	if (parent)
		addHidableWidgets(parent);
}

QWidget *Skin::topParentWidget(QWidget *widget) {
	QWidget *p = widget->parentWidget();
	return p ? topParentWidget(p) : p;
}

QWidget *Skin::widget() const {
	return d->w;
}

QWidget *Skin::screen() const {
	return d->screen;
}

void Skin::setVisible(bool visible) {
	for (auto w : d->hidable) {
		w->setVisible(visible);
	}
}

bool Skin::contains(const QPoint &gpos) const {
	for (auto w : d->hidable) {
		if (w->rect().contains(w->mapFromGlobal(gpos)))
			return true;
	}
	return false;
}

void Skin::setPlaceholder(Placeholder ph, const QString &text) {
	if (d->labels[ph].isEmpty())
		return;
	for (auto label : d->labels[ph]) {
		label->set(ph, text);
		if (label->label() != d->titleProxy || ph != media_name || !d->engine)
			continue;
		const auto file = d->engine->mrl().toLocalFile();
		if (file.isEmpty())
			continue;
		QFileInfo info(file);
		emit windowFilePathChanged(info.absoluteFilePath());
	}
}

void Skin::setMediaNumber(int number) {
	if (number != d->mediaNumber)
		setPlaceholder(media_number, QString::number(d->mediaNumber = number));
}

void Skin::setTotalMediaCount(int count) {
	if (count != d->mediaCount)
		setPlaceholder(total_media_count, QString::number(d->mediaCount = count));
}

void Skin::onMrlChanged(const Mrl &mrl) {
	setPlaceholder(media_name, mrl.displayName());
}

QString Skin::mediaStateText(State state) {
	switch (state) {
	case State::Playing:
		return tr("Playing");
	case State::Stopped:
		return tr("Stopped");
	case State::Finished:
		return tr("Finished");
	case State::Buffering:
		return tr("Buffering");
	case State::Opening:
		return tr("Opening");
	case State::Error:
		return tr("Error");
	case State::Preparing:
		return tr("Preparing");
	default:
		return tr("Paused");
	}
};

void Skin::onStateChanged(State state) {
	setPlaceholder(media_state, mediaStateText(state));
}
