#include "playlistmodel.hpp"
#include "misc/downloader.hpp"

auto reg_playlist_model() -> void {
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

auto PlaylistModel::roleNames() const -> QHash<int, QByteArray>
{
    QHash<int, QByteArray> names;
    names[NameRole] = "name";
    names[LocationRole] = "location";
    names[LoadedRole] = "isLoaded";
    return names;
}

auto PlaylistModel::number(int row) const -> QString
{
    if (m_fill.isNull())
        return QString::number(row+1);
    int digits = 0, left = rows();
    do {
        ++digits;
        left = left/10;
    } while (left);
    return _N(row+1, 10, digits, m_fill);
}

auto PlaylistModel::play(int row) -> void
{
    if (isValidRow(row))
        emit playRequested(row);
}

auto PlaylistModel::location(int row) const -> QString
{
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

auto PlaylistModel::setLoaded(int row) -> void
{
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

auto PlaylistModel::setLoaded(const Mrl &mrl) -> void
{
    setLoaded(rowOf(mrl));
}

auto PlaylistModel::setDownloader(Downloader *downloader) -> void
{
    m_downloader = downloader;
    connect(m_downloader, &Downloader::finished, this, [this] () {
        if (m_downloader->isCanceled())
            return;
        auto data = m_downloader->takeData();
        const auto type = Playlist::guessType(m_downloader->url().path());
        Playlist list;
        if (list.load(m_downloader->url(), &data, m_enc, type)) {
            setList(list);
            setVisible(true);
        }
    });
}

auto PlaylistModel::open(const Mrl &mrl, const QString &enc) -> bool
{
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
