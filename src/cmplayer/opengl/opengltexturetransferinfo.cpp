#include "opengltexturetransferinfo.hpp"

auto OpenGLTextureTransferInfo::get(OGL::TransferFormat format,
                                    int bpc) -> OpenGLTextureTransferInfo
{
    static QMap<OGL::TransferFormat, OpenGLTextureTransferInfo> formats[2];
    if (formats[0].isEmpty()) {
        formats[0][OGL::Red] = {OGL::R8_UNorm, OGL::Red, OGL::UInt8};
        formats[0][OGL::RG] = {OGL::RG8_UNorm, OGL::RG, OGL::UInt8};
        formats[0][OGL::Luminance] = {OGL::Luminance8_UNorm,
                                         OGL::Luminance, OGL::UInt8};
        formats[0][OGL::LuminanceAlpha] = {OGL::LuminanceAlpha8_UNorm,
                                              OGL::LuminanceAlpha, OGL::UInt8};
        formats[0][OGL::RGB] = {OGL::RGB8_UNorm, OGL::RGB, OGL::UInt8};
        formats[0][OGL::BGR] = {OGL::RGB8_UNorm, OGL::BGR, OGL::UInt8};
        formats[0][OGL::BGRA] = {OGL::RGBA8_UNorm,
                                    OGL::BGRA, OGL::UInt32_8_8_8_8_Rev};
        formats[0][OGL::RGBA] = {OGL::RGBA8_UNorm,
                                    OGL::RGBA, OGL::UInt32_8_8_8_8_Rev};

        formats[1][OGL::Red] = {OGL::R16_UNorm, OGL::Red, OGL::UInt16};
        formats[1][OGL::RG] = {OGL::R16_UNorm, OGL::RG, OGL::UInt16};
        formats[1][OGL::Luminance] = {OGL::Luminance16_UNorm,
                                         OGL::Luminance, OGL::UInt16};
        formats[1][OGL::LuminanceAlpha] = {OGL::LuminanceAlpha16_UNorm,
                                              OGL::LuminanceAlpha, OGL::UInt16};
        formats[1][OGL::RGB] = {OGL::RGB16_UNorm, OGL::RGB, OGL::UInt16};
        formats[1][OGL::BGR] = {OGL::RGB16_UNorm, OGL::BGR, OGL::UInt16};
        formats[1][OGL::BGRA] = {OGL::RGBA16_UNorm, OGL::BGRA, OGL::UInt16};
        formats[1][OGL::RGBA] = {OGL::RGBA16_UNorm, OGL::RGBA, OGL::UInt16};

        const bool rg = OGL::hasExtension(OGL::TextureRG);
        for (auto &fmt : formats) {
            if (rg) {
                fmt[OGL::OneComponent] = fmt[OGL::Red];
                fmt[OGL::TwoComponents] = fmt[OGL::RG];
            } else {
                fmt[OGL::OneComponent] = fmt[OGL::Luminance];
                fmt[OGL::TwoComponents] = fmt[OGL::LuminanceAlpha];
            }
        }
    }
    Q_ASSERT(_IsOneOf(bpc, 1, 2));
    return _C(formats[bpc-1])[format];
}
