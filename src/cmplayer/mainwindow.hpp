#ifndef MAINWINDOW_HPP
#define MAINWINDOW_HPP

#include "stdafx.hpp"

class Mrl;                             class PrefDialog;
class MainView;                        class Playlist;
class Subtitle;                        class PlaylistModel;
class PlayEngine;                      class TopLevelItem;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    MainWindow(const MainWindow &) = delete;
    MainWindow &operator = (const MainWindow &) = delete;
    ~MainWindow();
    void openFromFileManager(const Mrl &mrl);
    bool isFullScreen() const;
    void setFullScreen(bool full);
    void wake() {
        setVisible(true);
        raise();
        activateWindow();
    }
    void togglePlayPause();
    void play();
    PlayEngine *engine() const;
    PlaylistModel *playlist() const;
    TopLevelItem *topLevelItem() const;
public slots:
    void openMrl(const Mrl &mrl);
    void openMrl(const Mrl &mrl, const QString &enc);
    void exit();
private slots:
    void applyPref();
    void updateMrl(const Mrl &mrl);
    void setVideoSize(double rate);
    void clearSubtitleFiles();
    void updateRecentActions(const QList<Mrl> &list);
    void updateStaysOnTop();
    void reloadSkin();
    void checkWindowState();
private:
    // init functions
    void resetMoving();
    MainView *createView();
    void connectMenus();
    void connectObjects();
    void restoreAppState();
    void initContextMenu();
    void initPlayEngine();
    void initVideoRenderer();
    void initUndoStack();
    void initTimers();

    void updateTitle();
    Playlist generatePlaylist(const Mrl &mrl) const;
    bool load(Subtitle &subtitle, const QString &fileName, const QString &encoding);

    void doVisibleAction(bool visible);
    void showMessage(const QString &message, const bool *force = nullptr);
    void showMessage(const QString &cmd, int value, const QString &unit, bool sign = false);
    void showMessage(const QString &cmd, double value, const QString &unit, bool sign = false);
    void showMessage(const QString &cmd, const QString &desc) {showMessage(cmd + ": " + desc);}
    void showMessage(const QString &cmd, bool value) {showMessage(cmd, value ? tr("On") : tr("Off"));}
    void appendSubFiles(const QStringList &files, bool checked, const QString &enc);
    void changeEvent(QEvent *event);
    void closeEvent(QCloseEvent *event);
    int getStartTime(const Mrl &mrl);
    int getCache(const Mrl &mrl);
    void showEvent(QShowEvent *event);
    void hideEvent(QHideEvent *event);
    void onKeyPressEvent(QKeyEvent *event);
    void onMouseMoveEvent(QMouseEvent *event);
    void onMouseDoubleClickEvent(QMouseEvent *event);
    void onMouseReleaseEvent(QMouseEvent *event);
    void onMousePressEvent(QMouseEvent *event);
    void onWheelEvent(QWheelEvent *event);
    void dropEvent(QDropEvent *event);
    void dragEnterEvent(QDragEnterEvent *event);
    void resizeEvent(QResizeEvent *event);
    void moveEvent(QMoveEvent *event);
    friend class MainView;
    struct Data;
    Data *d;
    class VolumeCmd;    class SpeedCmd;        class AspectRatioCmd;
    class CropCmd;
};

class MainView : public QQuickView {
    Q_OBJECT
public:
    MainView(MainWindow *main): QQuickView(main->windowHandle()), m_main(main) {
        setColor(Qt::black);
        setResizeMode(QQuickView::SizeRootObjectToView);
        main->installEventFilter(this);
    }
private:
    bool eventFilter(QObject *obj, QEvent *ev) {
        if (obj != m_main)
            return false;
        switch (ev->type()) {
        case QEvent::KeyPress:
            keyPressEvent(static_cast<QKeyEvent*>(ev));
            break;
//        case QEvent::KeyRelease:
//            keyReleaseEvent(static_cast<QKeyEvent*>(ev));
//            break;
        case QEvent::MouseButtonPress:
            mousePressEvent(static_cast<QMouseEvent*>(ev));
            break;
        case QEvent::MouseButtonRelease:
            mouseReleaseEvent(static_cast<QMouseEvent*>(ev));
            break;
        case QEvent::MouseMove:
            mouseMoveEvent(static_cast<QMouseEvent*>(ev));
            break;
        case QEvent::MouseButtonDblClick:
            mouseDoubleClickEvent(static_cast<QMouseEvent*>(ev));
            break;
        case QEvent::Wheel:
            wheelEvent(static_cast<QWheelEvent*>(ev));
            break;
        default:
            return false;
        }
        return true;
    }
    void keyPressEvent(QKeyEvent *event);
    void mouseDoubleClickEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event) {
        m_main->resetMoving();
        event->setAccepted(false);
        QQuickView::mouseReleaseEvent(event);
        if (!event->isAccepted())
            m_main->onMouseReleaseEvent(event);
    }
    void mouseMoveEvent(QMouseEvent *event) {
        event->setAccepted(false);
        QQuickView::mouseMoveEvent(event);
        m_main->onMouseMoveEvent(event);
    }

    void wheelEvent(QWheelEvent *event) {
        event->setAccepted(false);
        QQuickView::wheelEvent(event);
        if (!event->isAccepted())
            m_main->onWheelEvent(event);
    }
    bool event(QEvent *event) {
        if (QQuickView::event(event))
            return true;
        if (event->type() == QEvent::DragMove) {
            m_main->dragEnterEvent(static_cast<QDragEnterEvent*>(event));
        } else if (event->type() == QEvent::Drop) {
            m_main->dropEvent(static_cast<QDropEvent*>(event));
        } else
            return false;
        return true;
    }
    MainWindow *m_main = nullptr;
};


#endif // MAINWINDOW_HPP
