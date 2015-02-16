#include "subtitlefinddialog.hpp"
#include "mbox.hpp"
#include "player/mrl.hpp"
#include "misc/downloader.hpp"
#include "misc/simplelistmodel.hpp"
#include "misc/objectstorage.hpp"
#include "subtitle/opensubtitlesfinder.hpp"
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

struct DownloadInfo {
    bool temp = false;
    QString fileName;
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
    QMap<QUrl, DownloadInfo> downloads;
    QMap<QString, QString> languages; // code, name
    QString langCode;
    QTemporaryDir temp;
    QFileInfo mediaFile;
    struct {
        bool preserve;
        QString format, fallback;
    } options;
    ObjectStorage storage;
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
        emit p->loadRequested(file->fileName());
    }
    auto setLangCode(const QString &code) -> void
    {
        langCode = code;
        proxy.setFilterFixedString(code);
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
    connect(&d->downloader, &Downloader::started, [this] () { d->updateState(); });
    connect(&d->downloader, &Downloader::progressed, [this] (qint64 written, qint64 total) {
        d->ui.prog->setMaximum(total);
        d->ui.prog->setValue(written);
    });
    connect(&d->downloader, &Downloader::finished, [this] () {
        auto it = d->downloads.find(d->downloader.url());
        Q_ASSERT(it != d->downloads.end());
        d->writeData(*it, _Uncompress(d->downloader.data()));
        d->downloads.erase(it);
        d->updateState();
    });
    connect(d->ui.open, &QPushButton::clicked, [this] () {
        auto file = _GetOpenFile(this, tr("Open"), VideoExt);
        if (!file.isEmpty())
            find(QUrl::fromLocalFile(file));
    });
    connect(SIGNAL_VT(d->ui.language, currentIndexChanged, int), [this] (int i) {
        if (d->ui.language->count() == 0)
            return;
        d->setLangCode(i > 1 ? d->ui.language->itemData(i).toString() : QString());
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
    connect(d->finder, &OpenSubtitlesFinder::stateChanged,
            [this] () { d->updateState(); });
    connect(d->finder, &OpenSubtitlesFinder::found,
            [this] (const QVector<SubtitleLink> &links) {
        auto prev = d->langCode;
        d->model.setList(links);
        // Select first entry of list
        d->ui.view->setCurrentIndex(d->ui.view->indexAt(QPoint()));
        d->languages.clear();
        d->ui.language->clear();
        if (!links.isEmpty()) {
            for (auto &it : links)
                d->languages[it.langCode] = it.language;
            d->ui.language->addItem(tr("All"), QString());
            d->ui.language->insertSeparator(1);
            for (auto it = d->languages.cbegin(); it != d->languages.cend(); ++it)
                d->ui.language->addItem(it.value(), it.key());
            const int idx = qMax(0, d->ui.language->findData(prev));
            if (idx > 0)
                d->ui.language->setCurrentIndex(idx == 1 ? 0 : idx);
            else
                d->setLangCode(QString());
        }
    });
    d->updateState();

    _SetWindowTitle(this, tr("Find Subtitle from OpenSubtitles.org"));
    d->storage.setObject(this, u"subtitle_find_dialog"_q, true);
    d->storage.add("language", &d->langCode);
    d->storage.restore();
}

SubtitleFindDialog::~SubtitleFindDialog() {
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
