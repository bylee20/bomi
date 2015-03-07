#ifndef APPWIN_H
#define APPWIN_H

#include "os/os.hpp"
#include <qt_windows.h>

namespace OS {

class Dxva2Info : public HwAcc {
public:
    Dxva2Info();
};

auto createAdapter(QWidget *w) -> WindowAdapter*;

class WinWindowAdapter : public WindowAdapter {
public:
    WinWindowAdapter(QWidget* w): WindowAdapter(w) { }
    auto setFullScreen(bool fs) -> void final;
    auto isFullScreen() const -> bool final { return m_fs; }
    auto isAlwaysOnTop() const -> bool final { return m_onTop; }
    auto setAlwaysOnTop(bool onTop) -> void final;
    auto setFrameless(bool frameless) -> void final;
    auto isFrameless() const -> bool final { return m_frameless; }
    auto fullScreenMargin() const -> int final { return 1; }
private:
    auto updateFrame() -> void;
    auto layer() const -> HWND
        { return m_onTop ? HWND_TOPMOST : HWND_NOTOPMOST; }
    bool m_onTop = false, m_fs = false, m_frameless = false;
    QRect m_prevGeometry;
    DWORD m_prevStyle = 0;
};

}

#endif // APPWIN_H
