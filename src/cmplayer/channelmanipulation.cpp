#include "channelmanipulation.hpp"
#include "log.hpp"
#include "enums.hpp"
#include "widgets.hpp"
#include "record.hpp"

DECLARE_LOG_CONTEXT(Audio)

struct ChannelName {const char *abbr; const char *desc;};

ChannelName ChNames[] = {
	{ "FL", "Front Left" },
	{ "FR", "Front Right" },
	{ "FC", "Front Center" },
	{ "LFE", "Low Frequency Effects" },
	{ "BL", "Back Left" },
	{ "BR", "Back Right" },
	{ "FLC", "Front Left-of-Center" },
	{ "FRC", "Front Right-of-Center" },
	{ "BC", "Back Center" },
	{ "SL", "Side Left" },
	{ "SR", "Side Right" }
};

static constexpr int ChNamesSize = sizeof(ChNames)/sizeof(ChNames[0]);

bool ChannelManipulation::isIdentity() const {
	for (int i=0; i<m_mix.size(); ++i) {
		const auto speaker_out = (mp_speaker_id)i;
		auto &sources = this->sources(speaker_out);
		if (sources.size() != 1 || sources.first() == speaker_out)
			return false;
	}
	return true;
}

QString ChannelManipulation::toString() const {
	QStringList list;
	for (int i=0; i<(int)m_mix.size(); ++i) {
		auto speaker = (mp_speaker_id)i;
		if (MP_SPEAKER_ID_FL <= speaker && speaker <= MP_SPEAKER_ID_SR && !m_mix[i].isEmpty()) {
			QStringList srcs;
			for (auto &src : m_mix[i])
				srcs << _L(ChNames[src].abbr);
			list << _L(ChNames[speaker].abbr) % '!' % srcs.join('/');
		}
	}
	return list.join(',');
}

ChannelManipulation ChannelManipulation::fromString(const QString &text) {
	ChannelManipulation man;
	auto list = text.split(',');
	auto nameToId = [] (const QString &name) {
		for (int i=0; i<ChNamesSize; ++i) {
			if (name == _L(ChNames[i].abbr))
				return (mp_speaker_id)i;
		}
		return MP_SPEAKER_ID_COUNT;
	};

	for (auto &one : list) {
		auto map = one.split('!', QString::SkipEmptyParts);
		if (map.size() != 2)
			continue;
		auto dest = nameToId(map[0]);
		if (dest == MP_SPEAKER_ID_COUNT)
			continue;
		auto srcs = map[1].split('/', QString::SkipEmptyParts);
		SourceArray sources;
		for (int i=0; i<srcs.size(); ++i) {
			auto src = nameToId(srcs[i]);
			if (src != MP_SPEAKER_ID_COUNT)
				sources.append(src);
		}
		if (!sources.isEmpty())
			man.set(dest, sources);
	}
	return man;
}

static inline mp_speaker_id to_mp_speaker_id(SpeakerId speaker) { return SpeakerIdInfo::data(speaker); }

static QVector<SpeakerId> speakersInLayout(ChannelLayout layout) {
	QVector<SpeakerId> list;
#define CHECK(a) {if (SpeakerId::a & (int)layout) {list << SpeakerId::a;}}
	CHECK(FrontLeft);
	CHECK(FrontRight);
	CHECK(FrontCenter);
	CHECK(LowFrequency);
	CHECK(BackLeft);
	CHECK(BackRight);
	CHECK(FrontLeftCenter);
	CHECK(FrontRightCenter);
	CHECK(BackCenter);
	CHECK(SideLeft);
	CHECK(SideRight);
#undef CHECK
	return list;
}

