#include "subtitlefinddialog.hpp"
#include "mbox.hpp"
#include "player/mrl.hpp"
#include "misc/downloader.hpp"
#include "misc/simplelistmodel.hpp"
#include "misc/objectstorage.hpp"
#include "subtitle/opensubtitlesfinder.hpp"
#include "misc/locale.hpp"
#include "ui_subtitlefinddialog.h"
#include <QFileDialog>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QSortFilterProxyModel>

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

class LanguageFilterModel : public QSortFilterProxyModel {
public:
    auto filterAcceptsRow(int srow, const QModelIndex &) const -> bool final
    {
        auto m = static_cast<SubtitleLinkModel*>(sourceModel());
        return std::binary_search(langCodes.begin(), langCodes.end(), m->at(srow).langCode);
    }
    QStringList langCodes;
};

struct DownloadInfo {
    bool temp = false;
    QString fileName;
};

SIA language(const QString &langCode) -> QString
{
    const auto lang = Locale::isoToNativeName(langCode);
    return lang.isEmpty() ? langCode : lang;
}

struct SubtitleFindDialog::Data {
    SubtitleFindDialog *p = nullptr;
    Ui::SubtitleFindDialog ui;
    Downloader downloader;
    OpenSubtitlesFinder *finder = nullptr;
    Mrl pending;
    SubtitleLinkModel model;
    LanguageFilterModel proxy;
    QString fileName;
    QMap<QUrl, DownloadInfo> downloads;
    QMap<QString, QString> languages; // code, name
    QTemporaryDir temp;
    QFileInfo mediaFile;
    struct {
        bool preserve;
        QString format, fallback;
    } options;
    ObjectStorage storage;
    Load load;

    auto fillLanguage(QVector<SubtitleLink> &links) -> bool
    {
        auto find = [&] (const QString &lang) -> bool {
            for (int i = 0; i < ui.language->count(); ++i) {
                if (ui.language->data(i).toString() == lang)
                    return true;
            }
            return false;
        };

        auto size = ui.language->count();
        for (auto &link : links) {
            link.language = language(link.langCode);
            if (find(link.langCode))
                continue;
            ui.language->addItem(link.language, link.langCode.toLower());
            ui.language->setChecked(ui.language->count() - 1, true);
        }
        if (size != ui.language->count())
            ui.language->sortItems();
        return size != ui.language->count();
    }

    void updateState() {
        const bool ok = finder->isAvailable() && !downloader.isRunning();
        ui.open->setEnabled(ok);
        ui.find_file->setEnabled(ok);
        ui.find_info->setEnabled(ok);
        ui.find_name->setEnabled(ok);
        ui.language->setEnabled(ok);
        ui.get->setEnabled(ok);
        ui.view->setEnabled(ok);
        ui.prog->setValue(0);
        if (ok) {
            ui.prog->setRange(0, 1);
            if (!pending.isEmpty()) {
                auto mrl = pending;
                pending = Mrl();
                p->find(mrl);
            }
        } else
            ui.prog->setRange(0, 0);
        updateStateText();
    }
    auto updateStateText() -> void
    {
        auto text = [=] () {
            switch (finder->state()) {
            case OpenSubtitlesFinder::Connecting:
                return tr("Connecting...");
            case OpenSubtitlesFinder::Finding:
                return tr("Finding...");
            case OpenSubtitlesFinder::Unavailable:
                return tr("Unavailable");
            case OpenSubtitlesFinder::Error:
                return tr("Error");
            default:
                return tr("Available");
            }
        };
        ui.state->setText(text());
    }
    auto getNameToPreserve(const QString &subName) -> QString
    {
        Q_ASSERT(options.preserve);

        auto dir = mediaFile.dir();
        QTemporaryFile temp(dir.absoluteFilePath(u"XXXXXX"_q));
        if (!temp.open())
            dir = QDir(options.fallback);

        const QFileInfo remoteFile(subName);
        auto fileName = options.format;
        fileName.replace(u"%MEDIA_NAME%"_q, mediaFile.completeBaseName());
        fileName.replace(u"%SUBTITLE_NAME%"_q, remoteFile.completeBaseName());
        fileName.replace(u"%SUBTITLE_EXT%"_q, remoteFile.suffix());
        auto file = dir.absoluteFilePath(fileName);
        const QFileInfo info(file);
        if (info.exists()) {
            MBox mbox(p, MBox::Icon::Question, tr("Find Subtitle"),
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
                file = QFileDialog::getSaveFileName(p, tr("Save As..."), file, filter);
                if (file.isEmpty())
                    return QString();
                if (!file.endsWith(suffix))
                    file += suffix;
                file = _ToAbsFilePath(file);
            } case BBox::AcceptRole:
                break;
            default:
                return QString();
            }
        }
        return file;
    }
    auto writeData(const DownloadInfo &info, const QByteArray &data) -> void
    {
        if (data.isEmpty() || info.fileName.isEmpty())
            return;
        QScopedPointer<QFile> file;
        if (info.temp) {
            auto tfile = new QTemporaryFile(temp.path() % "/XXXXXX_"_a % info.fileName);
            tfile->setAutoRemove(false);
            tfile->open();
            file.reset(tfile);
        } else {
            file.reset(new QFile(info.fileName));
            if (!file->open(QFile::WriteOnly | QFile::Truncate)) {
                MBox::error(p, tr("Find Subtitle"),
                               tr("Cannot write file!") % '\n'_q % file->fileName(),
                               { BBox::Ok });
                return;
            }
        }
        file->write(data);
        file->close();
        if (load)
            load(file->fileName());
    }
};

