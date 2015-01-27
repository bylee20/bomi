#ifndef WINDOWOBJECT_HPP
#define WINDOWOBJECT_HPP

#include "player/mainwindow.hpp"

class WindowObject : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool fullscreen READ fullscreen NOTIFY fullscreenChanged)
public:
    auto set(MainWindow *mw) -> void;
    auto fullscreen() const -> bool { return m->isFullScreen(); }
signals:
    void fullscreenChanged();
private:
    MainWindow *m = nullptr;
};

#endif // WINDOWOBJECT_HPP
