#include "logviewer.hpp"
#include "logoption.hpp"
#include "dataevent.hpp"
#include "simplelistmodel.hpp"
#include "log.hpp"
#include "dialog/mbox.hpp"
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
    LogViewer *p = nullptr;
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

    auto level(const QListWidgetItem *item) -> Log::Level
    {
        return item->data(Qt::UserRole).value<Log::Level>();
    }

    auto setLevel(QListWidgetItem *item, Log::Level lv) -> void
    {
        item->setData(Qt::UserRole, QVariant::fromValue(lv));
    }

    auto chceck(QListWidgetItem *item, bool c) -> void
    {
        item->setCheckState(c ? Qt::Checked : Qt::Unchecked);
    }

    auto setFilterChecked(QCheckBox *c, QListWidget *l) -> void
    {
        int checked = 0;
        for (auto i = 0; i < l->count(); ++i) {
            if (l->item(i)->checkState())
                ++checked;
        }
        QSignalBlocker sb(c);
        if (!checked)
            c->setCheckState(Qt::Unchecked);
        else if (checked < l->count())
            c->setCheckState(Qt::PartiallyChecked);
        else
            c->setCheckState(Qt::Checked);
    }

    auto syncLevel() -> void
    {
        for (auto i = 0; i < ui.level->count(); ++i) {
            const auto item = ui.level->item(i);
            proxy.level[level(item)] = item->checkState();
        }
        proxy.invalidate();
    }

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

    auto newContext(const QString &name, bool checked) -> QListWidgetItem*
    {
        if (ctx.find(name) != ctx.end())
            return nullptr;
        auto item = newItem(name, ui.context);
        chceck(item, checked);
        ctx.insert(name);
        return item;
    }
    auto restore() -> QRect
    {
        QSettings s;
        s.beginGroup(u"LogViewer"_q);
        int len = s.beginReadArray(ui.context->objectName());
        for (int i = 0; i < len; ++i) {
            s.setArrayIndex(i);
            auto &&name = s.value(u"name"_q).toString();
            auto checked = s.value(u"checked"_q).toBool();
            newContext(name, checked);
        }
        s.endArray();

        QMap<Log::Level, bool> levels;
        len = s.beginReadArray(ui.level->objectName());
        for (int i = 0; i < len; ++i) {
            s.setArrayIndex(i);
            auto level = Log::level(s.value(u"name"_q).toString());
            levels[level] = s.value(u"checked"_q).toBool();
        }
        s.endArray();
        auto geometry = s.value(u"geometry"_q).toRect();
        ui.autoscroll->setChecked(s.value(u"autoscroll"_q, true).toBool());
        s.endGroup();

        const auto max = Log::option().level(LogOutput::Viewer);
        for (int lv = 1; lv <= Log::Trace; ++lv) {
            auto level = (Log::Level)lv;
            auto item = newItem(Log::name(level), ui.level);
            setLevel(item, level);
            if (level > max)
                item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
            chceck(item, levels.contains(level) ? levels[level] : true);
        }
        syncContext();
        syncLevel();
        setFilterChecked(ui.contextCheck, ui.context);
        setFilterChecked(ui.levelCheck, ui.level);
        return geometry;
    }

    auto createMenu() -> void
    {
        menu = new QMenu(p);
        auto copy = menu->addAction(tr("Copy"));
        auto all  = menu->addAction(tr("Select All"));

        copy->setShortcut(QKeySequence::Copy);
        p->addAction(copy);
        connect(copy, &QAction::triggered, p, [=] () {
            auto rows = ui.view->selectionModel()->selectedRows();
            std::sort(rows.begin(), rows.end(),
                      [&] (auto &lhs, auto &rhs) { return lhs.row() < rhs.row(); });
            QString text;
            for (auto &row : rows)
                text += row.data().toString() % '\n'_q;
            auto c = qApp->clipboard();
            c->setText(text);
        });

        all->setShortcut(QKeySequence::SelectAll);
        p->addAction(all);
        connect(all, &QAction::triggered, ui.view, &QTreeView::selectAll);
        connect(ui.view, &QWidget::customContextMenuRequested,
                menu, [=] () { menu->popup(QCursor::pos()); });
    }
};

LogViewer::LogViewer(QWidget *parent)
    : QDialog(parent), d(new Data)
{
    d->p = this;
    std::fill(d->proxy.level.begin(), d->proxy.level.end(), true);
    d->ui.setupUi(this);
    d->ui.view->viewport()->setStyleSheet("background-color: rgb(0, 0, 0);"_a);
    _SetWindowTitle(this, tr("Log Viewer"));

    const auto geometry = d->restore();

    d->lines = Log::subscribe(this, LogEvent);
    d->proxy.setSourceModel(&d->model);
    d->ui.view->setModel(&d->proxy);
    d->createMenu();

    const QFontMetrics fm(font());
    const int mw = fm.width('M'_q) * 12;
    d->ui.level->setFixedWidth(mw);
    d->ui.level->setMaximumHeight(fm.height() * 10);
    connect(d->ui.level, &QListWidget::itemChanged, this, [=] (auto item) {
        d->proxy.level[d->level(item)] = item->checkState();
        d->proxy.invalidate();
        d->setFilterChecked(d->ui.levelCheck, d->ui.level);
    });

    d->ui.context->setFixedWidth(mw);
    connect(d->ui.context, &QListWidget::itemChanged, this, [=] () {
        d->syncContext();
        d->setFilterChecked(d->ui.contextCheck, d->ui.context);
    });

#define PLUG_FILTER(l, s) \
    connect(d->ui.l##Check, &QCheckBox::stateChanged, this, [=] (int checked) { \
        QSignalBlocker sb(d->ui.l); \
        for (auto i = 0; i < d->ui.l->count(); ++i) \
            d->chceck(d->ui.l->item(i), checked); \
        d->s(); \
    });
    PLUG_FILTER(level, syncLevel);
    PLUG_FILTER(context, syncContext);

    connect(d->ui.clear, &QPushButton::clicked, this, [=] () {
        if (MBox::ask(this, tr("Log Viewer"),
                      tr("Do you want remove all logs and contexts?"),
                      { BBox::Ok, BBox::Cancel }) == BBox::Ok) {
            d->model.clear();
            d->ui.context->clear();
            d->ctx.clear();
            d->syncContext();
        }
    });

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
    s.setValue(u"autoscroll"_q, d->ui.autoscroll->isChecked());
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

    if (d->newContext(entry.context, true)) {
        d->ui.context->sortItems();
        d->syncContext();
    }

    if (d->model.rows() > d->lines)
        d->model.remove(0);
    if (d->ui.autoscroll->isChecked())
        d->ui.view->scrollToBottom();
}
