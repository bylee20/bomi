#ifndef MPOSDITEM_HPP
#define MPOSDITEM_HPP

#include "stdafx.hpp"
#include "quick/simplefboitem.hpp"
#include "openglmisc.hpp"

enum EventType {EnqueueFrame = QEvent::User + 1, NextFrame, EmptyQueue, Rerender, UpdateDeint, Show, Hide, NewFrame };

struct sub_bitmaps;

class MpOsdItem : public SimpleFboItem {
    Q_OBJECT
public:
    MpOsdItem(QQuickItem *parent = nullptr);
    ~MpOsdItem();
    void drawOn(sub_bitmaps *imgs);
    void present(bool redraw);
    void drawOn(QImage &frame);
    void setImageSize(const QSize &size);
    QSize imageSize() const override;
private:
    void paint(OpenGLFramebufferObject *fbo) override;
    void initializeGL() override;
    void finalizeGL() override;
    void customEvent(QEvent *event) override;
    struct Data;
    Data *d;
};

#endif // MPOSDITEM_HPP
