#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include <QQuickView>

class OpenGLLogger;
class Mrl;                              class PlaylistModel;
class PlayEngine;                       class QQuickItem;
namespace OS { class WindowAdapter; }

class MainWindow : public QQuickView {
    Q_OBJECT
public:
    MainWindow();
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
    auto adapter() const -> OS::WindowAdapter*;
signals:
    void cursorChanged(const QCursor &cursor);
    void fullscreenChanged(bool fs);
    void sceneGraphInitialized();
private slots:
    void postInitialize();
    void setupSkinPlayer();
private:
    auto event(QEvent *event) -> bool final;
    auto showEvent(QShowEvent *event) -> void final;
    auto hideEvent(QHideEvent *event) -> void final;
    auto resizeEvent(QResizeEvent *event) -> void final;
    auto customEvent(QEvent *event) -> void final;
    auto keyPressEvent(QKeyEvent *event) -> void final;
    auto mouseDoubleClickEvent(QMouseEvent *event) -> void final;
    auto mousePressEvent(QMouseEvent *event) -> void final;
    auto mouseReleaseEvent(QMouseEvent *event) -> void final;
    auto mouseMoveEvent(QMouseEvent *event) -> void final;
    auto wheelEvent(QWheelEvent *event) -> void final;
    struct Data;
    bool m_sgInit = false;
    OpenGLLogger *m_glLogger;
    PlayEngine *m_engine;
    Data *d;


};

inline auto MainWindow::wake() -> void {
    setVisible(true);
    raise();
    requestActivate();
}

#endif // MAINWINDOW_HPP
