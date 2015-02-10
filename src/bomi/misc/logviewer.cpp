#include "logviewer.hpp"
#include "logoption.hpp"
#include "dataevent.hpp"
#include "simplelistmodel.hpp"
#include "log.hpp"
#include "ui_logviewer.h"

static const int LogEvent = QEvent::User + 10;

struct LogEntry {
    Log::Level level = Log::Off;
    QString context, message;
};

class LogEntryModel : public SimpleListModel<LogEntry>
{
public:
    LogEntryModel()
    {
        m_fgs[Log::Off]   = Qt::transparent;
        m_fgs[Log::Fatal] = Qt::red;
        m_fgs[Log::Error] = Qt::red;
        m_fgs[Log::Warn]  = Qt::yellow;
        m_fgs[Log::Info]  = Qt::white;
        m_fgs[Log::Debug] = Qt::green;
        m_fgs[Log::Trace] = Qt::gray;
    }

    auto displayData(int row, int /*column*/) const -> QVariant final
        { return at(row).message; }
    auto fontData(int /*row*/, int /*column*/) const -> QFont final
        { return m_mono; }
    auto roleData(int row, int /*column*/, int role) const -> QVariant final
    {
        switch (role) {
        case Qt::ForegroundRole:
            return m_fgs[at(row).level];
        case Qt::BackgroundRole:
            return m_bg;
        }
        return QVariant();
    }
private:
    QBrush m_bg = Qt::black;
    QBrush m_fgs[Log::Trace + 1];
    QFont m_mono{u"monospace"_q};
};

class LogFilterModel : public QSortFilterProxyModel {
public:
    auto filterAcceptsRow(int srow, const QModelIndex &) const -> bool final
    {
        auto m = static_cast<LogEntryModel*>(sourceModel());
        const auto &e = m->at(srow);
        return level[e.level] && std::binary_search(ctx.begin(), ctx.end(), e.context);
    }
    QVector<QString> ctx;
    std::array<bool, Log::Trace + 1> level;
};

struct LogViewer::Data {
    Ui::LogViewer ui;
    std::set<QString> ctx;
    LogFilterModel proxy;
    LogEntryModel model;
    int lines = 0;
    QMenu *menu = nullptr;

    struct {
        QMap<QString, bool> context;
        QMap<Log::Level, bool> level;
    } state;

    auto syncContext() -> void
    {
        QVector<QString> ctx;
        for (int i = 0; i < ui.context->count(); ++i) {
            auto item = ui.context->item(i);
            if (item->checkState())
                ctx.push_back(item->text());
        }
        proxy.ctx = ctx;
        proxy.invalidate();
    }

    auto newItem(const QString &text, QListWidget *w) -> QListWidgetItem*
    {
        auto item = new QListWidgetItem;
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setText(text);
        w->addItem(item);
        return item;
    }

};

