#include "enums.hpp"
const std::array<SpeakerIdInfo::Item, 11> SpeakerIdInfo::info{{
	{SpeakerId::FrontLeft, "FrontLeft", "", MP_SPEAKER_ID_FL},
	{SpeakerId::FrontRight, "FrontRight", "", MP_SPEAKER_ID_FR},
	{SpeakerId::FrontCenter, "FrontCenter", "", MP_SPEAKER_ID_FC},
	{SpeakerId::LowFrequency, "LowFrequency", "", MP_SPEAKER_ID_LFE},
	{SpeakerId::BackLeft, "BackLeft", "", MP_SPEAKER_ID_BL},
	{SpeakerId::BackRight, "BackRight", "", MP_SPEAKER_ID_BR},
	{SpeakerId::FrontLeftCenter, "FrontLeftCenter", "", MP_SPEAKER_ID_FLC},
	{SpeakerId::FrontRightCenter, "FrontRightCenter", "", MP_SPEAKER_ID_FRC},
	{SpeakerId::BackCenter, "BackCenter", "", MP_SPEAKER_ID_BC},
	{SpeakerId::SideLeft, "SideLeft", "", MP_SPEAKER_ID_SL},
	{SpeakerId::SideRight, "SideRight", "", MP_SPEAKER_ID_SR}
}};

const std::array<ChannelLayoutInfo::Item, 27> ChannelLayoutInfo::info{{
	{ChannelLayout::Default, "Default", "default", "empty"},
	{ChannelLayout::Mono, "Mono", "1.0", "mono"},
	{ChannelLayout::_2_0, "_2_0", "2.0", "stereo"},
	{ChannelLayout::_2_1, "_2_1", "2.1", "2.1"},
	{ChannelLayout::_3_0, "_3_0", "3.0", "3.0"},
	{ChannelLayout::_3_0_Back, "_3_0_Back", "3.0(back)", "3.0(back)"},
	{ChannelLayout::_3_1, "_3_1", "3.1", "3.1"},
	{ChannelLayout::_4_0, "_4_0", "4.0", "quad"},
	{ChannelLayout::_4_0_Side, "_4_0_Side", "4.0(side)", "quad(side)"},
	{ChannelLayout::_4_0_Diamond, "_4_0_Diamond", "4.0(diamond)", "4.0"},
	{ChannelLayout::_4_1, "_4_1", "4.1", "4.1(alsa)"},
	{ChannelLayout::_4_1_Diamond, "_4_1_Diamond", "4.1(diamond)", "4.1"},
	{ChannelLayout::_5_0, "_5_0", "5.0", "5.0(alsa)"},
	{ChannelLayout::_5_0_Side, "_5_0_Side", "5.0(side)", "5.0(side)"},
	{ChannelLayout::_5_1, "_5_1", "5.1", "5.1(alsa)"},
	{ChannelLayout::_5_1_Side, "_5_1_Side", "5.1(side)", "5.1(side)"},
	{ChannelLayout::_6_0, "_6_0", "6.0", "6.0"},
	{ChannelLayout::_6_0_Front, "_6_0_Front", "6.0(front)", "6.0(front)"},
	{ChannelLayout::_6_0_Hex, "_6_0_Hex", "6.0(hexagonal)", "hexagonal"},
	{ChannelLayout::_6_1, "_6_1", "6.1", "6.1"},
	{ChannelLayout::_6_1_Hex, "_6_1_Hex", "6.1(back)", "6.1(back)"},
	{ChannelLayout::_6_1_Front, "_6_1_Front", "6.1(front)", "6.1(front)"},
	{ChannelLayout::_7_0, "_7_0", "7.0", "7.0"},
	{ChannelLayout::_7_0_Front, "_7_0_Front", "7.0(front)", "7.0(front)"},
	{ChannelLayout::_7_1, "_7_1", "7.1", "7.1(alsa)"},
	{ChannelLayout::_7_1_Wide, "_7_1_Wide", "7.1(wide)", "7.1(wide)"},
	{ChannelLayout::_7_1_Side, "_7_1_Side", "7.1(side)", "7.1(wide-side)"}
}};

