#include "mainwindow_p.hpp"
#include "app.hpp"
#include "misc/trayicon.hpp"
#include "dialog/mbox.hpp"
#include "dialog/encoderdialog.hpp"
#include "quick/appobject.hpp"
#include <QSessionManager>

//DECLARE_LOG_CONTEXT(Main)

MainWindow::MainWindow()
    : QQuickView(), m_glLogger(new OpenGLLogger("SG"))
    , m_engine(new PlayEngine), d(new Data(this))
{
    d->p = this;

    cApp.setWindowTitle(this, QString());
    setColor(Qt::black);
    setResizeMode(QQuickView::SizeRootObjectToView);
    auto format = requestedFormat();
    if (OpenGLLogger::isAvailable())
        format.setOption(QSurfaceFormat::DebugContext);
    setFormat(format);
    setPersistentOpenGLContext(true);
    setPersistentSceneGraph(true);

    d->top = new TopLevelItem;

    d->pref.initialize();
    d->pref.load();
    d->undo.setActive(false);
    d->logViewer = d->dialog<LogViewer>();
    d->adapter = OS::adapter(this);

    AppObject::setTopLevelItem(d->top);
    AppObject::setQmlEngine(QQuickView::engine());
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

    d->initContextMenu();
    d->initItems();
    d->initTray();
    d->plugEngine();
    d->plugMenu();

    connect(this, &QQuickView::statusChanged, this, [=] (Status status)
        { if (status == Ready) d->top->setParentItem(contentItem()); });
    connect(d->adapter, &OS::WindowAdapter::stateChanged, this,
            [=] (Qt::WindowState ws) { d->updateWindowState(ws); });
    connect(this, &QQuickView::sceneGraphInitialized, this, [this] () {
        auto context = openglContext();
        if (cApp.isOpenGLDebugLoggerRequested())
            m_glLogger->initialize(context);
        m_sgInit = true;
        m_engine->initializeGL(this, context);
        emit sceneGraphInitialized();
        _Debug("Scene graph initialized.");
    }, Qt::DirectConnection);
    connect(this, &QQuickView::sceneGraphInvalidated, this, [this] () {
        auto context = QOpenGLContext::currentContext();
        m_sgInit = false;
        m_glLogger->finalize(context);
        m_glLogger->deleteLater();
        m_engine->finalizeGL(context);
        m_engine->deleteLater();
        _Debug("Scene graph invalidated.");
    }, Qt::DirectConnection);
    connect(this, &MainWindow::fullscreenChanged, this,
            [=] (bool fs) { d->setCursorVisible(!fs); });
    connect(&cApp, &App::commitDataRequest, this, [=] () { d->commitData(); });
    connect(&cApp, &App::saveStateRequest, this, [=] (QSessionManager &session)
        { session.setRestartHint(QSessionManager::RestartIfRunning); });

    d->restoreState();
    d->undo.setActive(true);
    QTimer::singleShot(1, this, SLOT(postInitialize()));

#ifdef Q_OS_WIN
    d->taskbar.setWindow(this);
    d->taskbar.progress()->setVisible(true);
#endif
}

MainWindow::~MainWindow() {
    cApp.setMprisActivated(false);
    if (d->jrServer)
        d->jrServer->setInterface(nullptr);
    delete d->jrServer;
    exit();
    setPersistentOpenGLContext(false);
    setPersistentSceneGraph(false);
    d->clear();
    d->deleteDialogs();
    _Debug("Delete main window.");
    delete d;
}

auto MainWindow::postInitialize() -> void
{
    if (d->as.win_frameless)
        d->menu(u"window"_q)[u"frameless"_q]->trigger();
    d->as.restoreWindowGeometry(this);
    d->adapter->setImeEnabled(false);
    d->applyPref();
    cApp.runCommands();
    d->noMessage = false;
}

auto MainWindow::adapter() const -> OS::WindowAdapter*
{
    return d->adapter;
}

auto MainWindow::setupSkinPlayer() -> void
{
    d->cropbox = nullptr;
    d->player = rootObject()->property("player").value<QQuickItem*>();
    if (!d->player)
        return;
    d->cropbox = d->findItem<QQuickItem>(u"cropbox"_q);
    if (d->encoder)
        d->cropbox->setVisible(d->encoder->isVisible());
    d->e.screen()->setParentItem(d->player);
    d->e.screen()->setWidth(d->player->width());
    d->e.screen()->setHeight(d->player->height());
    if (auto item = d->findItem(u"playinfo"_q))
        item->setProperty("show", d->as.playinfo_visible);
    if (auto item = d->findItem(u"logo"_q)) {
        item->setProperty("show", d->pref.show_logo());
        item->setProperty("color", d->pref.bg_color());
    }
}

