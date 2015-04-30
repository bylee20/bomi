#include "channelmanipulationwidget.hpp"
#include "enumcombobox.hpp"
#include "verticallabel.hpp"
#include "audio/channellayoutmap.hpp"
#include "audio/channelmanipulation.hpp"
#include "misc/objectstorage.hpp"
#include <QHeaderView>
#include <QTableWidget>

struct ChannelManipulationWidget::Data {
    ChannelComboBox *output, *input;
    QTableWidget *table;
    ChannelLayoutMap map = ChannelLayoutMap::default_();

    ChannelLayout currentInput = ChannelLayout::_2_0;
    ChannelLayout currentOutput = ChannelLayout::_2_0;

    ObjectStorage storage;

    void makeTable() {
        table->blockSignals(true);
        mp_chmap src, dest;
        ChannelLayout output = this->output->currentEnum();
        ChannelLayout  input = this-> input->currentEnum();
        auto makeHeader = [] (ChannelLayout layout, mp_chmap &chmap) {
            _ChmapFromLayout(&chmap, layout);
            QStringList header;
            for (int i=0; i<chmap.num; ++i) {
                const int speaker = chmap.speaker[i];
                Q_ASSERT(_InRange<int>(MP_SPEAKER_ID_FL,
                                       speaker, MP_SPEAKER_ID_SR));
                auto abbr = ChannelLayoutMap::channelNames()[speaker].abbr;
                header.push_back(_L(abbr));
            }
            return header;
        };
        auto header = makeHeader(output, dest);
        table->setRowCount(header.size());
        table->setVerticalHeaderLabels(header);

        header = makeHeader(input, src);
        table->setColumnCount(header.size());
        table->setHorizontalHeaderLabels(header);

        auto hv = table->horizontalHeader();
        hv->setSectionResizeMode(QHeaderView::ResizeToContents);
        hv = table->verticalHeader();
        hv->setSectionResizeMode(QHeaderView::ResizeToContents);
        hv->setDefaultAlignment(Qt::AlignRight);

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
                const auto speaker = static_cast<mp_speaker_id>(src.speaker[j]);
                const auto has = sources.contains(speaker);
                item->setCheckState(has ? Qt::Checked : Qt::Unchecked);
            }
        }
        currentInput = input;
        currentOutput = output;
        table->blockSignals(false);
    }
    void fillMap() {
        if (!table->rowCount() || !table->columnCount())
            return;
        mp_chmap src, dst;
        auto getChMap = [] (mp_chmap &chmap, ChannelLayout layout) {
            _ChmapFromLayout(&chmap, layout);
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
    hbox->addWidget(new QLabel(u'→'_q));
    hbox->addWidget(d->output);
    hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    vbox->addLayout(hbox);

    auto grid = new QGridLayout;
    vbox->addLayout(grid);
    hbox = new QHBoxLayout;
    auto si = new QSpacerItem(50, 0, QSizePolicy::Fixed, QSizePolicy::Fixed);
    hbox->addSpacerItem(si);
    hbox->addWidget(new QLabel(tr("Inputs")));
    grid->addLayout(hbox, 0, 1);
    auto vbox2 = new QVBoxLayout;
    si = new QSpacerItem(0, 50, QSizePolicy::Fixed, QSizePolicy::Fixed);
    vbox2->addSpacerItem(si);
    vbox2->addWidget(new VerticalLabel(tr("Outputs")));
    grid->addLayout(vbox2, 1, 0);
    grid->addWidget(d->table, 1, 1);

    QString ex;
    for (auto &name : ChannelLayoutMap::channelNames()) {
        if (!ex.isEmpty())
            ex += '\n'_q;
        ex += _L(name.abbr) % ": "_a % name.description();
    }
    grid->addWidget(new QLabel(ex), 0, 2, 2, 1);
    auto onComboChanged = [this] () { d->fillMap(); d->makeTable(); };
    connect(d->output, &DataComboBox::currentDataChanged, this, onComboChanged);
    connect(d-> input, &DataComboBox::currentDataChanged, this, onComboChanged);

    d->storage.setObject(this, u"channel-layouts"_q);
    d->storage.add("input", &d->currentInput);
    d->storage.add("output", &d->currentOutput);
    d->storage.restore();
    setCurrentLayouts(d->currentInput, d->currentOutput);

    auto signal = &ChannelManipulationWidget::mapChanged;
    connect(d->table, &QTableWidget::itemChanged, this, signal);
}

ChannelManipulationWidget::~ChannelManipulationWidget()
{
    d->storage.save();
    delete d;
}

auto ChannelManipulationWidget::setCurrentLayouts(ChannelLayout src,
                                                  ChannelLayout dst) -> void
{
    d->output->setCurrentEnum(dst);
    d-> input->setCurrentEnum(src);
}

auto ChannelManipulationWidget::setMap(const ChannelLayoutMap &map) -> void
{
    d->fillMap();
    if (_Change(d->map, map)) {
        d->makeTable();
        emit mapChanged();
    }
}

auto ChannelManipulationWidget::map() const -> ChannelLayoutMap
{
    d->fillMap();
    return d->map;
}
