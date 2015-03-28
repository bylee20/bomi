#include "framebufferobjectformat.hpp"

const std::array<FramebufferObjectFormatInfo::Item, 3> FramebufferObjectFormatInfo::info{{
    {FramebufferObjectFormat::Auto, u"Auto"_q, u""_q, OGL::NoTextureFormat},
    {FramebufferObjectFormat::Rgba8, u"Rgba8"_q, u""_q, OGL::RGBA8_UNorm},
    {FramebufferObjectFormat::Rgba16, u"Rgba16"_q, u""_q, OGL::RGBA16_UNorm}
}};
