#ifndef APPWIN_H
#define APPWIN_H

#include "os/os.hpp"
#include <qt_windows.h>
#include <QAbstractNativeEventFilter>

namespace OS {

class Dxva2Info : public HwAcc {
public:
    Dxva2Info();
};

auto createAdapter(QWindow *w) -> WindowAdapter*;

class WinWindowAdapter : public WindowAdapter, public QAbstractNativeEventFilter {
public:
    WinWindowAdapter(QWindow* w);
    auto setFullScreen(bool fs) -> void final;
    auto isFullScreen() const -> bool final { return m_fs; }
    auto isAlwaysOnTop() const -> bool final { return m_onTop; }
    auto setAlwaysOnTop(bool onTop) -> void final;
    auto setFrameless(bool frameless) -> void final;
    auto isFrameless() const -> bool final { return m_frameless; }
    auto setImeEnabled(bool enabled) -> void final;
    auto isImeEnabled() const -> bool final;
    auto isSnappableToEdge() const -> bool final { return true; }
private:
    auto nativeEventFilter(const QByteArray &, void *message, long *result) -> bool final;
    auto eventFilter(QObject *obj, QEvent *ev) -> bool final;
    auto layer() const -> HWND
        { return m_onTop ? HWND_TOPMOST : HWND_NOTOPMOST; }
    bool m_onTop = false, m_fs = false, m_frameless = false;
    HIMC m_ime = nullptr;
    QPoint m_position;
};

}

#endif // APPWIN_H
