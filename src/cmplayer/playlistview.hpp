//#ifndef PLAYLISTVIEW_HPP
//#define PLAYLISTVIEW_HPP

//#include "dialogs.hpp"

//class PlayEngine;        class Mrl;
//class Playlist;            class QModelIndex;
//class PlaylistModel;

//class PlaylistView : public ToggleDialog {
//    Q_OBJECT
//public:
//    PlaylistView(PlayEngine *engine, QWidget *parent);
//    ~PlaylistView();
//    void load(const Mrl &mrl, const QString &enc = QString());
//    const Playlist &playlist() const;
//    void setPlaylist(const Playlist &list);
//    void play(const Mrl &mrl);
//    void append(const Playlist &list);
//    void merge(const Playlist &list);
//    PlaylistModel *model() const;
//    static PlaylistView &get() {Q_ASSERT(obj != 0); return *obj;}
//    static Playlist generatePlaylist(const Mrl &mrl);
//public slots:
//    void playNext();
//    void playPrevious();
//    void clear();
//signals:
//    void finished();
//    void playRequested(const Mrl &mrl);
//private slots:
//    void updateCurrentMrl(const Mrl &mrl);
//    void handleFinished();
//    void showContextMenu(const QPoint &pos);
//    void addFile();
//    void addUrl();
//    void erase();
//    void up();
//    void down();
//    void open();
//    void save();
//    void handleDoubleClick(const QModelIndex &index);
//private:
//    void move(bool up);
//    void setCurrentIndex(int idx);
//    class Item;
//    class Table;
//    struct Data;
//    static PlaylistView *obj;
//    Data *d;
//};

//#endif // PLAYLISTVIEW_HPP