ChannelLayoutMap ChannelLayoutMap::default_() {
	ChannelLayoutMap map;
	auto &items = ChannelLayoutInfo::items();
	auto _mp = [] (SpeakerId speaker) { return to_mp_speaker_id(speaker); };

	for (auto &srcItem : items) {
		const auto srcLayout = srcItem.value;
		const auto srcSpeakers = speakersInLayout(srcLayout);
		for (auto &dstItem : items) {
			const auto dstLayout = dstItem.value;
			auto &mix = map.get(srcLayout, dstLayout).m_mix;
			for (auto srcSpeaker : srcSpeakers) {
				const auto mps = to_mp_speaker_id(srcSpeaker);
				if (srcSpeaker & (int)dstLayout) {
					mix[mps] << mps;
					continue;
				}
				if (dstLayout == ChannelLayout::Mono) {
					mix[MP_SPEAKER_ID_FC] << mps;
					continue;
				}

				auto testAndSet = [&mix, _mp, mps, dstLayout] (SpeakerId id) {
					if (dstLayout & (int)id) {
						mix[_mp(id)] << mps;
						return true;
					} else
						return false;
				};
				auto testAndSet2 = [dstLayout, &mix, _mp, mps] (SpeakerId left, SpeakerId right) {
					if (dstLayout & (left | right)) {
						mix[_mp(left)] << mps;
						mix[_mp(right)] << mps;
						return true;
					} else
						return false;
				};
				auto setLeft = [&mix, mps] () { mix[MP_SPEAKER_ID_FL] << mps; };
				auto setRight = [&mix, mps] () { mix[MP_SPEAKER_ID_FR] << mps; };
				auto setBoth = [&setLeft, &setRight] { setLeft(); setRight(); };
				switch (srcSpeaker) {
				case SpeakerId::FrontLeft:
				case SpeakerId::FrontRight:
					Q_ASSERT(false);
					break;
				case SpeakerId::LowFrequency:
					if (!testAndSet(SpeakerId::FrontCenter) || !testAndSet(SpeakerId::FrontLeftCenter))
						setLeft();
					break;
				case SpeakerId::FrontCenter:
					if (!testAndSet2(SpeakerId::FrontLeftCenter, SpeakerId::FrontRightCenter))
						setBoth();
					break;
				case SpeakerId::BackLeft:
					if (testAndSet(SpeakerId::BackCenter) || testAndSet(SpeakerId::SideLeft))
						break;
				case SpeakerId::FrontLeftCenter:
					setLeft();
					break;
				case SpeakerId::BackRight:
					if (testAndSet(SpeakerId::BackCenter) || testAndSet(SpeakerId::SideRight))
						break;
				case SpeakerId::FrontRightCenter:
					setRight();
					break;
				case SpeakerId::SideRight:
					if (!testAndSet(SpeakerId::BackRight))
						setRight();
					break;
				case SpeakerId::SideLeft:
					if (!testAndSet(SpeakerId::BackLeft))
						setLeft();
					break;
				case SpeakerId::BackCenter:
					if (!testAndSet2(SpeakerId::BackLeft, SpeakerId::BackRight)
							&& !testAndSet2(SpeakerId::SideLeft, SpeakerId::SideRight))
						setBoth();
					break;
				}
			}
		}
	}
	return map;
}

ChannelLayout ChannelLayoutMap::toLayout(const mp_chmap &chmap) {
	auto &items = SpeakerIdInfo::items();
	int layout = 0;
	for (int i=0; i<chmap.num; ++i) {
		auto id = (SpeakerId)(-1);
		for (auto &item : items) {
			if (item.data == chmap.speaker[i]) {
				id = item.value;
				break;
			}
		}
		if (id < 0) {
			char *str = nullptr;
			_Error("Cannot convert mp_chmap(%%) to ChannelLayout", str = mp_chmap_to_str(&chmap));
			talloc_free(str);
			return ChannelLayoutInfo::default_();
		}
		layout |= id;
	}
	return ChannelLayoutInfo::from(layout);
}

QString ChannelLayoutMap::toString() const {
	QStringList list;
	for (auto sit = m_map.begin(); sit != m_map.end(); ++sit) {
		auto srcName = ChannelLayoutInfo::name(sit.key());
		for (auto dit = sit->begin(); dit != sit->end(); ++dit) {
			auto dstName = ChannelLayoutInfo::name(dit.key());
			list << srcName % ":" % dstName % ":" % dit->toString();
		}
	}
	return list.join('#');
}

