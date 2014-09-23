#ifndef OPENGLTEXTURE2D_HPP
#define OPENGLTEXTURE2D_HPP

#include "opengltexturebase.hpp"

class OpenGLTexture2D : public OpenGLTextureBase {
    friend class OpenGLFramebufferObject;
public:
    OpenGLTexture2D(OGL::Target target = OGL::Target2D): OpenGLTextureBase(target) {}
    auto isRectangle() const -> bool { return target() == OGL::TargetRectangle; }
    auto width() const -> int { return m_width; }
    auto height() const -> int { return m_height; }
    auto size() const -> QSize { return {m_width, m_height}; }
    auto initialize(const QSize &size, const OpenGLTextureTransferInfo &info, const void *data = nullptr) -> void { initialize(size.width(), size.height(), info, data); }
    auto initialize(int width, int height, OGL::TransferFormat transfer, const void *data = nullptr) -> void;
    auto initialize(int width, int height, const OpenGLTextureTransferInfo &info, const void *data = nullptr) -> void {
        m_info = info; initialize(width, height, data);
    }
    auto initialize(const QSize &size, const void *data = nullptr) -> void { initialize(size.width(), size.height(), data); }
    auto initialize(int width, int height, const void *data = nullptr) -> void {
        m_width = width; m_height = height; initialize(data);
    }
    auto setAttributes(int width, int height, const OpenGLTextureTransferInfo &info) -> void {
        m_width = width; m_height = height; m_info = info;
    }
    auto initialize(const void *data = nullptr) -> void {
        if (!isEmpty())
            glTexImage2D(target(), 0, m_info.texture, m_width, m_height, 0, m_info.transfer.format, m_info.transfer.type, data);
    }
    auto isEmpty() const -> bool { return !isValid() || m_width <= 0 || m_height <= 0; }
    auto upload(int x, int y, int width, int height, const void *data) -> void {
        glTexSubImage2D(target(), 0, x, y, width, height, m_info.transfer.format, m_info.transfer.type, data);
    }
    auto upload(const QRect &rect, const void *data) -> void { upload(rect.x(), rect.y(), rect.width(), rect.height(), data); }
    auto upload(int width, int height, const void *data) -> void { upload(0, 0, width, height, data); }
    auto upload(const void *data) -> void { upload(0, 0, m_width, m_height, data); }
    auto toImage() const -> QImage;
    const QPointF &correction() const { return m_correction; }
    QPointF &correction() { return m_correction; }
    auto save(const QString &fileName) const -> bool;
private:
    friend class VideoFrameShader;
    int m_width = 0, m_height = 0;
    QPointF m_correction = {1.0, 1.0};
};

#endif // OPENGLTEXTURE2D_HPP
