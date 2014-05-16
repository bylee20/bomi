#ifndef APPOBJECT_HPP
#define APPOBJECT_HPP

#include "stdafx.hpp"

class PlayEngine;                       class HistoryModel;
class PlaylistModel;                    class TopLevelItem;
class Downloader;                       class ThemeObject;

class AppObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(PlayEngine *engine READ engine CONSTANT FINAL)
    Q_PROPERTY(HistoryModel *history READ history CONSTANT FINAL)
    Q_PROPERTY(PlaylistModel *playlist READ playlist CONSTANT FINAL)
    Q_PROPERTY(TopLevelItem *topLevelItem READ topLevelItem CONSTANT FINAL)
    Q_PROPERTY(Downloader *download READ downloader CONSTANT FINAL)
    Q_PROPERTY(ThemeObject *theme READ theme CONSTANT FINAL)
public:
    PlayEngine *engine() const { return s.engine; }
    HistoryModel *history() const { return s.history; }
    PlaylistModel *playlist() const { return s.playlist; }
    TopLevelItem *topLevelItem() const { return s.top; }
    Downloader *downloader() const { return s.down; }
    ThemeObject *theme() const { return s.theme; }
    static auto setTheme(ThemeObject *theme) -> void { s.theme = theme; }
    static auto setEngine(PlayEngine *engine) -> void { s.engine = engine; }
    static auto setHistory(HistoryModel *history) -> void { s.history = history; }
    static auto setPlaylist(PlaylistModel *pl) -> void { s.playlist = pl; }
    static auto setTopLevelItem(TopLevelItem *top) -> void { s.top = top; }
    static auto setDownloader(Downloader *down) -> void { s.down = down; }
private:
    struct StaticData {
        PlayEngine *engine = nullptr;
        HistoryModel *history = nullptr;
        PlaylistModel *playlist = nullptr;
        TopLevelItem *top = nullptr;
        Downloader *down = nullptr;
        ThemeObject *theme = nullptr;
    };
    static StaticData s;
};

#endif // APPOBJECT_HPP