const std::array<ColorRangeInfo::Item, 5> ColorRangeInfo::info{{
	{ColorRange::Auto, "Auto", "auto", (int)0},
	{ColorRange::Limited, "Limited", "limited", (int)1},
	{ColorRange::Full, "Full", "full", (int)2},
	{ColorRange::Remap, "Remap", "remap", (int)3},
	{ColorRange::Extended, "Extended", "luma", (int)4}
}};

const std::array<AdjustColorInfo::Item, 9> AdjustColorInfo::info{{
	{AdjustColor::Reset, "Reset", "reset", {0, 0, 0, 0}},
	{AdjustColor::BrightnessInc, "BrightnessInc", "brightness+", {1, 0, 0, 0}},
	{AdjustColor::BrightnessDec, "BrightnessDec", "brightness-", {-1, 0, 0, 0}},
	{AdjustColor::ContrastInc, "ContrastInc", "contrast+", {0, 1, 0, 0}},
	{AdjustColor::ContrastDec, "ContrastDec", "contrast-", {0, -1, 0, 0}},
	{AdjustColor::SaturationInc, "SaturationInc", "saturation+", {0, 0, 1, 0}},
	{AdjustColor::SaturationDec, "SaturationDec", "saturation-", {0, 0, -1, 0}},
	{AdjustColor::HueInc, "HueInc", "hue+", {0, 0, 0, 1}},
	{AdjustColor::HueDec, "HueDec", "hue-", {0, 0, 0, -1}}
}};

const std::array<SubtitleDisplayInfo::Item, 2> SubtitleDisplayInfo::info{{
	{SubtitleDisplay::OnLetterbox, "OnLetterbox", "on-letterbox", (int)0},
	{SubtitleDisplay::InVideo, "InVideo", "in-video", (int)1}
}};

const std::array<VideoRatioInfo::Item, 7> VideoRatioInfo::info{{
	{VideoRatio::Source, "Source", "source", -1.0},
	{VideoRatio::Window, "Window", "window", 0.0},
	{VideoRatio::_4__3, "_4__3", "4:3", 4.0/3.0},
	{VideoRatio::_16__10, "_16__10", "16:10", 16.0/10.0},
	{VideoRatio::_16__9, "_16__9", "16:9", 16.0/9.0},
	{VideoRatio::_1_85__1, "_1_85__1", "1.85:1", 1.85},
	{VideoRatio::_2_35__1, "_2_35__1", "2.35:1", 2.35}
}};

const std::array<DitheringInfo::Item, 3> DitheringInfo::info{{
	{Dithering::None, "None", "off", (int)0},
	{Dithering::Fruit, "Fruit", "random", (int)1},
	{Dithering::Ordered, "Ordered", "ordered", (int)2}
}};

const std::array<DecoderDeviceInfo::Item, 3> DecoderDeviceInfo::info{{
	{DecoderDevice::None, "None", "", (int)0},
	{DecoderDevice::CPU, "CPU", "", (int)1},
	{DecoderDevice::GPU, "GPU", "", (int)2}
}};

const std::array<DeintModeInfo::Item, 2> DeintModeInfo::info{{
	{DeintMode::None, "None", "off", (int)0},
	{DeintMode::Auto, "Auto", "auto", (int)1}
}};

const std::array<DeintDeviceInfo::Item, 4> DeintDeviceInfo::info{{
	{DeintDevice::None, "None", "", (int)0},
	{DeintDevice::CPU, "CPU", "", (int)1},
	{DeintDevice::GPU, "GPU", "", (int)2},
	{DeintDevice::OpenGL, "OpenGL", "", (int)4}
}};

