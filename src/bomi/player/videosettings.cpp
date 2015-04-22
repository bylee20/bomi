#include "videosettings.hpp"
#include "mrlstate_p.hpp"

auto VideoSettings::preset(Preset p) -> VideoSettings
{
    VideoSettings s;
    switch (p) {
    case Highest:
        s.fboFormat = FramebufferObjectFormat::Rgba16;
        s.dithering = Dithering::Fruit;
        s.hqUpscale = s.hqDownscale = true;
        s.interpolator = IntrplParamSet::EwaLanczosSharp;
        s.interpolatorDown = IntrplParamSet::EwaLanczosSoft;
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

auto VideoSettings::fill(MrlState *s) const -> void
{
    s->set_video_dithering(dithering);
    s->set_video_hq_downscaling(hqDownscale);
    s->set_video_hq_upscaling(hqUpscale);
    s->d->intrpl.set(interpolator);
    s->set_video_interpolator(interpolator.type());
    s->d->intrplDown.set(interpolatorDown);
    s->set_video_interpolator_down(interpolatorDown.type());
    s->d->chroma.set(chromaUpscaler);
    s->set_video_chroma_upscaler(chromaUpscaler.type());
}