ChannelLayoutMap ChannelLayoutMap::fromString(const QString &text) {
	auto list = text.split('#', QString::SkipEmptyParts);
	ChannelLayoutMap map;
	for (auto &one : list) {
		auto parts = one.split(':', QString::SkipEmptyParts);
		if (parts.size() != 3)
			continue;
		auto src = ChannelLayoutInfo::from(parts[0]);
		auto dst = ChannelLayoutInfo::from(parts[1]);
		auto man = ChannelManipulation::fromString(parts[2]);
		map.get(src, dst) = man;
	}
	return map;
}

/*****************************************************/

class VerticalLabel : public QFrame {
public:
	VerticalLabel(QWidget *parent = nullptr): QFrame(parent) { }
	VerticalLabel(const QString &text, QWidget *parent = nullptr)
	: QFrame(parent) { setText(text); }
	void setText(const QString &text) { if (_Change(m_text, text)) recalc(); }
	QString text() const { return m_text; }
	QSize sizeHint() const override { return minimumSizeHint(); }
	QSize minimumSizeHint() const override { return m_size.transposed(); }
protected:
	void changeEvent(QEvent *event) override {
		if (event->type() == QEvent::FontChange)
			recalc();
	}
	void paintEvent(QPaintEvent *event) override {
		QFrame::paintEvent(event);
		QPainter painter(this);
		painter.translate((width())*0.5, (height())*0.5);
		painter.rotate(-90);
		QPoint p;
		auto m_alignment = Qt::AlignVCenter | Qt::AlignRight;
		switch (m_alignment & Qt::AlignVertical_Mask) {
		case Qt::AlignTop:
			p.ry() = -width()*0.5;
			break;
		case Qt::AlignBottom:
			p.ry() = width()*0.5 - m_size.height();
			break;
		default:
			p.ry() = m_size.height()*0.5;
			break;
		}
		switch (m_alignment & Qt::AlignHorizontal_Mask) {
		case Qt::AlignLeft:
			p.rx() = -height()*0.5;
			break;
		case Qt::AlignRight:
			p.rx() = height()*0.5 - m_size.width();
			break;
		default:
			p.rx() = -m_size.width()*0.5;
			break;
		}

		painter.drawText(p, m_text);
	}
	void recalc() {
		m_size = fontMetrics().boundingRect(m_text).size();
		updateGeometry();
		update();
	}
private:
	QString m_text;
	QSize m_size;
};

using ChannelComboBox = EnumComboBox<ChannelLayout>;

struct ChannelManipulationWidget::Data {
	ChannelComboBox *output, *input;
	QTableWidget *table;
	ChannelLayoutMap map = ChannelLayoutMap::default_();

	ChannelLayout currentInput = ChannelLayout::Mono;
	ChannelLayout currentOutput = ChannelLayout::Mono;

	void makeTable() {
		mp_chmap src, dest;
		ChannelLayout output = this->output->currentValue();
		ChannelLayout  input = this-> input->currentValue();
		auto makeHeader = [] (ChannelLayout layout, mp_chmap &chmap) {
			mp_chmap_from_str(&chmap, bstr0(ChannelLayoutInfo::data(layout).constData()));
			QStringList header;
			for (int i=0; i<chmap.num; ++i) {
				const int speaker = chmap.speaker[i];
				Q_ASSERT(MP_SPEAKER_ID_FL <= speaker && speaker <= MP_SPEAKER_ID_SR);
				header.append(QString::fromLatin1(ChNames[speaker].abbr));
			}
			return header;
		};
		auto header = makeHeader(output, dest);
		table->setRowCount(header.size());
		table->setVerticalHeaderLabels(header);

		header = makeHeader(input, src);
		table->setColumnCount(header.size());
		table->setHorizontalHeaderLabels(header);

		table->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
		table->verticalHeader()->setDefaultAlignment(Qt::AlignRight);

		mp_chmap_reorder_norm(&dest);
		mp_chmap_reorder_norm(&src);

		for (int i=0; i<table->rowCount(); ++i) {
			for (int j=0; j<table->columnCount(); ++j) {
				auto item = table->item(i, j);
				auto &man = map.get(input, output);
				if (!item) {
					item = new QTableWidgetItem;
					table->setItem(i, j, item);
				}
				auto &sources = man.sources((mp_speaker_id)dest.speaker[i]);
				item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
				item->setCheckState(sources.contains((mp_speaker_id)src.speaker[j]) ? Qt::Checked : Qt::Unchecked);
			}
		}

		currentInput = input;
		currentOutput = output;
	}
	void fillMap() {
		if (!table->rowCount() || !table->columnCount())
			return;
		mp_chmap src, dst;
		auto getChMap = [] (mp_chmap &chmap, ChannelLayout layout) {
			mp_chmap_from_str(&chmap, bstr0(ChannelLayoutInfo::data(layout).constData()));
			mp_chmap_reorder_norm(&chmap);
		};
		getChMap(src, currentInput);
		getChMap(dst, currentOutput);
		auto &man = map.get(currentInput, currentOutput);
		for (int i=0; i<table->rowCount(); ++i) {
			ChannelManipulation::SourceArray sources;
			for (int j=0; j<table->columnCount(); ++j) {
				auto item = table->item(i, j);
				if (!item)
					continue;
				if (item->checkState() == Qt::Checked)
					sources.append((mp_speaker_id)src.speaker[j]);
			}
			man.set((mp_speaker_id)dst.speaker[i], sources);
		}
	}
};

