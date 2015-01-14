#include "speakerid.hpp"

const std::array<SpeakerIdInfo::Item, 11> SpeakerIdInfo::info{{
    {SpeakerId::FrontLeft, u"FrontLeft"_q, u""_q, MP_SPEAKER_ID_FL},
    {SpeakerId::FrontRight, u"FrontRight"_q, u""_q, MP_SPEAKER_ID_FR},
    {SpeakerId::FrontCenter, u"FrontCenter"_q, u""_q, MP_SPEAKER_ID_FC},
    {SpeakerId::LowFrequency, u"LowFrequency"_q, u""_q, MP_SPEAKER_ID_LFE},
    {SpeakerId::BackLeft, u"BackLeft"_q, u""_q, MP_SPEAKER_ID_BL},
    {SpeakerId::BackRight, u"BackRight"_q, u""_q, MP_SPEAKER_ID_BR},
    {SpeakerId::FrontLeftCenter, u"FrontLeftCenter"_q, u""_q, MP_SPEAKER_ID_FLC},
    {SpeakerId::FrontRightCenter, u"FrontRightCenter"_q, u""_q, MP_SPEAKER_ID_FRC},
    {SpeakerId::BackCenter, u"BackCenter"_q, u""_q, MP_SPEAKER_ID_BC},
    {SpeakerId::SideLeft, u"SideLeft"_q, u""_q, MP_SPEAKER_ID_SL},
    {SpeakerId::SideRight, u"SideRight"_q, u""_q, MP_SPEAKER_ID_SR}
}};