auto MainWindow::openFromFileManager(const Mrl &mrl, const QString &sub) -> void
{
    if (mrl.isDir())
        d->openDir(mrl.toLocalFile());
    else {
        if (mrl.isLocalFile() && _IsSuffixOf(PlaylistExt, mrl.suffix()))
            d->playlist.open(mrl, EncodingInfo());
        else {
            const auto mode = d->pref.open_media_from_file_manager();
            d->openWith(mode, QList<Mrl>() << mrl, sub);
        }
    }
}

auto MainWindow::setSubtitle(const QString &sub) -> void
{
    d->e.setSubtitleFiles(QStringList{sub}, d->pref.sub_enc());
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

auto MainWindow::pause() -> void
{
    d->pausedByHiding = false;
    if (d->stateChanging)
        return;
    if (d->e.isPlaying())
        d->e.pause();
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
            if (d->e.mrl().isEmpty())
                d->menu(u"open"_q)[u"file"_q]->trigger();
            else
                d->load(d->e.mrl());
            break;
        }
    }
}

auto MainWindow::togglePlayStop() -> void
{
    if (!d->e.isRunning())
        play();
    else
        d->e.stop();
}

auto MainWindow::togglePlayPause() -> void
{
    if (d->e.isPlaying() && !d->e.mrl().isImage())
        pause();
    else
        play();
}

auto MainWindow::isSceneGraphInitialized() const -> bool
{
    return m_sgInit;
}

auto MainWindow::setFullScreen(bool full, bool updateLastGeometry) -> void
{
    if (d->adapter->isFullScreen() == full)
        return;
    if (updateLastGeometry && !d->adapter->isFullScreen())
        d->as.updateLastWindowGeometry(this);
    d->adapter->setFullScreen(full);
}

auto MainWindow::isFullScreen() const -> bool
{
    return d->adapter->isFullScreen();
}

using MsBh = MouseBehavior;

#ifdef Q_OS_WIN
#define ME(name) { \
    if (d->adapter->isFullScreen()) { \
        auto l = event->localPos(); \
        auto w = event->windowPos(); \
        auto g = event->screenPos(); \
        QPointF dp(0, 0); \
        if (l.x() < 0) dp.rx() = -l.x(); \
        else if (l.x() > width()) dp.rx() = width() - l.x(); \
        if (l.y() < 0) dp.ry() = -l.y(); \
        else if (l.y() > height()) dp.ry() = height() - l.y(); \
        l += dp; w += dp; g +=dp; \
        QMouseEvent ev(event->type(), l, w, g, event->button(), \
                       event->buttons(), event->modifiers()); \
        QQuickView::name(&ev); \
    } else \
        QQuickView::name(event); \
}
#else
#define ME(name) {QQuickView::name(event);}
#endif

auto MainWindow::mouseMoveEvent(QMouseEvent *event) -> void
{
    event->setAccepted(false);
    ME(mouseMoveEvent);
    d->cancelToHideCursor();
    const bool full = isFullScreen();
    const auto gpos = event->globalPos();
    if (full)
        d->adapter->endMoveByDrag();
    else if (d->adapter->isMoveByDragStarted())
        d->adapter->moveByDrag(gpos);
    d->readyToHideCursor();
    d->e.sendMouseMove(event->pos());
    if (d->pressedButton == Qt::LeftButton)
        d->pressedButton = Qt::NoButton;
}


auto MainWindow::mouseDoubleClickEvent(QMouseEvent *event) -> void
{
    d->adapter->endMoveByDrag();
    event->setAccepted(false);
    if (AppObject::itemToAccept(AppObject::DoubleClickEvent, event->localPos()))
        QQuickView::mouseDoubleClickEvent(event);
    if (event->isAccepted())
        return;
    d->singleClick.unset();
    if (event->buttons() & Qt::LeftButton) {
        const auto act = d->menu.action(d->actionId(MsBh::DoubleClick, event));
        if (!act)
            return;
#ifdef Q_OS_MAC
        if (action == d->menu(u"window"_q)[u"toggle-fs"_q])
            QTimer::singleShot(300, action, SLOT(trigger()));
        else
#endif
        d->trigger(act);
    }
}

