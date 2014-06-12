#include "subtitlefinddialog.hpp"
#include "mbox.hpp"
#include "player/mrl.hpp"
#include "misc/downloader.hpp"
#include "misc/simplelistmodel.hpp"
#include "subtitle/opensubtitlesfinder.hpp"
#include "ui_subtitlefinddialog.h"

static constexpr int UrlRole = Qt::UserRole + 1;
static constexpr int FileNameRole = UrlRole + 1;

class SubtitleLinkModel
        : public SimpleListModel<SubtitleLink, QVector<SubtitleLink>> {
public:
    enum Column { Language, FileName, Date, ColumnCount };
    SubtitleLinkModel(QObject *parent = nullptr)
        : SimpleListModel(ColumnCount, parent) { }
    auto header(int column) const -> QString {
        switch (column) {
        case Language: return qApp->translate("SubtitleLinkModel", "Language");
        case FileName: return qApp->translate("SubtitleLinkModel", "File Name");
        case Date:     return qApp->translate("SubtitleLinkModel", "Date");
        default:       return QString();
        }
    }
    auto roleData(int row, int /*column*/, int role) const -> QVariant {
        if (role == UrlRole)      return at(row).url;
        if (role == FileNameRole) return at(row).fileName;
        return QVariant();
    }
    auto displayData(int row, int column) const -> QVariant {
        switch (column) {
        case Language: return at(row).language;
        case FileName: return at(row).fileName;
        case Date:     return at(row).date;
        default:       return QVariant();
        }
    }
};

struct SubtitleFindDialog::Data {
    SubtitleFindDialog *p = nullptr;
    Ui::SubtitleFindDialog ui;
    Downloader downloader;
    OpenSubtitlesFinder *finder = nullptr;
    Mrl pending;
    SubtitleLinkModel model;
    QSortFilterProxyModel proxy;
    QString fileName;
    QMap<QUrl, QString> downloads;
    void updateState() {
        const bool ok = finder->isAvailable() && !downloader.isRunning();
        ui.open->setEnabled(ok);
        ui.state->setEnabled(ok);
        ui.language->setEnabled(ok);
        ui.get->setEnabled(ok);
        ui.view->setEnabled(ok);
        if (ok) {
            ui.prog->setMaximum(1);
            ui.prog->setValue(0);
            if (!pending.isEmpty()) {
                auto mrl = pending;
                pending = Mrl();
                p->find(mrl);
            }
        } else {
            ui.prog->setMaximum(0);
        }
        switch (finder->state()) {
        case OpenSubtitlesFinder::Connecting:
            ui.state->setText(tr("Connecting..."));
            break;
        case OpenSubtitlesFinder::Finding:
            ui.state->setText(tr("Finding..."));
            break;
        default:
            ui.state->setText(tr("Find"));
            break;
        }
    }
};

SubtitleFindDialog::SubtitleFindDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
    d->p = this;
    d->ui.setupUi(this);
    d->proxy.setSourceModel(&d->model);
    d->proxy.setFilterKeyColumn(d->model.Language);
    d->ui.view->setModel(&d->proxy);
    d->ui.view->header()->resizeSection(0, 100);
    d->ui.view->header()->resizeSection(1, 450);
    d->ui.view->header()->resizeSection(2, 150);
    d->finder = new OpenSubtitlesFinder;
    connect(&d->downloader, &Downloader::started, [this] () { d->updateState(); });
    connect(&d->downloader, &Downloader::progressed, [this] (qint64 written, qint64 total) {
        d->ui.prog->setMaximum(total);
        d->ui.prog->setValue(written);
    });
    connect(&d->downloader, &Downloader::finished, [this] () {
        auto it = d->downloads.find(d->downloader.url());
        Q_ASSERT(it != d->downloads.end());
        auto data = _Uncompress(d->downloader.data());
        if (!data.isEmpty()) {
            QFile file(*it);
            file.open(QFile::WriteOnly | QFile::Truncate);
            file.write(data);
            file.close();
            emit loadRequested(file.fileName());
        }
        d->downloads.erase(it);
        d->updateState();
    });
    connect(d->ui.open, &QPushButton::clicked, [this] () {
        auto file = _GetOpenFile(this, tr("Open"), VideoExt);
        if (!file.isEmpty())
            find(QUrl::fromLocalFile(file));
    });
    Signal<QComboBox, int> changed = &QComboBox::currentIndexChanged;
    connect(d->ui.language, changed, [this] (int i) {
        const auto text = i > 0 ? d->ui.language->itemText(i) : QString();
        d->proxy.setFilterFixedString(text);
    });
    connect(d->ui.get, &QPushButton::clicked, [this] () {
        const auto index = d->ui.view->currentIndex();
        if (!index.isValid())
            return;
        const auto dir = QFileInfo(d->ui.file->text()).dir();
        auto file = dir.absoluteFilePath(index.data(FileNameRole).toString());
        const QFileInfo info(file);
        if (info.exists()) {
            MBox mbox(this, MBox::Icon::Question, tr("Find Subtitle"),
                      tr("A file with the same name already exists. "
                         "Do you want overwrite it?"));
            mbox.addButton(tr("Overwrite"), BBox::AcceptRole);
            mbox.addButton(tr("Save as..."), BBox::ActionRole);
            mbox.addButton(BBox::Cancel);
            mbox.exec();
            switch (mbox.clickedRole()) {
            case BBox::ActionRole: {
                const QString suffix = '.'_q % info.suffix();
                const QString filter = tr("Subtitle Files")
                                       % " (*"_a % suffix % ')'_q;
                file = QFileDialog::getSaveFileName(this, tr("Save As..."),
                                                    file, filter);
                if (file.isEmpty())
                    return;
                if (!file.endsWith(suffix))
                    file += suffix;
                file = QFileInfo(file).absoluteFilePath();
            } case BBox::AcceptRole:
                break;
            default:
                return;
            }
        }
        const auto url = d->ui.view->currentIndex().data(UrlRole).toUrl();
        if (d->downloader.start(url))
            d->downloads[d->downloader.url()] = file;
    });
    connect(d->finder, &OpenSubtitlesFinder::stateChanged,
            [this] () { d->updateState(); });
    connect(d->finder, &OpenSubtitlesFinder::found,
            [this] (const QVector<SubtitleLink> &links) {
        d->model.setList(links);
        std::set<QString> set;
        for (auto &it : links)
            set.insert(it.language);
        d->ui.language->clear();
        d->ui.language->addItem(tr("All"));
        d->ui.language->insertSeparator(1);
        for (auto &it : set)
            d->ui.language->addItem(it);
    });
    d->updateState();
}

SubtitleFindDialog::~SubtitleFindDialog() {
    delete d->finder;
    delete d;
}

auto SubtitleFindDialog::find(const Mrl &mrl) -> void
{
    d->ui.file->setText(mrl.toLocalFile());
    if (!d->finder->isAvailable()) {
        d->pending = mrl;
    } else if (!d->finder->find(mrl)) {
        const auto name = mrl.displayName();
        MBox::warn(this, tr("Find Subtitle"),
                   tr("Cannot find subtitles for %1.").arg(name),
                   { BBox::Ok });
    }
}
