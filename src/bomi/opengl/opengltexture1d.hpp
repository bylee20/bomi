#ifndef OPENGLTEXTURE1D_HPP
#define OPENGLTEXTURE1D_HPP

#include "opengltexturebase.hpp"

class OpenGLTexture1D : public OpenGLTextureBase {
public:
    OpenGLTexture1D(): OpenGLTextureBase(OGL::Target1D) {}
    auto width() const -> int { return m_width; }
    auto initialize(int width, const OpenGLTextureTransferInfo &info,
                    const void *data = nullptr) -> void;
    auto isEmpty() const -> bool { return !isValid() || m_width <= 0; }
private:
    int m_width = 0;
};

#endif // OPENGLTEXTURE1D_HPP
