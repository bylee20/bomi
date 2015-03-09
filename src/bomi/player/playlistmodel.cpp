#include "playlistmodel.hpp"
#include "misc/downloader.hpp"
#include "misc/encodinginfo.hpp"
#include <random>
#include <chrono>
#include <QQuickItem>

PlaylistModel::PlaylistModel(QObject *parent)
: Super(parent) {
    connect(this, &PlaylistModel::modelReset, this, &PlaylistModel::contentWidthChanged);
    connect(this, &PlaylistModel::rowsChanged, this, &PlaylistModel::countChanged);
    connect(this, &PlaylistModel::specialRowChanged, this, &PlaylistModel::loadedChanged);
    connect(this, &PlaylistModel::loadedChanged, this, &PlaylistModel::nextChanged);
}

PlaylistModel::~PlaylistModel() {}

auto PlaylistModel::next() const -> int
{
    if (isEmpty())
        return -1;
    if (!m_shuffled)
        return (loaded() >= rows() - 1 && m_repeat) ? 0 : loaded() + 1;
    if (m_shuffledIdx.size() != rows())
        shuffle();
    const int find = m_shuffledIdx.indexOf(loaded());
    if (find == -1)
        return m_shuffledIdx.first();
    if (find < m_shuffledIdx.size() - 1)
        return m_shuffledIdx[find + 1];
    if (!m_repeat)
        return -1;
    shuffle();
    return m_shuffledIdx.first();
}

auto PlaylistModel::previous() const -> int
{
    if (isEmpty())
        return -1;
    if (!m_shuffled)
        return (loaded() <= 0 && m_repeat) ? rows() - 1 : loaded() - 1;
    if (m_shuffledIdx.size() != rows())
        shuffle();
    const int find = m_shuffledIdx.indexOf(loaded());
    if (find == -1)
        return m_shuffledIdx.first();
    if (find > 0)
        return m_shuffledIdx[find - 1];
    if (!m_repeat)
        return -1;
    shuffle();
    return m_shuffledIdx.last();
}

auto PlaylistModel::shuffle() const -> void
{
    if (!m_shuffled) {
        m_shuffledIdx.clear();
        return;
    }
    m_shuffledIdx.resize(rows());
    for (int i = 0; i < m_shuffledIdx.size(); ++i)
        m_shuffledIdx[i] = i;
    if (m_shuffledIdx.size() < 2 || !m_shuffled)
        return;
    using namespace std; using std::chrono::system_clock;
    static const auto seed = system_clock::now().time_since_epoch().count();
    std::shuffle(m_shuffledIdx.begin(), m_shuffledIdx.end(),
                 default_random_engine(seed));
}

auto PlaylistModel::setShuffled(bool shuffled) -> void
{
    if (!_Change(m_shuffled, shuffled))
        return;
    emit shuffledChanged();
}

auto PlaylistModel::setRepeat(bool repeat) -> void
{
    if (_Change(m_repeat, repeat))
        emit repeatChanged();
}

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
        emit dataChanged(old);
    if (loaded() != -1)
        emit dataChanged(loaded());
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
        const auto suffix = m_downloader->suffixes().value(0);
        const auto type = Playlist::typeForSuffix(suffix);
        Playlist list;
        if (list.load(m_downloader->url(), &data, m_enc, type)) {
            setList(list);
            setVisible(true);
        }
    });
}

auto PlaylistModel::open(const QString &mrl) -> void
{
    open(Mrl(mrl), EncodingInfo::default_(EncodingInfo::Playlist));
}

auto PlaylistModel::open(const QString &mrl, const QString &enc) -> void
{
    open(Mrl(mrl), EncodingInfo::fromName(enc));
}

auto PlaylistModel::add(const QString &mrl) -> void
{
    append(Mrl(mrl));
}

auto PlaylistModel::open(const Mrl &mrl, const EncodingInfo &enc) -> void
{
    if (mrl.isLocalFile()) {
        setList({mrl, enc});
        setVisible(true);
    } else {
        if (m_downloader->isRunning())
            m_downloader->cancel();
        m_enc = enc;
        m_downloader->start(mrl.toString(), _ExtList(PlaylistExt));
    }
}

auto PlaylistModel::checkNextMrl() const -> Mrl
{
    auto mrl = nextMrl();
    if (mrl.isEmpty())
        emit finished();
    return mrl;
}