const std::array<DeintMethodInfo::Item, 8> DeintMethodInfo::info{{
	{DeintMethod::None, "None", "", (int)0},
	{DeintMethod::Bob, "Bob", "", (int)1},
	{DeintMethod::LinearBob, "LinearBob", "", (int)2},
	{DeintMethod::CubicBob, "CubicBob", "", (int)3},
	{DeintMethod::Median, "Median", "", (int)4},
	{DeintMethod::LinearBlend, "LinearBlend", "", (int)5},
	{DeintMethod::Yadif, "Yadif", "", (int)6},
	{DeintMethod::MotionAdaptive, "MotionAdaptive", "", (int)7}
}};

const std::array<InterpolatorTypeInfo::Item, 11> InterpolatorTypeInfo::info{{
	{InterpolatorType::Bilinear, "Bilinear", "bilinear", (int)0},
	{InterpolatorType::BicubicBS, "BicubicBS", "b-spline", (int)1},
	{InterpolatorType::BicubicCR, "BicubicCR", "catmull", (int)2},
	{InterpolatorType::BicubicMN, "BicubicMN", "mitchell", (int)3},
	{InterpolatorType::Spline16, "Spline16", "spline16", (int)4},
	{InterpolatorType::Spline36, "Spline36", "spline36", (int)5},
	{InterpolatorType::Spline64, "Spline64", "spline64", (int)6},
	{InterpolatorType::LanczosFast, "LanczosFast", "lanczos-fast", (int)7},
	{InterpolatorType::Lanczos2, "Lanczos2", "lancoz2", (int)8},
	{InterpolatorType::Lanczos3, "Lanczos3", "lanczos3", (int)9},
	{InterpolatorType::Lanczos4, "Lanczos4", "lanczos4", (int)10}
}};

const std::array<AudioDriverInfo::Item, 8> AudioDriverInfo::info{{
	{AudioDriver::Auto, "Auto", "", (int)0},
	{AudioDriver::CoreAudio, "CoreAudio", "", (int)1},
	{AudioDriver::PulseAudio, "PulseAudio", "", (int)2},
	{AudioDriver::OSS, "OSS", "", (int)3},
	{AudioDriver::ALSA, "ALSA", "", (int)4},
	{AudioDriver::JACK, "JACK", "", (int)5},
	{AudioDriver::PortAudio, "PortAudio", "", (int)6},
	{AudioDriver::OpenAL, "OpenAL", "", (int)7}
}};

const std::array<ClippingMethodInfo::Item, 3> ClippingMethodInfo::info{{
	{ClippingMethod::Auto, "Auto", "", (int)0},
	{ClippingMethod::Soft, "Soft", "", (int)1},
	{ClippingMethod::Hard, "Hard", "", (int)2}
}};

const std::array<StaysOnTopInfo::Item, 3> StaysOnTopInfo::info{{
	{StaysOnTop::None, "None", "off", (int)0},
	{StaysOnTop::Playing, "Playing", "playing", (int)1},
	{StaysOnTop::Always, "Always", "always", (int)2}
}};

const std::array<SeekingStepInfo::Item, 3> SeekingStepInfo::info{{
	{SeekingStep::Step1, "Step1", "", (int)0},
	{SeekingStep::Step2, "Step2", "", (int)1},
	{SeekingStep::Step3, "Step3", "", (int)2}
}};

const std::array<GeneratePlaylistInfo::Item, 2> GeneratePlaylistInfo::info{{
	{GeneratePlaylist::Similar, "Similar", "", (int)0},
	{GeneratePlaylist::Folder, "Folder", "", (int)1}
}};

const std::array<PlaylistBehaviorWhenOpenMediaInfo::Item, 3> PlaylistBehaviorWhenOpenMediaInfo::info{{
	{PlaylistBehaviorWhenOpenMedia::AppendToPlaylist, "AppendToPlaylist", "", (int)0},
	{PlaylistBehaviorWhenOpenMedia::ClearAndAppendToPlaylist, "ClearAndAppendToPlaylist", "", (int)1},
	{PlaylistBehaviorWhenOpenMedia::ClearAndGenerateNewPlaylist, "ClearAndGenerateNewPlaylist", "", (int)2}
}};

