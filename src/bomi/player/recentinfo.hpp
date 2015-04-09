#ifndef RECENTINFO_HPP
#define RECENTINFO_HPP

class Playlist;                         class Mrl;

class RecentInfo : public QObject {
    using Update = std::function<void(const QList<Mrl>&)>;
public:
    RecentInfo();
    ~RecentInfo();
    auto openList() const -> QList<Mrl>;
    auto stack(const Mrl &mrl) -> void;
    auto setLastPlaylist(const Playlist &list) -> void;
    auto setLastMrl(const Mrl &mrl) -> void;
    auto lastMrl() const -> Mrl;
    auto lastPlaylist() const -> Playlist;
    auto clear() -> void;
    auto setUpdateFunc(Update &&func) -> void;
private:
    auto save() const -> void;
    auto load() -> void;
    struct Data;
    Data *d;
};

#endif // RECENTINFO_HPP