ChannelManipulationWidget::ChannelManipulationWidget(QWidget *parent)
: QWidget(parent), d(new Data) {
	d->output = new ChannelComboBox;
	d->input = new ChannelComboBox;
	d->table = new QTableWidget;
	d->table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	auto vbox = new QVBoxLayout;
	setLayout(vbox);

	auto hbox = new QHBoxLayout;
	hbox->addWidget(new QLabel(tr("Layout:")));
	hbox->addWidget(d->input);
	hbox->addWidget(new QLabel(_U("→")));
	hbox->addWidget(d->output);
	hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
	vbox->addLayout(hbox);

	auto grid = new QGridLayout;
	vbox->addLayout(grid);
	hbox = new QHBoxLayout;
	hbox->addSpacerItem(new QSpacerItem(50, 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
	hbox->addWidget(new QLabel(tr("Inputs")));
	grid->addLayout(hbox, 0, 1);
	auto vbox2 = new QVBoxLayout;
	vbox2->addSpacerItem(new QSpacerItem(0, 50, QSizePolicy::Fixed, QSizePolicy::Fixed));
	vbox2->addWidget(new VerticalLabel(tr("Outputs")));
	grid->addLayout(vbox2, 1, 0);
	grid->addWidget(d->table, 1, 1);

	QString ex;
	for (uint i=0; i<sizeof(ChNames)/sizeof(ChNames[0]); ++i) {
		if (i > 0)
			ex += _L('\n');
		ex += _L(ChNames[i].abbr) % _L(": ") % _L(ChNames[i].desc);
	}
	grid->addWidget(new QLabel(ex), 0, 2, 2, 1);
	auto onComboChanged = [this] (const QVariant &) { d->fillMap(); d->makeTable(); };
	connect(d->output, &DataComboBox::currentDataChanged, onComboChanged);
	connect(d-> input, &DataComboBox::currentDataChanged, onComboChanged);

	Record r("channel_layouts");
	ChannelLayout src = ChannelLayout::_2_0;
	ChannelLayout dst = ChannelLayout::_2_0;
	r.read(dst, "output");
	r.read(src, "input");
	setCurrentLayouts(src, dst);
}

ChannelManipulationWidget::~ChannelManipulationWidget() {
	Record r("channel_layouts");
	r.write(d->output->currentValue(), "output");
	r.write(d->input->currentValue(), "input");
	delete d;
}

void ChannelManipulationWidget::setCurrentLayouts(ChannelLayout src, ChannelLayout dst) {
	d->output->setCurrentValue(dst);
	d-> input->setCurrentValue(src);
}

void ChannelManipulationWidget::setMap(const ChannelLayoutMap &map) {
	d->map = map;
	d->makeTable();
}

ChannelLayoutMap ChannelManipulationWidget::map() const {
	d->fillMap();
	return d->map;
}
