#ifndef OPENGLTEXTURETRANSFERINFO_HPP
#define OPENGLTEXTURETRANSFERINFO_HPP

#include "openglmisc.hpp"

class OpenGLTextureTransferInfo {
public:
    OpenGLTextureTransferInfo() = default;
    OpenGLTextureTransferInfo(OGL::TextureFormat texture,
                              OGL::TransferFormat transferFormat,
                              OGL::TransferType transferType)
        : texture(texture), transfer(transferFormat, transferType) { }
    auto operator == (const OpenGLTextureTransferInfo &rhs) const -> bool
        { return texture == rhs.texture && transfer == rhs.transfer; }
    auto operator != (const OpenGLTextureTransferInfo &rhs) const -> bool
        { return !operator == (rhs); }
    static auto get(OGL::TransferFormat componentFormat,
                    int bytesPerComponent = 1) -> OpenGLTextureTransferInfo;

    OGL::TextureFormat texture = OGL::RGBA8_UNorm;
    OGL::TransferInfo transfer;
};

#endif // OPENGLTEXTURETRANSFERINFO_HPP