SubtitleFindDialog::SubtitleFindDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
    d->p = this;
    d->ui.setupUi(this);

    d->proxy.setSourceModel(&d->model);
    d->proxy.setFilterKeyColumn(d->model.Language);
    d->proxy.setFilterRole(LangCodeRole);
    d->ui.view->setModel(&d->proxy);
    d->ui.view->header()->resizeSection(0, 100);
    d->ui.view->header()->resizeSection(1, 450);
    d->ui.view->header()->resizeSection(2, 150);
    d->finder = new OpenSubtitlesFinder;
    d->ui.open->set(PathButton::SingleFile, PathButton::Open);
    connect(&d->downloader, &Downloader::started, [this] () { d->updateState(); });
    connect(&d->downloader, &Downloader::progressed, [this] (qint64 written, qint64 total) {
        d->ui.prog->setRange(0, total);
        d->ui.prog->setValue(written);
    });
    connect(&d->downloader, &Downloader::finished, [this] () {
        auto it = d->downloads.find(d->downloader.url());
        Q_ASSERT(it != d->downloads.end());
        d->writeData(*it, _Uncompress(d->downloader.data()));
        d->downloads.erase(it);
        d->updateState();
    });
    connect(d->ui.open, &PathButton::fileSelected,
            [this] (const QString &file) { if (!file.isEmpty()) find(file); });
    connect(d->ui.find_file, &QPushButton::clicked, this, [=] () {
        if (d->mediaFile.exists())
            find(d->mediaFile.absoluteFilePath());
    });
    connect(d->ui.find_info, &QPushButton::clicked, this, [=] () {
        d->finder->find(d->ui.query->text(),
                        d->ui.season->value(), d->ui.episode->value());
    });
    connect(d->ui.find_name, &QPushButton::clicked,
            this, [=] () { d->finder->find(d->ui.tag->text()); });
    connect(d->ui.language, &CheckListWidget::checkedItemsChanged, [=] () {
        auto langs = d->ui.language->checkedData();
        d->proxy.langCodes.clear();
        for (auto &lang : langs)
            d->proxy.langCodes.push_back(lang.toString());
        d->proxy.langCodes.sort();
        d->proxy.invalidate();
    });
    connect(d->ui.get, &QPushButton::clicked, [this] () {
        const auto index = d->ui.view->currentIndex();
        if (!index.isValid())
            return;
        DownloadInfo info;
        info.temp = !d->options.preserve;
        info.fileName = index.data(FileNameRole).toString();
        if (d->options.preserve) {
            info.fileName = d->getNameToPreserve(info.fileName);
            if (info.fileName.isEmpty())
                return;
        }
        const auto url = d->ui.view->currentIndex().data(UrlRole).toUrl();
        if (d->downloader.start(url))
            d->downloads[d->downloader.url()] = info;
    });
    connect(d->finder, &OpenSubtitlesFinder::stateChanged, this,
            [this] () { d->updateState(); });
    connect(d->finder, &OpenSubtitlesFinder::found, this,
            [this] (QVector<SubtitleLink> links) {
        // Select first entry of list
        d->ui.view->setCurrentIndex(d->ui.view->indexAt(QPoint()));
        d->fillLanguage(links);
        d->model.setList(links);
        d->proxy.invalidate();
    });
    d->updateStateText();

    _SetWindowTitle(this, tr("Find Subtitle from OpenSubtitles.org"));
    d->storage.setObject(this, u"subtitle_find_dialog"_q);
    d->storage.add("language", [=] () { return d->ui.language->toVariant(CheckListData); },
                   [=] (auto &var) { d->ui.language->setFromVariant(var, CheckListData); });
    d->storage.restore();
    for (int i = 0; i < d->ui.language->count(); ++i) {
        auto item = d->ui.language->item(i);
        item->setText(language(d->ui.language->data(i).toString()));
    }
    d->ui.language->setHeaderCheckBox(d->ui.languageCheckBox);
    d->ui.view->setSortingEnabled(true);
}

SubtitleFindDialog::~SubtitleFindDialog() {
    d->storage.save();
    delete d->finder;
    delete d;
}

auto SubtitleFindDialog::setOptions(bool preserve, const QString &format,
                                    const QString &fb) -> void
{
    d->options.preserve = preserve;
    d->options.format = format;
    d->options.fallback = fb;
}

auto SubtitleFindDialog::find(const Mrl &mrl) -> void
{
    d->mediaFile = QFileInfo(mrl.toLocalFile());
    d->ui.fileName->setText(d->mediaFile.fileName());
    if (!d->finder->isAvailable()) {
        d->pending = mrl;
    } else if (!d->finder->find(mrl)) {
        const auto name = mrl.displayName();
        MBox::warn(this, tr("Find Subtitle"),
                   tr("Cannot find subtitles for %1.").arg(name),
                   { BBox::Ok });
    }
}

auto SubtitleFindDialog::setLoadFunc(Load &&load) -> void
{
    d->load = std::move(load);
}
