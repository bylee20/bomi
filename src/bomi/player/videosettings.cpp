#include "videosettings.hpp"

auto VideoSettings::preset(Preset p) -> VideoSettings
{
    VideoSettings s;
    switch (p) {
    case Highest:
        s.fboFormat = FramebufferObjectFormat::Rgba16;
        s.dithering = Dithering::Fruit;
        s.hqUpscale = s.hqDownscale = true;
        s.interpolator = IntrplParamSet::EwaLanczosSharp;
        s.interpolatorDown = IntrplParamSet::MitchellNetravali;
        s.chromaUpscaler = IntrplParamSet::EwaLanczosSharp;
        s.useIntrplDown = true;
        break;
    case High:
        s.fboFormat = FramebufferObjectFormat::Rgba16;
        s.dithering = Dithering::Fruit;
        s.hqUpscale = s.hqDownscale = true;
        s.interpolator = IntrplParamSet::Spline36;
        s.interpolatorDown = IntrplParamSet::MitchellNetravali;
        s.useIntrplDown = true;
        break;
    case Normal:
        s.fboFormat = FramebufferObjectFormat::Auto;
        s.interpolator = IntrplParamSet::CatmullRom;
        s.interpolatorDown = IntrplParamSet::MitchellNetravali;
        s.useIntrplDown = true;
        break;
    default:
        s.fboFormat = FramebufferObjectFormat::Rgba8;
        break;
    }
    return s;
}
