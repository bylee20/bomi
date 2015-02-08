#include "subtitlefinddialog.hpp"
#include "mbox.hpp"
#include "player/mrl.hpp"
#include "misc/downloader.hpp"
#include "misc/simplelistmodel.hpp"
#include "subtitle/opensubtitlesfinder.hpp"
#include "ui_subtitlefinddialog.h"

enum CustomRole {
    UrlRole = Qt::UserRole + 1,
    FileNameRole,
    LangCodeRole
};

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
        if (role == LangCodeRole) return at(row).langCode;
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
    QMap<QString, QString> languages; // code, name
    QString langCode;
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

SubtitleFindDialog::SubtitleFindDialog(const bool save, QWidget *parent)
: QDialog(parent), d(new Data) {
    d->p = this;
    d->ui.setupUi(this);
    d->proxy.setSourceModel(&d->model);
    d->proxy.setFilterKeyColumn(d->model.Language);
    d->proxy.setFilterRole(LangCodeRole);
    d->ui.directory->hide();
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
    connect(&d->downloader, &Downloader::finished, [this, save] () {
        auto it = d->downloads.find(d->downloader.url());
        Q_ASSERT(it != d->downloads.end());
        auto data = _Uncompress(d->downloader.data());
        if (!data.isEmpty()) {
            QFile nativeFile(*it);
            auto writable = nativeFile.open(QFile::WriteOnly | QFile::Truncate);
            QString path;
            if (save && writable) {
                nativeFile.write(data);
                nativeFile.close();
                path = nativeFile.fileName();
            } else {
                QTemporaryFile tempFile;
                tempFile.setAutoRemove(false);
                tempFile.open();
                tempFile.write(data);
                tempFile.close();
                path = tempFile.fileName();
            }
            emit loadRequested(path);
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
        d->langCode = i > 0 ? d->ui.language->itemData(i).toString() : QString();
        d->proxy.setFilterFixedString(d->langCode);
    });
    connect(d->ui.get, &QPushButton::clicked, [this] () {
        const auto index = d->ui.view->currentIndex();
        if (!index.isValid())
            return;
        auto dir = QDir(d->ui.directory->text());
        auto fileName = d->ui.fileName->text() + u"-"_q + index.data(FileNameRole).toString();
        auto file = dir.absoluteFilePath(fileName);
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
                file = _ToAbsFilePath(file);
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
        auto prev = d->langCode;
        d->model.setList(links);
        // Select first entry of list
        d->ui.view->setCurrentIndex(d->ui.view->indexAt(QPoint()));
        d->languages.clear();
        for (auto &it : links)
            d->languages[it.langCode] = it.language;
        d->ui.language->clear();
        d->ui.language->addItem(tr("All"), QString());
        d->ui.language->insertSeparator(1);
        for (auto it = d->languages.cbegin(); it != d->languages.cend(); ++it)
            d->ui.language->addItem(it.value(), it.key());
        setSelectedLangCode(prev);
    });
    d->updateState();
}

SubtitleFindDialog::~SubtitleFindDialog() {
    delete d->finder;
    delete d;
}

auto SubtitleFindDialog::find(const Mrl &mrl) -> void
{
    d->ui.fileName->setText(mrl.fileName());
    d->ui.directory->setText(QFileInfo(mrl.toLocalFile()).absolutePath());
    if (!d->finder->isAvailable()) {
        d->pending = mrl;
    } else if (!d->finder->find(mrl)) {
        const auto name = mrl.displayName();
        MBox::warn(this, tr("Find Subtitle"),
                   tr("Cannot find subtitles for %1.").arg(name),
                   { BBox::Ok });
    }
}

auto SubtitleFindDialog::selectedLangCode() const -> QString
{
    return d->langCode;
}

auto SubtitleFindDialog::setSelectedLangCode(const QString &langCode) -> void
{
    d->langCode = langCode;
    const int idx = d->ui.language->findData(langCode);
    if (d->ui.language->count() > 0)
        d->ui.language->setCurrentIndex(idx < 1 ? 0 : idx);
    d->proxy.setFilterFixedString(d->ui.language->currentData().toString());
}
