#include "openmediainfo.hpp"
#include "misc/json.hpp"

#define JSON_CLASS OpenMediaInfo

static const auto jio = JIO(JE(start_playback), JE(behavior));

JSON_DECLARE_FROM_TO_FUNCTIONS
