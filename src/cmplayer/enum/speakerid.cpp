#include "speakerid.hpp"

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