auto MainWindow::mouseReleaseEvent(QMouseEvent *event) -> void
{
    d->adapter->endMoveByDrag();
    event->setAccepted(false);
    ME(mouseReleaseEvent);
    if (event->isAccepted())
        return;
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



auto MainWindow::mousePressEvent(QMouseEvent *event) -> void
{
    d->adapter->endMoveByDrag();
    d->top->resetMousePressEventFilterState();
    event->setAccepted(false);
    ME(mousePressEvent);
    if (event->isAccepted() && !d->top->filteredMousePressEvent())
        return;
    d->singleClick.unset();
    d->pressedButton = Qt::NoButton;
    switch (event->button()) {
    case Qt::LeftButton:
        d->e.sendMouseClick(event->pos());
        if (!d->e.isMouseInButton())
            d->pressedButton = Qt::LeftButton;
        if (isFullScreen() || event->modifiers())
            break;
        d->adapter->startMoveByDrag(event->globalPos());
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

auto MainWindow::wheelEvent(QWheelEvent *event) -> void
{
    event->setAccepted(false);
    QQuickView::wheelEvent(event);
    if (!event->isAccepted()) {
        d->wheelAngles += event->angleDelta().y();
        const int delta = d->wheelAngles >= 120 ? 1 : d->wheelAngles <= -120 ? -1 : 0;
        if (delta) {
            const bool up = d->pref.invert_wheel() ? delta < 0 : delta > 0;
            const auto id = d->actionId(up ? MsBh::ScrollUp : MsBh::ScrollDown, event);
            d->trigger(d->menu.action(id));
            event->accept();
        }
    }
    if (event->isAccepted())
        d->wheelAngles = 0;
}

auto MainWindow::resizeEvent(QResizeEvent *event) -> void
{
    QQuickView::resizeEvent(event);
}

auto MainWindow::moveEvent(QMoveEvent *event) -> void
{
    QQuickView::moveEvent(event);
}

auto MainWindow::keyPressEvent(QKeyEvent *event) -> void
{
    event->setAccepted(false);
    if (auto item = AppObject::itemToAccept(AppObject::KeyEvent))
        sendEvent(item, event);
    if (event->isAccepted())
        return;
    constexpr int modMask = Qt::SHIFT | Qt::CTRL | Qt::ALT | Qt::META;
    const QKeySequence shortcut(event->key() + (event->modifiers() & modMask));
    d->trigger(RootMenu::instance().action(shortcut));
    event->accept();
}

auto MainWindow::showEvent(QShowEvent *event) -> void
{
    QQuickView::showEvent(event);
    d->doVisibleAction(true);
}

auto MainWindow::hideEvent(QHideEvent *event) -> void
{
    QQuickView::hideEvent(event);
    d->doVisibleAction(false);
}

auto MainWindow::event(QEvent *event) -> bool
{
    switch (event->type()) {
    case QEvent::InputMethodQuery: {
        auto e = static_cast<QInputMethodQueryEvent*>(event);
        e->setValue(Qt::ImEnabled, false);
        e->accept();
        return true;
    } case QEvent::DragEnter:
    case QEvent::DragMove: {
        auto e = static_cast<QDragEnterEvent*>(event);
        if (e->mimeData()->hasUrls())
            e->acceptProposedAction();
        return true;
    } case QEvent::Drop: {
        auto e = static_cast<QDropEvent*>(event);
        d->openMimeData(e->mimeData());
        return true;
    } case QEvent::Close: {
        setFullScreen(false);
    #ifndef Q_OS_MAC
        if (d->tray && d->pref.enable_system_tray()
                && d->pref.hide_rather_close()) {
            hide();
            if (d->as.ask_system_tray) {
                MBox mbox(nullptr);
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
        return true;
    } default:
        return QQuickView::event(event);
    }
}

auto MainWindow::customEvent(QEvent *event) -> void
{
    QWaitCondition *cond = nullptr;
    switch ((int)event->type()) {
    case GetSmbAuth: {
        SmbAuth *smb = nullptr;
        bool *res = nullptr;
        _TakeData(event, smb, res, cond);
        Q_ASSERT(smb && res && cond);
        auto dlg = d->dialog<SmbAuthDialog>();
        dlg->setAuthInfo(*smb);
        if (dlg->exec()) {
            *res = true;
            *smb = dlg->authInfo();
        }
        cond->wakeAll();
        break;
    } default:
        break;
    }
}

auto MainWindow::wake() -> void {
    setVisible(true);
    raise();
    requestActivate();
}

auto MainWindow::eventFilter(QObject *o, QEvent *e) -> bool
{
#ifdef Q_OS_WIN
    if (qobject_cast<QDialog*>(o) && e->type() == QEvent::Show)
        d->dialogWorkaround.start();
#endif
    return QQuickView::eventFilter(o, e);
}

auto MainWindow::isFrameless() const -> bool
{
    return d->as.win_frameless;
}
