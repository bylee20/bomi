#include "enums.hpp"
const std::array<AudioDriverInfo::Item, 7> AudioDriverInfo::info{{
	{AudioDriver::Auto, "Auto"},
	{AudioDriver::CoreAudio, "CoreAudio"},
	{AudioDriver::PulseAudio, "PulseAudio"},
	{AudioDriver::ALSA, "ALSA"},
	{AudioDriver::JACK, "JACK"},
	{AudioDriver::PortAudio, "PortAudio"},
	{AudioDriver::OpenAL, "OpenAL"}
}};

const std::array<SoftwareVolumeInfo::Item, 3> SoftwareVolumeInfo::info{{
	{SoftwareVolume::Auto, "Auto"},
	{SoftwareVolume::Always, "Always"},
	{SoftwareVolume::Never, "Never"}
}};

const std::array<ClippingMethodInfo::Item, 3> ClippingMethodInfo::info{{
	{ClippingMethod::Auto, "Auto"},
	{ClippingMethod::Soft, "Soft"},
	{ClippingMethod::Hard, "Hard"}
}};

const std::array<DeintModeInfo::Item, 3> DeintModeInfo::info{{
	{DeintMode::Never, "Never"},
	{DeintMode::Auto, "Auto"},
	{DeintMode::Always, "Always"}
}};

const std::array<StaysOnTopInfo::Item, 3> StaysOnTopInfo::info{{
	{StaysOnTop::Always, "Always"},
	{StaysOnTop::Playing, "Playing"},
	{StaysOnTop::Never, "Never"}
}};

const std::array<SeekingStepInfo::Item, 3> SeekingStepInfo::info{{
	{SeekingStep::Step1, "Step1"},
	{SeekingStep::Step2, "Step2"},
	{SeekingStep::Step3, "Step3"}
}};

const std::array<GeneratePlaylistInfo::Item, 2> GeneratePlaylistInfo::info{{
	{GeneratePlaylist::Similar, "Similar"},
	{GeneratePlaylist::Folder, "Folder"}
}};

const std::array<PlaylistBehaviorWhenOpenMediaInfo::Item, 3> PlaylistBehaviorWhenOpenMediaInfo::info{{
	{PlaylistBehaviorWhenOpenMedia::AppendToPlaylist, "AppendToPlaylist"},
	{PlaylistBehaviorWhenOpenMedia::ClearAndAppendToPlaylist, "ClearAndAppendToPlaylist"},
	{PlaylistBehaviorWhenOpenMedia::ClearAndGenerateNewPlaylist, "ClearAndGenerateNewPlaylist"}
}};

const std::array<SubtitleAutoloadInfo::Item, 3> SubtitleAutoloadInfo::info{{
	{SubtitleAutoload::Matched, "Matched"},
	{SubtitleAutoload::Contain, "Contain"},
	{SubtitleAutoload::Folder, "Folder"}
}};

const std::array<SubtitleAutoselectInfo::Item, 4> SubtitleAutoselectInfo::info{{
	{SubtitleAutoselect::Matched, "Matched"},
	{SubtitleAutoselect::First, "First"},
	{SubtitleAutoselect::All, "All"},
	{SubtitleAutoselect::EachLanguage, "EachLanguage"}
}};

const std::array<OsdScalePolicyInfo::Item, 3> OsdScalePolicyInfo::info{{
	{OsdScalePolicy::Width, "Width"},
	{OsdScalePolicy::Height, "Height"},
	{OsdScalePolicy::Diagonal, "Diagonal"}
}};

const std::array<ClickActionInfo::Item, 4> ClickActionInfo::info{{
	{ClickAction::OpenFile, "OpenFile"},
	{ClickAction::Fullscreen, "Fullscreen"},
	{ClickAction::Pause, "Pause"},
	{ClickAction::Mute, "Mute"}
}};

const std::array<WheelActionInfo::Item, 6> WheelActionInfo::info{{
	{WheelAction::Seek1, "Seek1"},
	{WheelAction::Seek2, "Seek2"},
	{WheelAction::Seek3, "Seek3"},
	{WheelAction::PrevNext, "PrevNext"},
	{WheelAction::Volume, "Volume"},
	{WheelAction::Amp, "Amp"}
}};

const std::array<KeyModifierInfo::Item, 4> KeyModifierInfo::info{{
	{KeyModifier::None, "None"},
	{KeyModifier::Ctrl, "Ctrl"},
	{KeyModifier::Shift, "Shift"},
	{KeyModifier::Alt, "Alt"}
}};

const std::array<PositionInfo::Item, 9> PositionInfo::info{{
	{Position::CC, "CC"},
	{Position::TL, "TL"},
	{Position::TC, "TC"},
	{Position::TR, "TR"},
	{Position::CL, "CL"},
	{Position::CR, "CR"},
	{Position::BL, "BL"},
	{Position::BC, "BC"},
	{Position::BR, "BR"}
}};
