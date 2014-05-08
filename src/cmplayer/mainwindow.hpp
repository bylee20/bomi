#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "stdafx.hpp"

class Mrl;                              class PrefDialog;
class MainQuickView;                    class Playlist;
class Subtitle;                         class PlaylistModel;
class PlayEngine;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    MainWindow(const MainWindow &) = delete;
    MainWindow &operator = (const MainWindow &) = delete;
    ~MainWindow();
    auto openFromFileManager(const Mrl &mrl) -> void;
    auto isFullScreen() const -> bool;
    auto setFullScreen(bool full) -> void;
    auto wake() -> void;
    auto togglePlayPause() -> void;
    auto play() -> void;
    auto engine() const -> PlayEngine*;
    auto playlist() const -> PlaylistModel*;
    auto openMrl(const Mrl &mrl) -> void;
    auto openMrl(const Mrl &mrl, const QString &enc) -> void;
    auto exit() -> void;
private:
    auto applyPref() -> void;
    auto updateMrl(const Mrl &mrl) -> void;
    auto setVideoSize(double rate) -> void;
    auto clearSubtitleFiles() -> void;
    auto updateRecentActions(const QList<Mrl> &list) -> void;
    auto updateStaysOnTop() -> void;
    auto reloadSkin() -> void;
    auto checkWindowState() -> void;
    // init functions
    auto resetMoving() -> void;
    auto createView() -> MainQuickView*;
    auto connectMenus() -> void;
    auto connectObjects() -> void;
    auto restoreAppState() -> void;
    auto initContextMenu() -> void;
    auto initPlayEngine() -> void;
    auto initVideoRenderer() -> void;
    auto initUndoStack() -> void;
    auto initTimers() -> void;

    auto updateTitle() -> void;
    auto generatePlaylist(const Mrl &mrl) const -> Playlist;
    auto load(Subtitle &subtitle, const QString &fileName, const QString &encoding) -> bool;

    auto doVisibleAction(bool visible) -> void;
    auto showMessage(const QString &message, const bool *force = nullptr) -> void;
    auto showMessage(const QString &cmd, int value, const QString &unit, bool sign = false) -> void;
    auto showMessage(const QString &cmd, double value, const QString &unit, bool sign = false) -> void;
    auto showMessage(const QString &cmd, const QString &desc) -> void {showMessage(cmd + ": " + desc);}
    auto showMessage(const QString &cmd, bool value) -> void {showMessage(cmd, value ? tr("On") : tr("Off"));}
    auto appendSubFiles(const QStringList &files, bool checked, const QString &enc) -> void;
    auto changeEvent(QEvent *event) -> void;
    auto closeEvent(QCloseEvent *event) -> void;
    auto getStartTime(const Mrl &mrl) -> int;
    auto getCache(const Mrl &mrl) -> int;
    auto showEvent(QShowEvent *event) -> void;
    auto hideEvent(QHideEvent *event) -> void;
    auto onKeyPressEvent(QKeyEvent *event) -> void;
    auto onMouseMoveEvent(QMouseEvent *event) -> void;
    auto onMouseDoubleClickEvent(QMouseEvent *event) -> void;
    auto onMouseReleaseEvent(QMouseEvent *event) -> void;
    auto onMousePressEvent(QMouseEvent *event) -> void;
    auto onWheelEvent(QWheelEvent *event) -> void;
    auto dropEvent(QDropEvent *event) -> void;
    auto dragEnterEvent(QDragEnterEvent *event) -> void;
    auto resizeEvent(QResizeEvent *event) -> void;
    auto moveEvent(QMoveEvent *event) -> void;
    friend class MainQuickView;
    struct Data;
    Data *d;
};

inline auto MainWindow::wake() -> void {
    setVisible(true);
    raise();
    activateWindow();
}

#endif // MAINWINDOW_HPP
