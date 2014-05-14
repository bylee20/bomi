#ifndef MPOSDITEM_HPP
#define MPOSDITEM_HPP

#include "stdafx.hpp"
#include "quick/simplefboitem.hpp"
#include "opengl/openglmisc.hpp"
#include "videoimagepool.hpp"

class MpOsdBitmap;

class MpOsdItem : public SimpleFboItem {
    Q_OBJECT
    using Cache = VideoImageCache<MpOsdBitmap>;
public:
    MpOsdItem(QQuickItem *parent = nullptr);
    ~MpOsdItem();
    auto drawOn(QImage &frame) -> void;
    auto imageSize() const -> QSize override;
    auto draw(const Cache &cache) -> void;
private:
    auto afterUpdate() -> void;
    auto paint(OpenGLFramebufferObject *fbo) -> void override;
    auto initializeGL() -> void override;
    auto finalizeGL() -> void override;
    struct Data;
    Data *d;
};

#endif // MPOSDITEM_HPP
