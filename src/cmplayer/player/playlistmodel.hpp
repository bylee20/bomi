#ifndef PLAYLISTMODEL_HPP
#define PLAYLISTMODEL_HPP

#include "playlist.hpp"
#include "misc/simplelistmodel.hpp"

class Downloader;

class PlaylistModel : public SimpleListModel<Mrl, Playlist> {
    Q_OBJECT
    Q_PROPERTY(int loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(int count READ rows NOTIFY countChanged)
    Q_PROPERTY(QChar fillChar READ fillChar WRITE setFillChar)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_PROPERTY(int selected READ selected WRITE select NOTIFY selectedChanged)
    Q_ENUMS(Role)
public:
    enum Role {NameRole = Qt::UserRole + 1, LocationRole, LoadedRole};
    PlaylistModel(QObject *parent = 0);
    ~PlaylistModel();
    auto roleData(int row, int column, int role) const -> QVariant final;
    auto open(const Mrl &mrl, const QString &enc) -> void;
    auto loaded() const -> int { return specialRow(); }
    auto next() const -> int {return loaded()+1;}
    auto previous() const -> int {return loaded()-1;}
    auto nextMrl() const -> Mrl {return value(loaded() + 1);}
    auto previousMrl() const -> Mrl {return value(loaded() - 1);}
    auto hasNext() const -> bool {return isValidRow(next());}
    auto hasPrevious() const -> bool {return isValidRow(previous());}
    auto loadedMrl() const -> Mrl {return value(loaded());}
    auto setLoaded(const Mrl &mrl) -> void;
    auto roleNames() const -> QHash<int, QByteArray> override;
    auto fillChar() const -> QChar {return m_fill;}
    auto setFillChar(QChar c) -> void;
    auto setVisible(bool visible) -> void;
    auto isVisible() const -> bool { return m_visible; }
    auto toggle() -> void { setVisible(!isVisible()); }
    auto setDownloader(Downloader *downloader) -> void;
    auto clear() -> void { setList(Playlist()); }
    auto playNext() -> void {play(next());}
    auto playPrevious() -> void {play(previous());}
    auto select(int row) -> void;
    auto selected() const -> int { return m_selected; }
    Q_INVOKABLE void play(int row);
    Q_INVOKABLE QString name(int row) const {return value(row).displayName();}
    Q_INVOKABLE QString location(int row) const;
    Q_INVOKABLE QString number(int row) const;
    Q_INVOKABLE bool isLoaded(int row) const {return loaded() == row;}
signals:
    void finished();
    void loadedChanged(int row);
    void contentWidthChanged();
    void playRequested(int row);
    void changeVisibilityRequested(bool visible);
    void fillCharChanged();
    void visibleChanged(bool visible);
    void countChanged();
    void selectedChanged();
private:
    friend class PlayEngine;
    auto setLoaded(int row) -> void;
    QChar m_fill = QChar::Null;
    bool m_visible = false;
    int m_selected = -1;
    Downloader *m_downloader = nullptr;
    QString m_enc;
};

inline auto PlaylistModel::setFillChar(QChar c) -> void
{ if (_Change(m_fill, c)) emit fillCharChanged(); }

inline auto PlaylistModel::setVisible(bool visible) -> void
{ if (_Change(m_visible, visible)) emit visibleChanged(m_visible); }

inline auto PlaylistModel::select(int row) -> void
{ if (isValidRow(row) && _Change(m_selected, row)) emit selectedChanged(); }

#endif // PLAYLISTMODEL_HPP
