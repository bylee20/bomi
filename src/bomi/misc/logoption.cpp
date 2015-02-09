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
    option.m_levels[LogOutput::StdOut] = Log::Off;
    option.m_levels[LogOutput::Journal] = Log::Debug;
#else
    option.m_levels[LogOutput::StdOut] = Log::Info;
    option.m_levels[LogOutput::Journal] = Log::Off;
#endif
    option.m_levels[LogOutput::StdErr] = Log::Off;
    option.m_levels[LogOutput::File] = Log::Off;

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
};

LogOptionWidget::LogOptionWidget(QWidget *parent)
    : QGroupBox(parent), d(new Data)
{
    auto vbox = new QVBoxLayout;

    auto grid = new QGridLayout;
    auto l = d->create(LogOutput::Journal, grid, 0, 0);
#if HAVE_SYSTEMD
    Q_UNUSED(l);
#else
    l->name->setEnabled(false);
    l->combo->setEnabled(false);
#endif

    auto file = d->create(LogOutput::File, grid, 1, 0)->combo;

    d->create(LogOutput::StdOut, grid, 0, 2);
    d->create(LogOutput::StdErr, grid, 1, 2);

    vbox->addLayout(grid);

    QWidget *fw = new QWidget;
    auto hbox = new QHBoxLayout;
    hbox->addWidget(new QLabel(tr("File Path")));
    hbox->setMargin(0);
    d->file = new QLineEdit;
    hbox->addWidget(d->file);
    fw->setLayout(hbox);

    vbox->addWidget(fw);
    setLayout(vbox);

    auto signal = &LogOptionWidget::optionChanged;
    for (auto &l : d->lines)
        PLUG_CHANGED(l.combo);
    PLUG_CHANGED(d->file);

    connect(SIGNAL_V(file, currentDataChanged), this,
            [=] () { fw->setEnabled(file->currentValue<Log::Level>()); });

    fw->setEnabled(false);
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
    return option;
}

auto LogOptionWidget::setOption(const LogOption &option) -> void
{
    for (auto it = d->lines.begin(); it != d->lines.end(); ++it)
        it->combo->setCurrentValue(option.level(it.key()));
    d->file->setText(option.file());
}
