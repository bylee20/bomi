#ifndef MRLSTATE_P_HPP
#define MRLSTATE_P_HPP

#include "mrlstate.hpp"
#include "enum/autoselectmode.hpp"
#include "video/deintoption.hpp"
#include "video/interpolatorparams.hpp"
#include "misc/smbauth.hpp"

struct MrlState::Data {
    CacheInfo cache;
    SmbAuth smb;
    bool autoselect = false, disc = false, preferExternal = false;
    AutoselectMode autoselectMode = AutoselectMode::Matched;
    QString autoselectExt;
    DeintOptionSet deint;
    QString audioDevice = _L("auto");
    IntrplParamSetMap intrpl, chroma, intrplDown;
};

#endif // MRLSTATE_P_HPP

