#include "mainwindow_p.hpp"
#include "app.hpp"
#include "misc/trayicon.hpp"
#include "dialog/mbox.hpp"
#include "quick/appobject.hpp"

//DECLARE_LOG_CONTEXT(Main)

#ifdef Q_OS_MAC
void qt_mac_set_dock_menu(QMenu *menu);
#include <Carbon/Carbon.h>
#endif

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent, Qt::Window)
    , d(new Data(this))
{
    AppObject::setEngine(&d->engine);
    AppObject::setHistory(&d->history);
    AppObject::setPlaylist(&d->playlist);
    AppObject::setDownloader(&d->downloader);
    AppObject::setTheme(&d->theme);
    d->playlist.setDownloader(&d->downloader);

    d->engine.run();
    d->initWidget();
    d->initContextMenu();
    d->initTimers();
    d->initItems();
    d->initEngine();

    d->dontShowMsg = true;
    d->connectMenus();
    d->syncWithState();

    d->playlist.setVisible(d->as.playlist_visible);
    d->history.setVisible(d->as.history_visible);

    if (d->as.win_size.isValid()) {
        auto screen = d->screenSize();
        const auto x = screen.width() * d->as.win_pos.x();
        const auto y = screen.height() * d->as.win_pos.y();
        move(x, y);
        resize(d->as.win_size);
    }

    d->vr.setEffects(d->as.state.video_effects);
    auto effectGroup = d->menu(u"video"_q)(u"filter"_q).g();
    for (auto &item : VideoEffectInfo::items()) {
        if (d->as.state.video_effects & item.value)
            effectGroup->setChecked(QVariant::fromValue(item.value), true);
    }
    d->menu(u"tool"_q)[u"auto-exit"_q]->setChecked(d->as.auto_exit);

    d->dontShowMsg = false;

    d->playlist.setList(d->recent.lastPlaylist());
    if (!d->recent.lastMrl().isEmpty()) {
        d->load(d->recent.lastMrl(), false);
        d->setOpen(d->recent.lastMrl());
    }
    d->updateRecentActions(d->recent.openList());

    d->winState = d->prevWinState = windowState();

    connect(&cApp, &App::commitDataRequest, [this] () { d->commitData(); });
    connect(&cApp, &App::saveStateRequest, [this] (QSessionManager &session) {
        session.setRestartHint(QSessionManager::RestartIfRunning);
    });

    d->undo = new QUndoStack(this);
    auto undo = d->menu(u"tool"_q)[u"undo"_q];
    auto redo = d->menu(u"tool"_q)[u"redo"_q];
    connect(d->undo, &QUndoStack::canUndoChanged, undo, &QAction::setEnabled);
    connect(d->undo, &QUndoStack::canRedoChanged, redo, &QAction::setEnabled);
    connect(undo, &QAction::triggered, d->undo, &QUndoStack::undo);
    connect(redo, &QAction::triggered, d->undo, &QUndoStack::redo);
    d->menu(u"tool"_q)[u"undo"_q]->setEnabled(d->undo->canUndo());
    d->menu(u"tool"_q)[u"redo"_q]->setEnabled(d->undo->canRedo());

    if (!VideoRenderer::supportsHighQualityRendering()) {
        auto &video = d->menu(u"video"_q);
        auto key = _L(DitheringInfo::typeKey());
        video(key).g(key)->find(Dithering::Fruit)->setDisabled(true);
        key = _L(InterpolatorTypeInfo::typeKey());
        auto disable = [&] (const QString &subkey) {
            for (auto a : video(subkey).g(key)->actions()) {
                if (a->data().toInt() != InterpolatorType::Bilinear)
                    a->setDisabled(true);
            }
        };
        disable(u"chroma-upscaler"_q);
        disable(u"interpolator"_q);
    }
    if (TrayIcon::isAvailable()) {
        d->tray = new TrayIcon(cApp.defaultIcon(), this);
        connect(d->tray, &TrayIcon::activated,
                this, [this] (TrayIcon::ActivationReason reason) {
            if (reason == TrayIcon::Trigger)
                setVisible(!isVisible());
            else if (reason == TrayIcon::Context)
                d->contextMenu.exec(QCursor::pos());
            else if (reason == TrayIcon::Show)
                setVisible(true);
            else if (reason == TrayIcon::Quit)
                exit();
        });
        d->tray->setVisible(d->preferences.enable_system_tray);
    }
}

MainWindow::~MainWindow() {
    cApp.setMprisActivated(false);
    d->view->engine()->clearComponentCache();
    exit();
    delete d->view;
    delete d;
}

auto MainWindow::openFromFileManager(const Mrl &mrl) -> void
{
    if (mrl.isLocalFile() && _IsSuffixOf(PlaylistExt, mrl.suffix()))
        d->playlist.open(mrl, QString());
    else {
        const auto mode = d->pref().open_media_from_file_manager;
        d->openWith(mode, QList<Mrl>() << mrl);
    }
}

