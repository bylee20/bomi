#ifndef PLAYLISTMODEL_HPP
#define PLAYLISTMODEL_HPP

#include "stdafx.hpp"
#include "playlist.hpp"
#include "simplelistmodel.hpp"

class Downloader;

class PlaylistModel : public SimpleListModel<Mrl, Playlist> {
    Q_OBJECT
    Q_PROPERTY(int loaded READ loaded NOTIFY loadedChanged)
    Q_PROPERTY(int count READ rows NOTIFY countChanged)
    Q_PROPERTY(QChar fillChar READ fillChar WRITE setFillChar)
    Q_PROPERTY(bool visible READ isVisible WRITE setVisible NOTIFY visibleChanged)
    Q_ENUMS(Role)
public:
    enum Role {NameRole = Qt::UserRole + 1, LocationRole, LoadedRole};
    PlaylistModel(QObject *parent = 0);
    ~PlaylistModel();
    auto roleData(int row, int column, int role) const -> QVariant final;
    bool open(const Mrl &mrl, const QString &enc = QString());
    bool open(const QByteArray &data, const QString &enc = QString());
    auto loaded() const -> int { return specialRow(); }
    int next() const {return loaded()+1;}
    int previous() const {return loaded()-1;}
    Mrl nextMrl() const {return value(loaded() + 1);}
    Mrl previousMrl() const {return value(loaded() - 1);}
    bool hasNext() const {return isValidRow(next());}
    bool hasPrevious() const {return isValidRow(previous());}
    Mrl loadedMrl() const {return value(loaded());}
    void setLoaded(const Mrl &mrl);
    QHash<int, QByteArray> roleNames() const override;
    Q_INVOKABLE void play(int row);
    Q_INVOKABLE QString name(int row) const {return value(row).displayName();}
    Q_INVOKABLE QString location(int row) const;
    Q_INVOKABLE QString number(int row) const;
    Q_INVOKABLE bool isLoaded(int row) const {return loaded() == row;}
    QChar fillChar() const {return m_fill;}
    void setFillChar(QChar c) {if (_Change(m_fill, c)) emit fillCharChanged();}
    bool isVisible() const { return m_visible; }
    void setVisible(bool visible) { if (_Change(m_visible, visible)) emit visibleChanged(m_visible); }
    void toggle() { setVisible(!isVisible()); }
    void setDownloader(Downloader *downloader);
public slots:
    void clear() { setList(Playlist()); }
    void playNext() {play(next());}
    void playPrevious() {play(previous());}
signals:
    void finished();
    void loadedChanged(int row);
    void contentWidthChanged();
    void playRequested(int row);
    void changeVisibilityRequested(bool visible);
    void fillCharChanged();
    void visibleChanged(bool visible);
    void countChanged();
private:
    friend class PlayEngine;
    void setLoaded(int row);
    QChar m_fill = QChar::Null;
    bool m_visible = false;
    Downloader *m_downloader = nullptr;
    QString m_enc;
};

#endif // PLAYLISTMODEL_HPP
