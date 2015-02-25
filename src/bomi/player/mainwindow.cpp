#include "mainwindow_p.hpp"
#include "app.hpp"
#include "misc/trayicon.hpp"
#include "dialog/mbox.hpp"
#include "quick/appobject.hpp"

//DECLARE_LOG_CONTEXT(Main)

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent, Qt::Window), d(new Data)
{
    d->p = this;
    d->view = new MainQuickView(this);
    d->pref.initialize();
    d->pref.load();
    d->undo.setActive(false);
    d->logViewer = new LogViewer(this);
    d->adapter = OS::adapter(this);

    AppObject::setEngine(&d->e);
    AppObject::setHistory(&d->history);
    AppObject::setPlaylist(&d->playlist);
    AppObject::setDownloader(&d->downloader);
    AppObject::setTheme(&d->theme);
    AppObject::setWindow(this);

    d->playlist.setDownloader(&d->downloader);
    d->e.setHistory(&d->history);
    d->e.setYouTube(&d->youtube);
    d->e.setYle(&d->yle);
    d->e.run();

    d->initWindow();
    d->initContextMenu();
    d->initItems();
    d->initTray();

    d->plugEngine();
    d->plugMenu();

    d->restoreState();

    d->undo.setActive(true);
    QTimer::singleShot(1, this, SLOT(postInitialize()));
}

MainWindow::~MainWindow() {
    cApp.setMprisActivated(false);
    d->view->clear();
    exit();
    _Delete(d->view);
    d->deleteDialogs();
    delete d;
}

auto MainWindow::postInitialize() -> void
{
    d->as.restoreWindowGeometry(this);
    OS::setImeEnabled(windowHandle(), false);
    OS::setImeEnabled(d->view, false);
    d->applyPref();
    cApp.runCommands();
    d->noMessage = false;
}

auto MainWindow::setupSkinPlayer() -> void
{
    d->player = d->view->rootObject()->property("player").value<QQuickItem*>();
    if (!d->player)
        return;
    d->e.screen()->setParentItem(d->player);
    d->e.screen()->setWidth(d->player->width());
    d->e.screen()->setHeight(d->player->height());
    d->player->setProperty("screen", QVariant::fromValue(d->e.screen()));
    if (auto item = d->view->findItem(u"playinfo"_q))
        item->setProperty("show", d->as.playinfo_visible);
    if (auto item = d->view->findItem(u"logo"_q)) {
        item->setProperty("show", d->pref.show_logo());
        item->setProperty("color", d->pref.bg_color());
    }
}

auto MainWindow::openFromFileManager(const Mrl &mrl) -> void
{
    if (mrl.isDir())
        d->openDir(mrl.toLocalFile());
    else {
        if (mrl.isLocalFile() && _IsSuffixOf(PlaylistExt, mrl.suffix()))
            d->playlist.open(mrl, EncodingInfo());
        else {
            const auto mode = d->pref.open_media_from_file_manager();
            d->openWith(mode, QList<Mrl>() << mrl);
        }
    }
}

auto MainWindow::screen() const -> QScreen*
{
    return d->view->screen();
}

auto MainWindow::engine() const -> PlayEngine*
{
    return &d->e;
}

auto MainWindow::playlist() const -> PlaylistModel*
{
    return &d->playlist;
}

auto MainWindow::exit() -> void
{
    static bool done = false;
    if (!done) {
        OS::setScreensaverEnabled(true);
        d->commitData();
        cApp.quit();
        done = true;
    }
}

auto MainWindow::play() -> void
{
    if (d->stateChanging)
        return;
    if (d->e.mrl().isImage())
        d->menu(u"play"_q)[u"next"_q]->trigger();
    else {
        const auto state = d->e.state();
        switch (state) {
        case PlayEngine::Playing:
            break;
        case PlayEngine::Paused:
            d->e.unpause();
            break;
        default:
            d->load(d->e.mrl());
            break;
        }
    }
}

