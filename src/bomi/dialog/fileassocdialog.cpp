#include "fileassocdialog.hpp"
#include "bbox.hpp"
#include "mbox.hpp"
#include "ui_fileassocdialog.h"
#include "os/os.hpp"
#include "misc/simplelistmodel.hpp"
#include "misc/objectstorage.hpp"

static const QStringList s_videoCommon = {
    u"asf"_q, u"avi"_q, u"flv"_q, u"ifo"_q,
    u"m2ts"_q, u"m4v"_q, u"mkv"_q, u"mov"_q,
    u"mp4"_q, u"mts"_q, u"ogg"_q, u"ogm"_q, u"qt"_q,
    u"rm"_q, u"rmvb"_q, u"trp"_q, u"tp"_q, u"ts"_q,
    u"vob"_q, u"webm"_q, u"wmv"_q
};

static const QStringList s_audioCommon = {
    u"aiff"_q, u"flac"_q,
    u"m4a"_q, u"mka"_q, u"mp2"_q, u"mp3"_q,
    u"ogg"_q, u"pcm"_q, u"wav"_q, u"wma"_q
};

auto _CommonExtList(ExtTypes ext) -> QStringList
{
    QStringList list;
    if (ext & VideoExt)
        list.append(s_videoCommon);
    if (ext & AudioExt)
        list.append(s_audioCommon);
    return list;
}

enum MediaSuffixFlag {
    NoFlag = 0, VideoSuffix = 1, AudioSuffix = 2, CommonSuffix = 4
};

class MediaSuffix {
public:
    MediaSuffix() { }
    MediaSuffix(const QString suffix)
        : m_suffix(suffix)
    {
        if (_IsSuffixOf(VideoExt, suffix)) {
            m_flags |= VideoSuffix;
            if (s_videoCommon.contains(suffix))
                m_flags |= CommonSuffix;
        }
        if (_IsSuffixOf(AudioExt, suffix)) {
            m_flags |= AudioSuffix;
            if (s_audioCommon.contains(suffix))
                m_flags |= CommonSuffix;
        }
        m_desc = _DescriptionForSuffix(m_suffix);
    }
    auto flags() const -> int { return m_flags; }
    auto suffix() const -> QString { return m_suffix; }
    auto description() const -> QString { return m_desc; }
private:
    QString m_suffix, m_desc;
    int m_flags = 0;
};

class MediaSuffixModel : public SimpleListModel<MediaSuffix> {
    Q_DECLARE_TR_FUNCTIONS(MediaSuffixModel)
public:
    enum Column { Suffix, Type, Desc, Columns };
    MediaSuffixModel(): Super(Columns) { setCheckable(Suffix, true); }
private:
    auto displayData(int row, int column) const -> QVariant final
    {
        switch (column) {
        case Suffix:
            return QString('.'_q % at(row).suffix());
        case Type: {
            switch (at(row).flags() & (AudioSuffix | VideoSuffix)) {
            case AudioSuffix:
                return tr("Audio");
            case VideoSuffix:
                return tr("Video");
            default:
                return tr("Media");
            }
        } case Desc:
            return at(row).description();
        default:
            return QVariant();
        }
    }
    auto header(int column) const -> QString final
    {
        switch (column) {
        case Suffix:
            return tr("Extension");
        case Type:
            return tr("Type");
        case Desc:
            return tr("Description");
        default:
            return QString();
        }
    }
};

struct FileAssocDialog::Data {
    Ui::FileAssocDialog ui;
    MediaSuffixModel model;
    ObjectStorage storage;
    auto extensions() const -> QStringList
    {
        QStringList ret;
        for (int i = 0; i < model.rows(); ++i) {
            if (model.isChecked(i, MediaSuffixModel::Suffix))
                ret.push_back(model.at(i).suffix());
        }
        return ret;
    }
};

FileAssocDialog::FileAssocDialog()
    : d(new Data)
{
    d->ui.setupUi(this);
    d->ui.view->setModel(&d->model);

    auto exts = _ExtList(VideoExt | AudioExt);
    exts.removeDuplicates();
    exts.sort();
    QList<MediaSuffix> list;
    list.reserve(exts.size());
    for (auto &ext : exts)
        list.push_back({ext});
    d->model.setList(list);

    auto plug = [=] (int flags, QPushButton *button) {
        connect(button, &QPushButton::clicked, this, [=] () {
            for (int i = 0; i < d->model.rows(); ++i) {
                if ((d->model.at(i).flags() & flags) == flags)
                    d->model.setChecked(i, MediaSuffixModel::Suffix, true);
            }
        });
    };
    plug(VideoSuffix | CommonSuffix, d->ui.common_video);
    plug(AudioSuffix | CommonSuffix, d->ui.common_audio);
    plug(VideoSuffix, d->ui.all_video);
    plug(AudioSuffix, d->ui.all_audio);
    connect(d->ui.clear, &QPushButton::clicked, this, [=] () {
        for (int i = 0; i < d->model.rows(); ++i)
            d->model.setChecked(i, MediaSuffixModel::Suffix, false);
    });

    d->ui.view->resizeColumnToContents(MediaSuffixModel::Suffix);
    d->ui.view->resizeColumnToContents(MediaSuffixModel::Type);

    d->storage.setObject(this, u"file-assoc-dialog"_q);
    d->storage.restore();

    auto win = [=] () { winId(); return windowHandle(); };
    auto alert = [=] () {
        MBox::error(this, tr("Permission Denied"),
                    tr("Failed to obtain privilege to access registry."),
                    { BBox::Ok });
    };

    connect(d->ui.assoc, &QPushButton::clicked, this, [=] () {
        if (!OS::associateFileTypes(win(), true, d->extensions()))
            alert();
    });
    connect(d->ui.unassoc, &QPushButton::clicked,
            this, [=] () { if (!OS::unassociateFileTypes(win(), true)) alert(); });
}

FileAssocDialog::~FileAssocDialog()
{
    d->storage.save();
    delete d;
}
