#ifndef MPOSDITEM_HPP
#define MPOSDITEM_HPP

#include "stdafx.hpp"
#include "quick/simplefboitem.hpp"
#include "opengl/openglmisc.hpp"

enum EventType {EnqueueFrame = QEvent::User + 1, Show, Hide, NewFrame };

struct sub_bitmaps;

class MpOsdItem : public SimpleFboItem {
    Q_OBJECT
public:
    MpOsdItem(QQuickItem *parent = nullptr);
    ~MpOsdItem();
    auto drawOn(sub_bitmaps *imgs) -> void;
    auto present(bool redraw) -> void;
    auto drawOn(QImage &frame) -> void;
    auto setImageSize(const QSize &size) -> void;
    auto imageSize() const -> QSize override;
private:
    auto paint(OpenGLFramebufferObject *fbo) -> void override;
    auto initializeGL() -> void override;
    auto finalizeGL() -> void override;
    auto customEvent(QEvent *event) -> void override;
    struct Data;
    Data *d;
};

#endif // MPOSDITEM_HPP
