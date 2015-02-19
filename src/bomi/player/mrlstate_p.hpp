#ifndef MRLSTATE_P_HPP
#define MRLSTATE_P_HPP

#include "mrlstate.hpp"
#include "enum/autoselectmode.hpp"
#include "video/deintoption.hpp"
#include "video/interpolatorparams.hpp"
#include "misc/charsetdetector.hpp"

struct MrlState::Data {
    CacheInfo cache;
    double autodetect = -1;
    bool autoselect = false, disc = false;
    AutoselectMode autoselectMode = AutoselectMode::Matched;
    QString autoselectExt;
    EncodingInfo subtitleEncoding;
    DeintOptionSet deint;
    QString audioDevice = _L("auto");
    IntrplParamSetMap intrpl, chroma;
};

#endif // MRLSTATE_P_HPP

