#include "opengltexture1d.hpp"

auto OpenGLTexture1D::initialize(int w, const OpenGLTextureTransferInfo &info,
                                 const void *data) -> void
{
    m_info = info; m_width = w;
    if (!isEmpty())
        glTexImage1D(target(), 0, m_info.texture, m_width, 0,
                     m_info.transfer.format, m_info.transfer.type, data);
}
