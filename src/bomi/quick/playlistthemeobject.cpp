#include "playlistthemeobject.hpp"
#include "misc/json.hpp"

#define JSON_CLASS PlaylistTheme
static const auto jio = JIO(
    JE(showLocation)
);

JSON_DECLARE_FROM_TO_FUNCTIONS
