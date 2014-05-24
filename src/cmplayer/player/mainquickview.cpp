#include "mainquickview.hpp"
#include "skin.hpp"
#include "mainwindow.hpp"
#include "quick/appobject.hpp"
#include "quick/toplevelitem.hpp"
#include "quick/globalqmlobject.hpp"

struct MainQuickView::Data {
    MainWindow *main = nullptr;
};

MainQuickView::MainQuickView(MainWindow *main)
    : QQuickView(main->windowHandle())
    , d(new Data)
{
    d->main = main;
    setColor(Qt::black);
    setResizeMode(QQuickView::SizeRootObjectToView);
    main->installEventFilter(this);
    m_top = new TopLevelItem;
    AppObject::setTopLevelItem(m_top);
    UtilObject::setMainWindow(d->main);
    UtilObject::setQmlEngine(engine());
    connect(this, &QQuickView::statusChanged, this, [this] (Status status) {
       if (status == Ready)
            m_top->setParentItem(contentItem());
    });
}

MainQuickView::~MainQuickView()
{
    delete m_top;
    delete d;
}

auto MainQuickView::setSkin(const QString &name) -> bool
{
    engine()->clearComponentCache();
    rootContext()->setContextProperty("Util", &UtilObject::get());
    Skin::apply(this, name);
    if (status() != QQuickView::Error)
        return true;
    setSource(QUrl("qrc:/emptyskin.qml"));
    return false;
}

auto MainQuickView::setCursorVisible(bool visible) -> void
{
    if (visible && cursor().shape() == Qt::BlankCursor) {
        unsetCursor();
        UtilObject::setCursorVisible(true);
    } else if (!visible && cursor().shape() != Qt::BlankCursor) {
        setCursor(Qt::BlankCursor);
        UtilObject::setCursorVisible(false);
    }
}

auto MainQuickView::eventFilter(QObject *obj, QEvent *ev) -> bool
{
    if (obj != d->main)
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

auto MainQuickView::mouseDoubleClickEvent(QMouseEvent *event) -> void
{
    d->main->resetMoving();
    UtilObject::resetFilterDoubleClick();
    QQuickView::mouseDoubleClickEvent(event);
    if (!UtilObject::isDoubleClickFiltered())
        d->main->onMouseDoubleClickEvent(event);
}

auto MainQuickView::keyPressEvent(QKeyEvent *event) -> void
{
    event->setAccepted(false);
    if (auto item = UtilObject::itemToAcceptKey())
        sendEvent(item, event);
    if (!event->isAccepted())
        d->main->onKeyPressEvent(event);
}

auto MainQuickView::mousePressEvent(QMouseEvent *event) -> void
{
    d->main->resetMoving();
    m_top->resetMousePressEventFilterState();
    event->setAccepted(false);
    QQuickView::mousePressEvent(event);
    if (!event->isAccepted() || m_top->filteredMousePressEvent())
        d->main->onMousePressEvent(event);
}

auto MainQuickView::mouseReleaseEvent(QMouseEvent *event) -> void
{
    d->main->resetMoving();
    event->setAccepted(false);
    QQuickView::mouseReleaseEvent(event);
    if (!event->isAccepted())
        d->main->onMouseReleaseEvent(event);
}

auto MainQuickView::mouseMoveEvent(QMouseEvent *event) -> void
{
    event->setAccepted(false);
    QQuickView::mouseMoveEvent(event);
    d->main->onMouseMoveEvent(event);
}

auto MainQuickView::wheelEvent(QWheelEvent *event) -> void
{
    event->setAccepted(false);
    QQuickView::wheelEvent(event);
    if (!event->isAccepted())
        d->main->onWheelEvent(event);
}

auto MainQuickView::event(QEvent *event) -> bool
{
    if (QQuickView::event(event))
        return true;
    if (event->type() == QEvent::DragMove) {
        d->main->dragEnterEvent(static_cast<QDragEnterEvent*>(event));
    } else if (event->type() == QEvent::Drop) {
        d->main->dropEvent(static_cast<QDropEvent*>(event));
    } else
        return false;
    return true;
}

auto MainQuickView::topLevelItem() const -> QQuickItem*
{
    return m_top;
}
