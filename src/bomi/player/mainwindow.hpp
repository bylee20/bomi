#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

class Mrl;                              class PlaylistModel;
class PlayEngine;                       class QQuickItem;
namespace OS { class WindowAdapter; }

class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    auto openFromFileManager(const Mrl &mrl) -> void;
    auto isFullScreen() const -> bool;
    auto setFullScreen(bool full, bool updateLastGeometry = true) -> void;
    auto wake() -> void;
    auto togglePlayPause() -> void;
    auto play() -> void;
    auto engine() const -> PlayEngine*;
    auto playlist() const -> PlaylistModel*;
    auto exit() -> void;
    auto isSceneGraphInitialized() const -> bool;
    auto resetMoving() -> void;
    auto screen() const -> QScreen*;
    auto adapter() const -> OS::WindowAdapter*;
signals:
    void cursorChanged(const QCursor &cursor);
    void fullscreenChanged(bool fs);
    void sceneGraphInitialized();
private slots:
    void postInitialize();
    void setupSkinPlayer();
private:
    auto showEvent(QShowEvent *event) -> void;
    auto hideEvent(QHideEvent *event) -> void;
    auto changeEvent(QEvent *event) -> void;
    auto closeEvent(QCloseEvent *event) -> void;
    auto dropEvent(QDropEvent *event) -> void;
    auto dragEnterEvent(QDragEnterEvent *event) -> void;
    auto resizeEvent(QResizeEvent *event) -> void;
    auto moveEvent(QMoveEvent *event) -> void;
    auto customEvent(QEvent *event) -> void final;
    auto onKeyPressEvent(QKeyEvent *event) -> void;
    auto onMouseMoveEvent(QMouseEvent *event) -> void;
    auto onMouseDoubleClickEvent(QMouseEvent *event) -> void;
    auto onMouseReleaseEvent(QMouseEvent *event) -> void;
    auto onMousePressEvent(QMouseEvent *event) -> void;
    auto onWheelEvent(QWheelEvent *event) -> void;
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
