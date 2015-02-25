#include "deintwidget.hpp"
#include "datacombobox.hpp"
#include "enumcombobox.hpp"
#include "video/deintcaps.hpp"
#include "video/deintoption.hpp"

using DeintMethodComboBox = EnumComboBox<DeintMethod>;

struct Line {
    QMap<DeintMethod, DeintCaps> caps;
    DeintMethodComboBox *combo = nullptr;
    QCheckBox *doubler = nullptr;
};

struct DeintWidget::Data {
    DeintWidget *p = nullptr;
    Line lines[2];
    auto line(Processor proc) -> Line& { return lines[proc == Processor::GPU]; }
    auto create(Processor proc, const QString &label, QGridLayout *grid) -> void
    {
        auto &l = line(proc);

        l.combo = new DeintMethodComboBox(false, p);
        Q_ASSERT(l.combo->enum_(0) == DeintMethod::None);
        l.combo->removeItem(0);
        for (auto &caps : DeintCaps::list(proc)) {
            Q_ASSERT(caps.supports(proc));
            const auto m = caps.method();
            l.caps[m] = caps;
            l.combo->addItem(_EnumName(m), QVariant::fromValue(m));
        }

        auto methodText = [] (DeintMethod method, const QString &desc) -> QString
            { return DeintMethodInfo::name(method) % ": "_a % desc; };
        l.combo->setToolTip(
            methodText(DeintMethod::Bob,
                       tr("Display each line twice.")) % '\n'_q %
            methodText(DeintMethod::LinearBob,
                       tr("Bob with linear interpolation.")) % '\n'_q %
            methodText(DeintMethod::CubicBob,
                       tr("Bob with cubic interpolation.")) % '\n'_q %
            methodText(DeintMethod::Yadif,
                       tr("Use complicated temporal and spatial interpolation."))
        );

        l.doubler = new QCheckBox(tr("Double framerate"), p);
        l.doubler->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        l.doubler->setToolTip(tr("This option makes the framerate doubled and motions smoother.\n"
                                 "This requires much more CPU or GPU usage."));

        const int row = proc == Processor::GPU;
        grid->addWidget(new QLabel(label), row, 0);
        grid->addWidget(l.combo, row, 1);
        grid->addWidget(l.doubler, row, 2);

        connect(SIGNAL_V(l.combo, currentDataChanged), p, [=, &l] (const QVariant &data) {
            l.doubler->setEnabled(l.caps[data.value<DeintMethod>()].doubler());
            emit p->optionsChanged();
        });
        connect(l.doubler, &QCheckBox::toggled, p, &DeintWidget::optionsChanged);
    }
    auto setOption(Processor proc, const DeintOption &option)
    {
        auto &l = line(proc);
        l.combo->setCurrentEnum(option.method);
        l.doubler->setChecked(option.doubler);
    }
    auto option(Processor proc) -> DeintOption
    {
        auto &l = line(proc);

        DeintOption opt;
        opt.processor = proc;
        opt.method = l.combo->currentEnum();
        opt.doubler = l.doubler->isEnabled() && l.doubler->isChecked();
        return opt;
    }
};

DeintWidget::DeintWidget(QWidget *parent)
    : QGroupBox(parent)
    , d(new Data)
{
    d->p = this;

    auto grid = new QGridLayout;
    d->create(Processor::CPU, tr("For S/W decoding"), grid);
    d->create(Processor::GPU, tr("For H/W decoding"), grid);
    setLayout(grid);
}

DeintWidget::~DeintWidget() {
    delete d;
}

auto DeintWidget::set(const DeintOptionSet &options) -> void
{
    d->setOption(Processor::CPU, options.option(Processor::CPU));
    d->setOption(Processor::GPU, options.option(Processor::GPU));
}

auto DeintWidget::get() const -> DeintOptionSet
{
    DeintOptionSet set;
    set.swdec = d->option(Processor::CPU);
    set.hwdec = d->option(Processor::GPU);
    return set;
}