auto MainWindow::engine() const -> PlayEngine*
{
    return &d->engine;
}

auto MainWindow::playlist() const -> PlaylistModel*
{
    return &d->playlist;
}

auto MainWindow::exit() -> void
{
    static bool done = false;
    if (!done) {
        cApp.setScreensaverDisabled(false);
        d->commitData();
        d->vr.setOverlay(nullptr);
        cApp.quit();
        done = true;
    }
}

auto MainWindow::play() -> void
{
    if (d->stateChanging)
        return;
    if (d->pref().pause_to_play_next_image && d->engine.mrl().isImage())
        d->menu(u"play"_q)[u"next"_q]->trigger();
    else {
        const auto state = d->engine.state();
        switch (state) {
        case PlayEngine::Playing:
        case PlayEngine::Loading:
            break;
        case PlayEngine::Paused:
            d->engine.unpause();
            break;
        default:
            d->load(d->engine.mrl());
            break;
        }
    }
}

auto MainWindow::togglePlayPause() -> void
{
    if (d->stateChanging)
        return;
    if (d->pref().pause_to_play_next_image && d->engine.mrl().isImage())
        d->menu(u"play"_q)[u"next"_q]->trigger();
    else {
        const auto state = d->engine.state();
        switch (state) {
        case PlayEngine::Playing:
        case PlayEngine::Loading:
            d->engine.pause();
            break;
        default:
            play();
            break;
        }
    }
}

auto MainWindow::isSceneGraphInitialized() const -> bool
{
    return d->sgInit;
}

auto MainWindow::setFullScreen(bool full) -> void
{
    d->dontPause = true;
    if (full != d->fullScreen) {
        d->fullScreen = full;
        d->updateWindowPosState();
#ifdef Q_OS_MAC
        if (!d->pref().lion_style_fullscreen) {
            static Qt::WindowFlags flags = windowFlags();
            static QRect geometry;
            if (full) {
                auto desktop = cApp.desktop();
                const int screen = desktop->screenNumber(this);
                if (screen >= 0) {
                    flags = windowFlags();
                    geometry = this->geometry();
                    setWindowFlags(flags | Qt::FramelessWindowHint);
                    SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);
                    show();
                    setGeometry(QRect(QPoint(0, 0),
                                      desktop->screenGeometry(this).size()));
                }
            } else {
                setWindowFlags(flags);
                setGeometry(geometry);
                SetSystemUIMode(kUIModeNormal, 0);
            }
            d->checkWindowState(d->winState);
            d->updateTitle();
            d->updateStaysOnTop();
        } else
#endif
        {
            constexpr auto minfull = Qt::WindowMinimized | Qt::WindowFullScreen;
            setWindowState(full ? Qt::WindowFullScreen
                                : (d->prevWinState & ~minfull));
        }
        emit fullscreenChanged(d->fullScreen);
    }
    d->dontPause = false;
}

auto MainWindow::isFullScreen() const -> bool
{
    return d->fullScreen || QWidget::isFullScreen();
}

auto MainWindow::resetMoving() -> void
{
    if (d->moving) {
        d->moving = false;
        d->prevPos = QPoint();
    }
}

using MsBh = MouseBehavior;

auto MainWindow::onMouseMoveEvent(QMouseEvent *event) -> void
{
    QWidget::mouseMoveEvent(event);
    d->cancelToHideCursor();
    const bool full = isFullScreen();
    const auto gpos = event->globalPos();
    if (full) {
        resetMoving();
    } else {
        if (d->moving) {
            move(this->pos() + gpos - d->prevPos);
            d->prevPos = gpos;
        }
    }
    d->readyToHideCursor();
    d->engine.sendMouseMove(d->vr.mapToVideo(event->pos()));
}

auto MainWindow::onMouseDoubleClickEvent(QMouseEvent *event) -> void
{
    QWidget::mouseDoubleClickEvent(event);
    if (event->buttons() & Qt::LeftButton) {
        const auto act = d->menu.action(d->actionId(MsBh::DoubleClick, event));
        if (!act)
            return;
#ifdef Q_OS_MAC
        if (action == d->menu(u"window"_q)[u"full"_q])
            QTimer::singleShot(300, action, SLOT(trigger()));
        else
#endif
        d->trigger(act);
    }
}

auto MainWindow::onMouseReleaseEvent(QMouseEvent *event) -> void
{
    QWidget::mouseReleaseEvent(event);
    const auto rect = geometry();
    if (d->pressedButton == event->button()
            && rect.contains(event->localPos().toPoint() + rect.topLeft())) {
        const auto mb = MouseBehaviorInfo::fromData(d->pressedButton);
        if (mb != MsBh::DoubleClick)
            d->trigger(d->menu.action(d->actionId(mb, event)));
    }
}

