#include "playlistmodel.hpp"
#include "downloader.hpp"

void reg_playlist_model() {
    qmlRegisterType<PlaylistModel>();
}

PlaylistModel::PlaylistModel(QObject *parent)
: Super(parent) {
    connect(this, &PlaylistModel::modelReset,
            this, &PlaylistModel::contentWidthChanged);
    connect(this, &PlaylistModel::rowsChanged,
            this, &PlaylistModel::countChanged);
    connect(this, &PlaylistModel::specialRowChanged,
            this, &PlaylistModel::loadedChanged);
}

PlaylistModel::~PlaylistModel() {}

QHash<int, QByteArray> PlaylistModel::roleNames() const {
    QHash<int, QByteArray> names;
    names[NameRole] = "name";
    names[LocationRole] = "location";
    names[LoadedRole] = "isLoaded";
    return names;
}

QString PlaylistModel::number(int row) const {
    if (m_fill.isNull())
        return QString::number(row+1);
    int digits = 0, left = rows();
    do {
        ++digits;
        left = left/10;
    } while (left);
    return _N(row+1, 10, digits, m_fill);
}

void PlaylistModel::play(int row) {
    if (isValidRow(row))
        emit playRequested(row);
}

QString PlaylistModel::location(int row) const {
    auto mrl = value(row);
    return mrl.isLocalFile() ? mrl.toLocalFile() : mrl.toString();
}

auto PlaylistModel::roleData(int row, int, int role) const -> QVariant
{
    if (!isValidRow(row))
        return QVariant();
    if (role == NameRole) {
        return name(row);
    } else if (role == LocationRole) {
        return location(row);
    } else if (role == LoadedRole)
        return loaded() == row;
    return QVariant();
}

void PlaylistModel::setLoaded(int row) {
    if (!isValidRow(row))
        row = -1;
    if (loaded() == row)
        return;
    const int old = loaded();
    setSpecialRow(row);
    if (old != -1)
        emitDataChanged(old);
    if (loaded() != -1)
        emitDataChanged(loaded());
}

void PlaylistModel::setLoaded(const Mrl &mrl) {
    setLoaded(rowOf(mrl));
}

void PlaylistModel::setDownloader(Downloader *downloader) {
    m_downloader = downloader;
    connect(m_downloader, &Downloader::finished, this, [this] () {
        if (m_downloader->isCanceled())
            return;
        auto data = m_downloader->takeData();
        const auto type = Playlist::guessType(m_downloader->url().path());
        Playlist list;
        if (list.load(&data, m_enc, type)) {
            setList(list);
            setVisible(true);
        }
    });
}

bool PlaylistModel::open(const Mrl &mrl, const QString &enc) {
    if (!mrl.isPlaylist())
        return false;
    if (mrl.isLocalFile()) {
        setList({mrl, enc});
        setVisible(true);
    } else {
        if (m_downloader->isRunning())
            m_downloader->cancel();
        m_enc = enc;
        m_downloader->start(mrl.toString());
    }
    return true;
}
