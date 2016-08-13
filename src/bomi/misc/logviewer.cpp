#include "logviewer.hpp"
#include "logoption.hpp"
#include "dataevent.hpp"
#include "simplelistmodel.hpp"
#include "log.hpp"
#include "dialog/mbox.hpp"
#include "ui_logviewer.h"
#include "misc/objectstorage.hpp"
#include <QMenu>
#include <QClipboard>
#include <set>
#include <QSortFilterProxyModel>
#include <QFileDialog>

static const int LogEvent = QEvent::User + 10;

struct LogEntry {
    Log::Level level = Log::Off;
    QString context, message;
};


class EntryFilterWidget : public QWidget {
public:
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
    QStringList ctx;
    QVector<bool> level = QVector<bool>(Log::Trace + 1);
};

struct LogViewer::Data {
    LogViewer *p = nullptr;
    Ui::LogViewer ui;
    std::set<QString> ctx;
    LogFilterModel proxy;
    LogEntryModel model;
    int lines = 0;
    bool stop = false;
    QMenu *menu = nullptr;
    ObjectStorage storage;

    auto syncContext() -> void
    {
        proxy.ctx = ui.context->checkedTexts();
        proxy.ctx.sort();
        proxy.invalidate();
    }

    auto newContext(const QString &name, bool checked) -> bool
    {
        if (ctx.find(name) != ctx.end())
            return false;
        ui.context->addItem(name);
        ui.context->setChecked(ui.context->count()-1, checked);
        ctx.insert(name);
        return true;
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
    d->ui.setupUi(this);
    std::fill(d->proxy.level.begin(), d->proxy.level.end(), true);
    for (int i = 0; i < d->proxy.level.size(); ++i) {
        d->ui.level->addItem(Log::name((Log::Level)i), i);
        d->ui.level->setChecked(i, true);
    }
    d->ui.level->item(0)->setHidden(true);
    d->ui.view->viewport()->setStyleSheet("background-color: rgb(0, 0, 0);"_a);
    _SetWindowTitle(this, tr("Log Viewer"));

    d->lines = Log::subscribe(this, LogEvent);
    d->proxy.setSourceModel(&d->model);
    d->ui.view->setModel(&d->proxy);
    d->createMenu();

    const QFontMetrics fm(font());
    const int mw = fm.width('M'_q) * 12;
    d->ui.level->setFixedWidth(mw);
    d->ui.level->setMaximumHeight(fm.height() * 10);
    d->ui.context->setFixedWidth(mw);

    d->storage.setObject(this, u"log-viewer"_q);
    d->storage.add("autoscroll", d->ui.autoscroll, "checked");
    d->storage.add(d->ui.level->objectName().toLatin1(),
                   [=] () { return d->ui.level->toVariant(0); },
                   [=] (auto &var) { d->ui.level->setFromVariant(var, 0, false); });
    d->storage.add(d->ui.context->objectName().toLatin1(),
                   [=] () { return d->ui.context->toVariant(CheckListText); },
                   [=] (auto &var) { d->ui.context->setFromVariant(var, CheckListText); });
    d->storage.restore();
    for (auto i = 0; i < d->ui.context->count(); ++i)
        d->ctx.insert(d->ui.context->item(i)->text());
    d->ui.context->sortItems();
    d->syncContext();

    d->ui.level->setHeaderCheckBox(d->ui.levelCheck);
    d->ui.context->setHeaderCheckBox(d->ui.contextCheck);

    connect(d->ui.level, &CheckListWidget::checkedItemsChanged, this, [=] () {
        d->proxy.level = d->ui.level->checkedStates().toVector();
        d->proxy.invalidate();
    });
    connect(d->ui.context, &CheckListWidget::checkedItemsChanged,
            this, [=] () { d->syncContext(); });
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
    connect(d->ui.save, &QPushButton::clicked, this, [=] () {
        auto path = _LastOpenPath(u"log-viewer"_q);
        path = QFileDialog::getSaveFileName(this, tr("Save Log"), path);
        if (path.isEmpty())
            return;
        _SetLastOpenPath(path, u"log-viewer"_q);

        QFile file(path);
        if (file.open(QFile::WriteOnly | QFile::Truncate)) {
            QTextStream out(&file);
            out.setCodec("UTF-8");
            for (int i = 0; i < d->model.size(); ++i)
                out << d->model.at(i).message << endl;
        } else
            MBox::error(this, tr("Log Viewer"), tr("Failed to open file to save."), {BBox::Ok});
    });
}

LogViewer::~LogViewer()
{
    d->stop = true;
    Log::unsubscribe(this);
    d->storage.save();
    delete d;
}

auto LogViewer::customEvent(QEvent *ev) -> void
{
    if (ev->type() != LogEvent)
        return;
    if (d->stop)
        return;
    LogEntry entry;
    _TakeData(ev, entry.level, entry.message);
    Q_ASSERT(entry.message.at(3) == '['_q);
    const int idx = entry.message.indexOf(']'_q, 4);
    if (idx < 0) {
        qDebug("Unknown logging context. Skip it.");
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

auto LogViewer::showEvent(QShowEvent *event) -> void
{
    QDialog::showEvent(event);
    const auto lv = Log::option().level(LogOutput::Viewer);
    if (lv <= Log::Off) {
        d->ui.max->setText(tr("Log viewer is disabled. Turn it on in preferences and restart bomi."));
    } else
        d->ui.max->setText(tr("Maximum log level: %1").arg(Log::name(lv)));
}