auto MainWindow::onMousePressEvent(QMouseEvent *event) -> void
{
    QWidget::mousePressEvent(event);
    d->pressedButton = Qt::NoButton;
    bool showContextMenu = false;
    switch (event->button()) {
    case Qt::LeftButton:
        if (isFullScreen())
            break;
        d->moving = true;
        d->prevPos = event->globalPos();
        break;
    case Qt::MiddleButton:    case Qt::ExtraButton1:
    case Qt::ExtraButton2:    case Qt::ExtraButton3:
    case Qt::ExtraButton4:    case Qt::ExtraButton5:
    case Qt::ExtraButton6:    case Qt::ExtraButton7:
    case Qt::ExtraButton8:    case Qt::ExtraButton9:
        d->pressedButton = event->button();
        break;
    case Qt::RightButton:
        showContextMenu = true;
        break;
    default:
        break;
    }
    if (showContextMenu)
        d->contextMenu.exec(QCursor::pos());
    else
        d->contextMenu.hide();
    d->engine.sendMouseClick(d->vr.mapToVideo(event->pos()));
}

auto MainWindow::onWheelEvent(QWheelEvent *ev) -> void
{
    QWidget::wheelEvent(ev);
    const auto delta = ev->delta();
    if (delta) {
        const bool up = d->pref().invert_wheel ? delta < 0 : delta > 0;
        const auto id = d->actionId(up ? MsBh::ScrollUp : MsBh::ScrollDown, ev);
        d->trigger(d->menu.action(id));
        ev->accept();
    }
}

auto MainWindow::dragEnterEvent(QDragEnterEvent *event) -> void
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

auto MainWindow::dropEvent(QDropEvent *event) -> void
{
    const auto urls = event->mimeData()->urls();
    if (!event->mimeData()->hasUrls() || urls.isEmpty())
        return;
    Playlist playlist;
    QStringList subList;
    for (int i=0; i<urls.size(); ++i) {
        const auto suffix = QFileInfo(urls[i].path()).suffix().toLower();
        if (_IsSuffixOf(PlaylistExt, suffix)) {
            Playlist list;
            list.load(urls[i]);
            playlist += list;
        } else if (_IsSuffixOf(SubtitleExt, suffix))
            subList << urls[i].toLocalFile();
        else if (_IsSuffixOf(VideoExt, suffix) || _IsSuffixOf(AudioExt, suffix))
            playlist.append(urls[i]);
    }
    if (!playlist.isEmpty()) {
        d->openWith(d->pref().open_media_by_drag_and_drop, playlist);
    } else if (!subList.isEmpty())
        d->appendSubFiles(subList, true, d->pref().sub_enc);
}

auto MainWindow::resizeEvent(QResizeEvent *event) -> void
{
    QWidget::resizeEvent(event);
    if (!d->fullScreen)
        d->updateWindowSizeState();
}

auto MainWindow::onKeyPressEvent(QKeyEvent *event) -> void
{
    QWidget::keyPressEvent(event);
    constexpr int modMask = Qt::SHIFT | Qt::CTRL | Qt::ALT | Qt::META;
    const QKeySequence shortcut(event->key() + (event->modifiers() & modMask));
    d->trigger(RootMenu::instance().action(shortcut));
    event->accept();
}

auto MainWindow::showEvent(QShowEvent *event) -> void
{
    QWidget::showEvent(event);
    d->doVisibleAction(true);
}

auto MainWindow::hideEvent(QHideEvent *event) -> void
{
    QWidget::hideEvent(event);
    d->doVisibleAction(false);
}

auto MainWindow::changeEvent(QEvent *ev) -> void
{
    QWidget::changeEvent(ev);
    if (ev->type() == QEvent::WindowStateChange) {
        auto event = static_cast<QWindowStateChangeEvent*>(ev);
        d->checkWindowState(event->oldState());
    }
}

auto MainWindow::closeEvent(QCloseEvent *event) -> void
{
    QWidget::closeEvent(event);
#ifndef Q_OS_MAC
    if (d->tray && d->pref().enable_system_tray
            && d->pref().hide_rather_close) {
        hide();
        if (d->as.ask_system_tray) {
            MBox mbox(this);
            mbox.setIcon(MBox::Icon::Information);
            mbox.setTitle(tr("System Tray Icon"));
            mbox.setText(tr("CMPlayer will be running in "
                "the system tray when the window closed."));
            mbox.setInformativeText(
                tr("You can change this behavior in the preferences. "
                    "If you want to exit CMPlayer, please use 'Exit' menu."));
            mbox.addButton(BBox::Ok);
            mbox.checkBox()->setText(tr("Do not display this message again"));
            mbox.exec();
            d->as.ask_system_tray = !mbox.checkBox()->isChecked();
        }
        event->ignore();
    } else {
        event->accept();
        exit();
    }
#else
    event->accept();
#endif
}

auto MainWindow::moveEvent(QMoveEvent *event) -> void
{
    QWidget::moveEvent(event);
    if (!d->fullScreen)
        d->updateWindowPosState();
}
