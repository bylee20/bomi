#ifndef PLAYLISTMODEL_HPP
#define PLAYLISTMODEL_HPP

#include "playlist.hpp"
#include "misc/simplelistmodel.hpp"

class Downloader;                       class EncodingInfo;

class PlaylistModel : public SimpleListModel<Mrl, Playlist> {
    Q_OBJECT
    Q_PROPERTY(int loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(int count READ rows NOTIFY countChanged)
    Q_PROPERTY(int length READ rows NOTIFY countChanged)
    Q_PROPERTY(int currentNumber READ currentNumber NOTIFY loadedChanged)
    Q_PROPERTY(int currentIndex READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(QChar fillChar READ fillChar WRITE setFillChar)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(int selected READ selected WRITE select NOTIFY selectedChanged)
    Q_PROPERTY(bool shuffled READ isShuffled NOTIFY shuffledChanged)
    Q_PROPERTY(bool repetitive READ repeat NOTIFY repeatChanged)
    Q_ENUMS(Role)
public:
    enum Role {NameRole = Qt::UserRole + 1, LocationRole, LoadedRole};
    PlaylistModel(QObject *parent = 0);
    ~PlaylistModel();

    auto roleData(int row, int column, int role) const -> QVariant final;
    auto loaded() const -> int { return specialRow(); }
    auto currentNumber() const -> int { return specialRow() + 1; }
    auto next() const -> int;
    auto previous() const -> int;
    auto checkNextMrl() const -> Mrl;
    auto nextMrl() const -> Mrl { return value(next()); }
    auto previousMrl() const -> Mrl { return value(previous()); }
    auto hasNext() const -> bool { return isValidRow(next()); }
    auto hasPrevious() const -> bool { return isValidRow(previous()); }
    auto loadedMrl() const -> Mrl { return value(loaded()); }
    auto roleNames() const -> QHash<int, QByteArray> override;
    auto fillChar() const -> QChar { return m_fill; }
    auto isVisible() const -> bool { return m_visible; }
    auto isShuffled() const -> bool { return m_shuffled; }
    auto selected() const -> int { return m_selected; }
    auto repeat() const -> bool { return m_repeat; }
    Q_INVOKABLE QString name(int row) const {return value(row).displayName();}
    Q_INVOKABLE QString location(int row) const;
    Q_INVOKABLE QString number(int row) const;
    Q_INVOKABLE bool isLoaded(int row) const {return loaded() == row;}

    auto open(const Mrl &mrl, const EncodingInfo &enc) -> void;
    Q_INVOKABLE void open(const QString &mrl);
    Q_INVOKABLE void open(const QString &mrl, const QString &enc);
    Q_INVOKABLE void add(const QString &mrl);
    auto setLoaded(const Mrl &mrl) -> void;
    auto setFillChar(QChar c) -> void;
    auto setVisible(bool visible) -> void;
    auto toggle() -> void { setVisible(!isVisible()); }
    auto setDownloader(Downloader *downloader) -> void;
    Q_INVOKABLE void clear() { setList(Playlist()); }
    Q_INVOKABLE void playNext() { play(next()); }
    Q_INVOKABLE void playPrevious() { play(previous()); }
    auto select(int row) -> void;
    auto setShuffled(bool shuffled) -> void;
    auto setRepeat(bool repeat) -> void;
    Q_INVOKABLE void play(int row);
signals:
    void finished() const;
    void loadedChanged(int row);
    void contentWidthChanged();
    void playRequested(int row);
    void changeVisibilityRequested(bool visible);
    void fillCharChanged();
    void visibleChanged(bool visible);
    void countChanged();
    void selectedChanged();
    void shuffledChanged();
    void repeatChanged();
    void nextChanged();
private:
    friend class PlayEngine;
    auto setLoaded(int row) -> void;
    auto shuffle() const -> void;
    QChar m_fill = QChar::Null;
    bool m_visible = false;
    int m_selected = -1;
    Downloader *m_downloader = nullptr;
    EncodingInfo m_enc;
    bool m_shuffled = false, m_repeat = false;
    mutable QVector<int> m_shuffledIdx;
};

inline auto PlaylistModel::setFillChar(QChar c) -> void
{ if (_Change(m_fill, c)) emit fillCharChanged(); }

inline auto PlaylistModel::setVisible(bool visible) -> void
{ if (_Change(m_visible, visible)) emit visibleChanged(m_visible); }

inline auto PlaylistModel::select(int row) -> void
{ if (isValidRow(row) && _Change(m_selected, row)) emit selectedChanged(); }

#endif // PLAYLISTMODEL_HPP