auto MainWindow::togglePlayPause() -> void
{
    if (d->stateChanging)
        return;
    if (d->e.mrl().isImage())
        d->menu(u"play"_q)[u"next"_q]->trigger();
    else {
        const auto state = d->e.state();
        switch (state) {
        case PlayEngine::Playing:
            d->e.pause();
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
    if (d->adapter->isFullScreen() == full)
        return;
    d->adapter->setFullScreen(full);
#ifdef Q_OS_WIN // This should be checked in WindowStatesChange event for others
    emit fullscreenChanged(full);
    d->updateStaysOnTop();
#endif
}

auto MainWindow::isFullScreen() const -> bool
{
    return d->adapter->isFullScreen();
}

auto MainWindow::resetMoving() -> void
{
    if (d->moving) {
        d->moving = false;
        d->winStartPos = d->mouseStartPos = QPoint();
    }
}

using MsBh = MouseBehavior;

auto MainWindow::onMouseMoveEvent(QMouseEvent *event) -> void
{
    QWidget::mouseMoveEvent(event);
    d->cancelToHideCursor();
    const bool full = isFullScreen();
    const auto gpos = event->globalPos();
    if (full)
        resetMoving();
    else if (d->moving)
        move(d->winStartPos + (gpos - d->mouseStartPos));
    d->readyToHideCursor();
    d->e.sendMouseMove(event->pos());
    if (d->pressedButton == Qt::LeftButton)
        d->pressedButton = Qt::NoButton;
}

auto MainWindow::onMouseDoubleClickEvent(QMouseEvent *event) -> void
{
    QWidget::mouseDoubleClickEvent(event);
    d->singleClick.unset();
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
    const auto singleClickAction = d->singleClick.action;
    const auto pressed = d->pressedButton;
    d->singleClick.unset();
    d->pressedButton = Qt::NoButton;
    if (pressed != event->button())
        return;
    if (pressed == Qt::LeftButton && d->e.isMouseInButton())
        return;
    const auto rect = geometry();
    if (!rect.contains(event->localPos().toPoint() + rect.topLeft()))
        return;
    const auto mb = MouseBehaviorInfo::fromData(pressed);
    if (mb == MsBh::NoBehavior)
        return;
    if (pressed == Qt::LeftButton) {
        if (singleClickAction) {
            d->singleClick.action = singleClickAction;
            d->singleClick.timer.start(qApp->doubleClickInterval() + 10);
        }
    } else
        d->trigger(d->menu.action(d->actionId(mb, event)));
}

auto MainWindow::onMousePressEvent(QMouseEvent *event) -> void
{
    QWidget::mousePressEvent(event);
    d->singleClick.unset();
    d->pressedButton = Qt::NoButton;
    switch (event->button()) {
    case Qt::LeftButton:
        d->e.sendMouseClick(event->pos());
        if (!d->e.isMouseInButton())
            d->pressedButton = Qt::LeftButton;
        if (isFullScreen() || event->modifiers())
            break;
        d->moving = true;
        d->mouseStartPos = event->globalPos();
        d->winStartPos = pos();
        break;
    case Qt::MiddleButton:    case Qt::ExtraButton1:
    case Qt::ExtraButton2:    case Qt::ExtraButton3:
    case Qt::ExtraButton4:    case Qt::ExtraButton5:
    case Qt::ExtraButton6:    case Qt::ExtraButton7:
    case Qt::ExtraButton8:    case Qt::ExtraButton9:
    case Qt::RightButton:
        d->pressedButton = event->button();
        break;
    default:
        break;
    }

    d->contextMenu.hide();
    // context menu should be displayed as soon as pressed
    if (d->pressedButton == d->contextMenuButton
            && d->contextMenuModifier == event->modifiers()) {
        d->contextMenu.exec(event->globalPos());
        d->pressedButton = Qt::NoButton;
    }

    if (d->pressedButton == Qt::LeftButton)
        d->singleClick.action = d->menu.action(d->actionId(MsBh::LeftClick, event));
}

auto MainWindow::onWheelEvent(QWheelEvent *ev) -> void
{
    QWidget::wheelEvent(ev);
    const auto delta = ev->delta();
    if (delta) {
        const bool up = d->pref.invert_wheel() ? delta < 0 : delta > 0;
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
    d->openMimeData(event->mimeData());
}

auto MainWindow::resizeEvent(QResizeEvent *event) -> void
{
    QWidget::resizeEvent(event);
    if (d->adapter->isFullScreen())
        d->container->setGeometry(QRect(QPoint(0, 0), frameSize()));
    else
        d->container->setGeometry(QRect(QPoint(0, 0), size()));
//    d->as.updateWindowGeometry(this);
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
        setWindowFilePath(d->filePath);
        resetMoving();
        d->readyToHideCursor();
        auto changed = [&] (Qt::WindowState state) -> bool
            { return !((event->oldState() & windowState()) & state); };
        if (!d->stateChanging && changed(Qt::WindowMinimized))
            d->doVisibleAction(!isMinimized());
#ifndef Q_OS_WIN // this doesn't work for windows because of fake fullscreen
        if (changed(Qt::WindowFullScreen))
            emit fullscreenChanged(isFullScreen());
#endif
        d->updateStaysOnTop();
    }
}

auto MainWindow::closeEvent(QCloseEvent *event) -> void
{
    setFullScreen(false);
    QWidget::closeEvent(event);
#ifndef Q_OS_MAC
    if (d->tray && d->pref.enable_system_tray()
            && d->pref.hide_rather_close()) {
        hide();
        if (d->as.ask_system_tray) {
            MBox mbox(this);
            mbox.setIcon(MBox::Icon::Information);
            mbox.setTitle(tr("System Tray Icon"));
            mbox.setText(tr("bomi will be running in "
                "the system tray when the window closed."));
            mbox.setInformativeText(
                tr("You can change this behavior in the preferences. "
                    "If you want to exit bomi, please use 'Exit' menu."));
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
}

auto MainWindow::customEvent(QEvent *event) -> void
{
    if ((int)event->type() == GetSmbAuth) {
        SmbAuth *smb = nullptr;
        bool *res = nullptr;
        QWaitCondition *cond = nullptr;
        _TakeData(event, smb, res, cond);
        Q_ASSERT(smb && res && cond);
        SmbAuthDialog dlg(this);
        dlg.setAuthInfo(*smb);
        if (dlg.exec()) {
            *res = true;
            *smb = dlg.authInfo();
        }
        cond->wakeAll();
    }
}
