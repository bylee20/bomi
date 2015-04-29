#include "logoption.hpp"
#include "configure.hpp"
#include "widget/datacombobox.hpp"

auto LogOption::operator == (const LogOption &rhs) const -> bool
{
    for (auto &item : LogOutputInfo::items()) {
        if (level(item.value) != rhs.level(item.value))
            return false;
    }
    return m_file == rhs.m_file;
}

auto LogOption::toJson() const -> QJsonObject
{
    QJsonObject json;
    json.insert(u"file"_q, file());
    json.insert(u"lines"_q, m_lines);
    QJsonObject map;
    for (auto it = m_levels.begin(); it != m_levels.end(); ++it)
        map.insert(_EnumName(it.key()), Log::name(it.value()));
    json.insert(u"levels"_q, map);
    return json;
}

auto LogOption::setFromJson(const QJsonObject &json) -> bool
{
    if (!json.contains(u"file"_q) || !json.contains(u"levels"_q))
        return false;
    m_lines = json[u"lines"_q].toDouble();
    m_file = json[u"file"_q].toString();
    auto map = json[u"levels"_q].toObject();
    m_levels.clear();
    for (auto it = map.begin(); it != map.end(); ++it)
        setLevel(_EnumFrom<LogOutput>(it.key()), Log::level(it.value().toString()));
    return true;
}

auto LogOption::default_() -> LogOption
{
    LogOption option;

#if HAVE_SYSTEMD
    option.m_levels[LogOutput::Journal] = Log::Debug;
#else
    option.m_levels[LogOutput::StdOut] = Log::Info;
#endif

    option.m_file = _WritablePath(Location::Cache) % "/log"_a;

    return option;
}

/******************************************************************************/

struct Line {
    QLabel *name = nullptr;
    DataComboBox *combo = nullptr;
};

struct LogOptionWidget::Data {
    QHash<LogOutput, Line> lines;
    QLineEdit *file = nullptr;
    QSpinBox *viewer = nullptr;
    auto create(LogOutput output, QGridLayout *grid, int r, int c) -> Line*
    {
        auto &l = lines[output];
        l.name = new QLabel(LogOutputInfo::description(output));
        l.combo = new DataComboBox;
        auto names = Log::levelNames();
        for (int i = 0; i < names.size(); ++i)
            l.combo->addItem(names[i], (Log::Level)i);
        grid->addWidget(l.name, r, c);
        grid->addWidget(l.combo, r, c+1);
        return &l;
    }

    auto sync(DataComboBox *) -> void { }
    template<class... Args>
    auto sync(DataComboBox *c, QWidget *w, Args... ws) -> void
    {
        connect(SIGNAL_V(c, currentDataChanged), w,
                [=] () { w->setEnabled(c->currentValue<Log::Level>()); });
        w->setEnabled(false);
        sync(c, ws...);
    }
};

LogOptionWidget::LogOptionWidget(QWidget *parent)
    : QGroupBox(parent), d(new Data)
{
    auto grid = new QGridLayout;
    auto space = [&] (int r, int c)
        { grid->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding), r, c); };

    d->create(LogOutput::Journal, grid, 0, 0);
    d->create(LogOutput::StdOut, grid, 0, 3);
    space(0, 5);
    d->create(LogOutput::StdErr, grid, 0, 6);
    space(0, 8);

    d->create(LogOutput::File, grid, 1, 0);
    grid->addWidget(new QLabel(tr("Path")), 1, 2);
    d->file = new QLineEdit;
    grid->addWidget(d->file, 1, 3, 1, -1);

    d->create(LogOutput::Viewer, grid, 2, 0);
    grid->addWidget(new QLabel(tr("Keep")), 2, 2);
    QWidget *vw = new QWidget;
    auto hbox = new QHBoxLayout;
    hbox->setMargin(0);
    d->viewer = new QSpinBox;
    d->viewer->setSuffix(tr(" Lines"));
    d->viewer->setSpecialValueText(tr("No Limit"));
    d->viewer->setRange(0, 9999999);
    d->viewer->setAccelerated(true);
    hbox->addWidget(d->viewer);
    hbox->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding));
    vw->setLayout(hbox);
    grid->addWidget(vw, 2, 3, 1, -1);

    setLayout(grid);

    auto signal = &LogOptionWidget::optionChanged;
    for (auto &l : d->lines)
        PLUG_CHANGED(l.combo);
    PLUG_CHANGED(d->file);

    auto file = d->lines[LogOutput::File].combo;
    d->sync(file, d->file);

    auto viewer = d->lines[LogOutput::Viewer].combo;
    d->sync(viewer, d->viewer);

#if !HAVE_SYSTEMD
    auto &l = d->lines[LogOutput::Journal];
    l.name->setEnabled(false);
    l.combo->setEnabled(false);
#endif
}

LogOptionWidget::~LogOptionWidget()
{
    delete d;
}

auto LogOptionWidget::option() const -> LogOption
{
    LogOption option;
    for (auto it = d->lines.begin(); it != d->lines.end(); ++it)
        option.setLevel(it.key(), it->combo->currentValue<Log::Level>());
    option.setFile(d->file->text());
    option.setLines(d->viewer->value());
    return option;
}

auto LogOptionWidget::setOption(const LogOption &option) -> void
{
    for (auto it = d->lines.begin(); it != d->lines.end(); ++it)
        it->combo->setCurrentValue(option.level(it.key()));
    d->file->setText(option.file());
    d->viewer->setValue(option.lines());
}
