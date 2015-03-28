#include "framebufferobjectformat.hpp"

const std::array<FramebufferObjectFormatInfo::Item, 3> FramebufferObjectFormatInfo::info{{
    {FramebufferObjectFormat::Auto, u"Auto"_q, u"auto"_q, OGL::NoTextureFormat},
    {FramebufferObjectFormat::Rgba8, u"Rgba8"_q, u"rgba8"_q, OGL::RGBA8_UNorm},
    {FramebufferObjectFormat::Rgba16, u"Rgba16"_q, u"rgba16"_q, OGL::RGBA16_UNorm}
}};
