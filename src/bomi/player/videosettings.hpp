#ifndef VIDEOSETTINGS_HPP
#define VIDEOSETTINGS_HPP

#include "enum/framebufferobjectformat.hpp"
#include "enum/interpolator.hpp"
#include "enum/dithering.hpp"
#include "video/interpolatorparams.hpp"

class MrlState;

struct VideoSettings {
    DECL_EQ(VideoSettings, &T::fboFormat, &T::chromaUpscaler, &T::interpolator,
            &T::interpolatorDown, &T::hqDownscale, &T::hqUpscale, &T::useIntrplDown,
            &T::dithering)
    enum Preset { Basic, Normal, High, Highest };
    FramebufferObjectFormat fboFormat = FramebufferObjectFormat::Auto;
    IntrplParamSet chromaUpscaler, interpolator, interpolatorDown;
    bool hqUpscale = false, hqDownscale = false, useIntrplDown = false;
    Dithering dithering = Dithering::None;
    QString name;
    auto fill(MrlState *s) const -> void;
    static auto preset(Preset p) -> VideoSettings;
};

#endif // VIDEOSETTINGS_HPP
