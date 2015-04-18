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
    auto openFromFileManager(const Mrl &mrl, const QString &sub = QString()) -> void;
    auto setSubtitle(const QString &sub) -> void;
    auto isFullScreen() const -> bool;
    auto isFrameless() const -> bool;
    auto setFullScreen(bool full, bool updateLastGeometry = true) -> void;
    auto wake() -> void;
    auto togglePlayPause() -> void;
    auto togglePlayStop() -> void;
    auto play() -> void;
    auto pause() -> void;
    auto engine() const -> PlayEngine*;
    auto qmlEngine() const -> QQmlEngine* { return QQuickView::engine(); }
    auto playlist() const -> PlaylistModel*;
    auto exit() -> void;
    auto isSceneGraphInitialized() const -> bool;
    auto adapter() const -> OS::WindowAdapter*;
signals:
    void cursorChanged(const QCursor &cursor);
    void fullscreenChanged(bool fs);
    void framelessChanged(bool frameless);
    void sceneGraphInitialized();
private slots:
    void postInitialize();
    void setupSkinPlayer();
private:
    auto eventFilter(QObject *o, QEvent *e) -> bool final;
    auto event(QEvent *event) -> bool final;
    auto showEvent(QShowEvent *event) -> void final;
    auto hideEvent(QHideEvent *event) -> void final;
    auto resizeEvent(QResizeEvent *event) -> void final;
    auto moveEvent(QMoveEvent *event) -> void final;
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

#endif // MAINWINDOW_HPP