LogViewer::LogViewer(QWidget *parent)
    : QDialog(parent), d(new Data)
{
    d->ui.setupUi(this);
    std::fill(d->proxy.level.begin(), d->proxy.level.end(), true);

    QSettings s;
    s.beginGroup(u"LogViewer"_q);
    int len = s.beginReadArray(d->ui.context->objectName());
    for (int i = 0; i < len; ++i) {
        s.setArrayIndex(i);
        auto &&name = s.value(u"name"_q).toString();
        auto checked = s.value(u"checked"_q).toBool();
        if (d->ctx.find(name) != d->ctx.end())
            continue;
        auto item = d->newItem(name, d->ui.context);
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        d->ctx.insert(name);
    }
    s.endArray();

    QMap<Log::Level, bool> levels;
    len = s.beginReadArray(d->ui.level->objectName());
    for (int i = 0; i < len; ++i) {
        s.setArrayIndex(i);
        auto level = Log::level(s.value(u"name"_q).toString());
        levels[level] = s.value(u"checked"_q).toBool();
    }
    s.endArray();
    auto geometry = s.value(u"geometry"_q).toRect();
    s.endGroup();

    for (int lv = 1; lv <= Log::Trace; ++lv) {
        auto level = (Log::Level)lv;
        auto item = d->newItem(Log::name(level), d->ui.level);
        item->setData(Qt::UserRole, QVariant::fromValue(level));
        if (level > Log::option().level(LogOutput::Viewer))
            item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        const auto checked = levels.contains(level) ? levels[level] : true;
        item->setCheckState(checked ? Qt::Checked : Qt::Unchecked);
        d->proxy.level[level] = checked;
    }

    Log::subscribe(this, LogEvent);
    _SetWindowTitle(this, tr("Log Viewer"));
    d->lines = Log::option().lines();
    if (d->lines <= 0)
        d->lines = _Max<int>();

    d->proxy.setSourceModel(&d->model);
    d->ui.view->setModel(&d->proxy);

    const QFontMetrics fm(font());
    const int mw = fm.width('M'_q) * 12;
    d->ui.level->setFixedWidth(mw);
    d->ui.level->setMaximumHeight(fm.height() * 10);
    connect(d->ui.level, &QListWidget::itemChanged, this, [=] (QListWidgetItem *item) {
        const auto lv = item->data(Qt::UserRole).value<Log::Level>();
        d->proxy.level[lv] = item->checkState();
        d->proxy.invalidate();
    });

    d->ui.context->setFixedWidth(mw);
    connect(d->ui.context, &QListWidget::itemChanged, this, [=] () { d->syncContext(); });

    d->menu = new QMenu(this);
    auto copy = d->menu->addAction(tr("Copy"));
    auto all  = d->menu->addAction(tr("Select All"));

    copy->setShortcut(QKeySequence::Copy);
    addAction(copy);
    connect(copy, &QAction::triggered, this, [=] () {
        auto rows = d->ui.view->selectionModel()->selectedRows();
        std::sort(rows.begin(), rows.end(),
                  [&] (auto &lhs, auto &rhs) { return lhs.row() < rhs.row(); });
        QString text;
        for (auto &row : rows)
            text += row.data().toString() % '\n'_q;
        auto c = qApp->clipboard();
        c->setText(text);
    });

    all->setShortcut(QKeySequence::SelectAll);
    addAction(all);
    connect(all, &QAction::triggered, d->ui.view, &QTreeView::selectAll);
    connect(this, &QWidget::customContextMenuRequested,
            this, [=] () { d->menu->popup(QCursor::pos()); });
    d->syncContext();
    if (!geometry.isEmpty())
        setGeometry(geometry);
}

LogViewer::~LogViewer()
{
    Log::unsubscribe(this);
    QSettings s;
    s.beginGroup(u"LogViewer"_q);
    auto save = [&] (QListWidget *list) {
        s.beginWriteArray(list->objectName(), d->ui.context->count());
        for (int i = 0; i < list->count(); ++i) {
            auto item = list->item(i);
            s.setArrayIndex(i);
            s.setValue(u"name"_q, item->text());
            s.setValue(u"checked"_q, !!item->checkState());
        }
        s.endArray();
    };
    save(d->ui.level);
    save(d->ui.context);
    s.setValue(u"geometry"_q, geometry());
    s.endGroup();
    delete d;
}

auto LogViewer::customEvent(QEvent *ev) -> void
{
    if (ev->type() != LogEvent)
        return;
    LogEntry entry;
    QByteArray data;
    _TakeData(ev, entry.level, data);
    entry.message = QString::fromLocal8Bit(data);
    entry.message.chop(1);
    Q_ASSERT(entry.message.at(3) == '['_q);
    const int idx = entry.message.indexOf(']'_q, 4);
    if (idx < 0) {
        qDebug("Unkown logging context. Skip it.");
        return;
    }
    entry.context = entry.message.mid(4, idx -4 );
    d->model.append(entry);

    if (d->ctx.find(entry.context) == d->ctx.end()) {
        auto item = new QListWidgetItem;
        item->setText(entry.context);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(Qt::Checked);
        d->ui.context->addItem(item);
        d->ui.context->sortItems();
        d->ctx.insert(entry.context);
        d->syncContext();
    }

    if (d->model.rows() > d->lines)
        d->model.remove(0);
    d->ui.view->scrollToBottom();
}