const std::array<SubtitleAutoloadInfo::Item, 3> SubtitleAutoloadInfo::info{{
	{SubtitleAutoload::Matched, "Matched", "", (int)0},
	{SubtitleAutoload::Contain, "Contain", "", (int)1},
	{SubtitleAutoload::Folder, "Folder", "", (int)2}
}};

const std::array<SubtitleAutoselectInfo::Item, 4> SubtitleAutoselectInfo::info{{
	{SubtitleAutoselect::Matched, "Matched", "", (int)0},
	{SubtitleAutoselect::First, "First", "", (int)1},
	{SubtitleAutoselect::All, "All", "", (int)2},
	{SubtitleAutoselect::EachLanguage, "EachLanguage", "", (int)3}
}};

const std::array<OsdScalePolicyInfo::Item, 3> OsdScalePolicyInfo::info{{
	{OsdScalePolicy::Width, "Width", "", (int)0},
	{OsdScalePolicy::Height, "Height", "", (int)1},
	{OsdScalePolicy::Diagonal, "Diagonal", "", (int)2}
}};

const std::array<ClickActionInfo::Item, 4> ClickActionInfo::info{{
	{ClickAction::OpenFile, "OpenFile", "", (int)0},
	{ClickAction::Fullscreen, "Fullscreen", "", (int)1},
	{ClickAction::Pause, "Pause", "", (int)2},
	{ClickAction::Mute, "Mute", "", (int)3}
}};

const std::array<WheelActionInfo::Item, 6> WheelActionInfo::info{{
	{WheelAction::Seek1, "Seek1", "", (int)0},
	{WheelAction::Seek2, "Seek2", "", (int)1},
	{WheelAction::Seek3, "Seek3", "", (int)2},
	{WheelAction::PrevNext, "PrevNext", "", (int)3},
	{WheelAction::Volume, "Volume", "", (int)4},
	{WheelAction::Amp, "Amp", "", (int)5}
}};

const std::array<KeyModifierInfo::Item, 4> KeyModifierInfo::info{{
	{KeyModifier::None, "None", "", (int)Qt::NoModifier},
	{KeyModifier::Ctrl, "Ctrl", "", (int)Qt::ControlModifier},
	{KeyModifier::Shift, "Shift", "", (int)Qt::ShiftModifier},
	{KeyModifier::Alt, "Alt", "", (int)Qt::AltModifier}
}};

const std::array<VerticalAlignmentInfo::Item, 3> VerticalAlignmentInfo::info{{
	{VerticalAlignment::Top, "Top", "top", Qt::AlignTop},
	{VerticalAlignment::Center, "Center", "v-center", Qt::AlignVCenter},
	{VerticalAlignment::Bottom, "Bottom", "bottom", Qt::AlignBottom}
}};

const std::array<HorizontalAlignmentInfo::Item, 3> HorizontalAlignmentInfo::info{{
	{HorizontalAlignment::Left, "Left", "left", Qt::AlignLeft},
	{HorizontalAlignment::Center, "Center", "h-center", Qt::AlignHCenter},
	{HorizontalAlignment::Right, "Right", "right", Qt::AlignRight}
}};

const std::array<MoveTowardInfo::Item, 5> MoveTowardInfo::info{{
	{MoveToward::Reset, "Reset", "reset", {0, 0}},
	{MoveToward::Upward, "Upward", "up", {0, -1}},
	{MoveToward::Downward, "Downward", "down", {0, 1}},
	{MoveToward::Leftward, "Leftward", "left", {-1, 0}},
	{MoveToward::Rightward, "Rightward", "right", {1, 0}}
}};

const std::array<ChangeValueInfo::Item, 3> ChangeValueInfo::info{{
	{ChangeValue::Reset, "Reset", "reset", 0},
	{ChangeValue::Increase, "Increase", "increase", 1},
	{ChangeValue::Decrease, "Decrease", "decrease", -1}
}};
