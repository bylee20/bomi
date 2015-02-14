#ifndef MAINQUICKVIEW_HPP
#define MAINQUICKVIEW_HPP

#include <QQuickView>
#include <QQuickItem>

class MainWindow;                       class TopLevelItem;

class MainQuickView : public QQuickView {
    Q_OBJECT
public:
    MainQuickView(MainWindow *main);
    ~MainQuickView();
    template <class T = QObject>
    auto findItem(const QString &name = QString()) -> T*
        { return rootObject()->findChild<T*>(name); }
    auto topLevelItem() const -> QQuickItem*;
    auto setCursorVisible(bool visible) -> void;
    auto setSkin(const QString &name) -> bool;
    auto clear() -> void;
private:
    auto event(QEvent *event) -> bool final;
    auto eventFilter(QObject *obj, QEvent *ev) -> bool final;
    auto keyPressEvent(QKeyEvent *event) -> void final;
    auto mouseDoubleClickEvent(QMouseEvent *event) -> void final;
    auto mousePressEvent(QMouseEvent *event) -> void final;
    auto mouseReleaseEvent(QMouseEvent *event) -> void final;
    auto mouseMoveEvent(QMouseEvent *event) -> void final;
    auto wheelEvent(QWheelEvent *event) -> void final;
    TopLevelItem *m_top = nullptr;
    struct Data;
    Data *d;
};

#endif // MAINQUICKVIEW_HPP
