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
    QString autoselectExt, subtitleEncoding;
    DeintOption deint_swdec, deint_hwdec;
    QString audioDevice = _L("auto");
    IntrplParamSetMap intrpl, chroma;

    auto detect(const QString &file, const QString &fallback) const -> QString
    {
        if (autodetect < 0)
            return fallback;
        const auto enc = CharsetDetector::detect(file, autodetect);
        return enc.isEmpty() ? fallback : enc;
    }
    auto detect(const QString &file) const -> QString
        { return detect(file, subtitleEncoding); }
};

#endif // MRLSTATE_P_HPP

