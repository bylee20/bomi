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
//    auto setPlaylist(const Playlist &list) -> void;
//    auto play(const Mrl &mrl) -> void;
//    auto append(const Playlist &list) -> void;
//    auto merge(const Playlist &list) -> void;
//    PlaylistModel *model() const;
//    static PlaylistView &get() {Q_ASSERT(obj != 0); return *obj;}
//    static auto generatePlaylist(const Mrl &mrl) -> Playlist;
//public slots:
//    auto playNext() -> void;
//    auto playPrevious() -> void;
//    auto clear() -> void;
//signals:
//    auto finished() -> void;
//    auto playRequested(const Mrl &mrl) -> void;
//private slots:
//    auto updateCurrentMrl(const Mrl &mrl) -> void;
//    auto handleFinished() -> void;
//    auto showContextMenu(const QPoint &pos) -> void;
//    auto addFile() -> void;
//    auto addUrl() -> void;
//    auto erase() -> void;
//    auto up() -> void;
//    auto down() -> void;
//    auto open() -> void;
//    auto save() -> void;
//    auto handleDoubleClick(const QModelIndex &index) -> void;
//private:
//    auto move(bool up) -> void;
//    auto setCurrentIndex(int idx) -> void;
//    class Item;
//    class Table;
//    struct Data;
//    static PlaylistView *obj;
//    Data *d;
//};

//#endif // PLAYLISTVIEW_HPP
