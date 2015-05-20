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
    auto showMaximized() -> void final;
    auto showMinimized() -> void final;
    auto showNormal() -> void final;
    auto screen() const -> QScreen* final;
private:
    auto nativeEventFilter(const QByteArray &, void *message, long *result) -> bool final;
    auto eventFilter(QObject *obj, QEvent *ev) -> bool final;
    auto layer() const -> HWND
        { return m_onTop ? HWND_TOPMOST : HWND_NOTOPMOST; }
    auto sysFrameMargins() const -> QMargins;
    auto updateFrame() -> void;
    auto frameMarginsHack() -> void;
    HWND m_hwnd = 0;
    bool m_onTop = false, m_fs = false, m_frameless = false;
    bool m_wmMove = false, m_sizing = false, m_moving = false;
    HIMC m_ime = nullptr;
    QPoint m_startMousePos, m_startWinPos;
    QRect m_normalGeometry;
    LONG m_frameStyle = 0;
};

}

#endif // APPWIN_H
