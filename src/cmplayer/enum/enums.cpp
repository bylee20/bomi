#include "enums.hpp"
#include "textthemestyle.hpp"
#include "speakerid.hpp"
#include "channellayout.hpp"
#include "colorrange.hpp"
#include "adjustcolor.hpp"
#include "subtitledisplay.hpp"
#include "videoratio.hpp"
#include "dithering.hpp"
#include "decoderdevice.hpp"
#include "deintmode.hpp"
#include "deintdevice.hpp"
#include "deintmethod.hpp"
#include "interpolatortype.hpp"
#include "audiodriver.hpp"
#include "clippingmethod.hpp"
#include "staysontop.hpp"
#include "seekingstep.hpp"
#include "generateplaylist.hpp"
#include "playlistbehaviorwhenopenmedia.hpp"
#include "subtitleautoload.hpp"
#include "subtitleautoselect.hpp"
#include "osdscalepolicy.hpp"
#include "keymodifier.hpp"
#include "verticalalignment.hpp"
#include "horizontalalignment.hpp"
#include "movetoward.hpp"
#include "changevalue.hpp"
#include "videoeffect.hpp"
bool _IsEnumTypeId(int userType) {
    return userType == qMetaTypeId<TextThemeStyle>()
        || userType == qMetaTypeId<SpeakerId>()
        || userType == qMetaTypeId<ChannelLayout>()
        || userType == qMetaTypeId<ColorRange>()
        || userType == qMetaTypeId<AdjustColor>()
        || userType == qMetaTypeId<SubtitleDisplay>()
        || userType == qMetaTypeId<VideoRatio>()
        || userType == qMetaTypeId<Dithering>()
        || userType == qMetaTypeId<DecoderDevice>()
        || userType == qMetaTypeId<DeintMode>()
        || userType == qMetaTypeId<DeintDevice>()
        || userType == qMetaTypeId<DeintMethod>()
        || userType == qMetaTypeId<InterpolatorType>()
        || userType == qMetaTypeId<AudioDriver>()
        || userType == qMetaTypeId<ClippingMethod>()
        || userType == qMetaTypeId<StaysOnTop>()
        || userType == qMetaTypeId<SeekingStep>()
        || userType == qMetaTypeId<GeneratePlaylist>()
        || userType == qMetaTypeId<PlaylistBehaviorWhenOpenMedia>()
        || userType == qMetaTypeId<SubtitleAutoload>()
        || userType == qMetaTypeId<SubtitleAutoselect>()
        || userType == qMetaTypeId<OsdScalePolicy>()
        || userType == qMetaTypeId<KeyModifier>()
        || userType == qMetaTypeId<VerticalAlignment>()
        || userType == qMetaTypeId<HorizontalAlignment>()
        || userType == qMetaTypeId<MoveToward>()
        || userType == qMetaTypeId<ChangeValue>()
        || userType == qMetaTypeId<VideoEffect>()
        || false;
}

bool _GetEnumFunctionsForSql(int varType, EnumVariantToSqlFunc &toSql, EnumVariantFromSqlFunc &fromSql) {
    if (varType == qMetaTypeId<TextThemeStyle>()) {
        toSql = _EnumVariantToSql<TextThemeStyle>;
        fromSql = _EnumVariantFromSql<TextThemeStyle>;
    } else    if (varType == qMetaTypeId<SpeakerId>()) {
        toSql = _EnumVariantToSql<SpeakerId>;
        fromSql = _EnumVariantFromSql<SpeakerId>;
    } else    if (varType == qMetaTypeId<ChannelLayout>()) {
        toSql = _EnumVariantToSql<ChannelLayout>;
        fromSql = _EnumVariantFromSql<ChannelLayout>;
    } else    if (varType == qMetaTypeId<ColorRange>()) {
        toSql = _EnumVariantToSql<ColorRange>;
        fromSql = _EnumVariantFromSql<ColorRange>;
    } else    if (varType == qMetaTypeId<AdjustColor>()) {
        toSql = _EnumVariantToSql<AdjustColor>;
        fromSql = _EnumVariantFromSql<AdjustColor>;
    } else    if (varType == qMetaTypeId<SubtitleDisplay>()) {
        toSql = _EnumVariantToSql<SubtitleDisplay>;
        fromSql = _EnumVariantFromSql<SubtitleDisplay>;
    } else    if (varType == qMetaTypeId<VideoRatio>()) {
        toSql = _EnumVariantToSql<VideoRatio>;
        fromSql = _EnumVariantFromSql<VideoRatio>;
    } else    if (varType == qMetaTypeId<Dithering>()) {
        toSql = _EnumVariantToSql<Dithering>;
        fromSql = _EnumVariantFromSql<Dithering>;
    } else    if (varType == qMetaTypeId<DecoderDevice>()) {
        toSql = _EnumVariantToSql<DecoderDevice>;
        fromSql = _EnumVariantFromSql<DecoderDevice>;
    } else    if (varType == qMetaTypeId<DeintMode>()) {
        toSql = _EnumVariantToSql<DeintMode>;
        fromSql = _EnumVariantFromSql<DeintMode>;
    } else    if (varType == qMetaTypeId<DeintDevice>()) {
        toSql = _EnumVariantToSql<DeintDevice>;
        fromSql = _EnumVariantFromSql<DeintDevice>;
    } else    if (varType == qMetaTypeId<DeintMethod>()) {
        toSql = _EnumVariantToSql<DeintMethod>;
        fromSql = _EnumVariantFromSql<DeintMethod>;
    } else    if (varType == qMetaTypeId<InterpolatorType>()) {
        toSql = _EnumVariantToSql<InterpolatorType>;
        fromSql = _EnumVariantFromSql<InterpolatorType>;
    } else    if (varType == qMetaTypeId<AudioDriver>()) {
        toSql = _EnumVariantToSql<AudioDriver>;
        fromSql = _EnumVariantFromSql<AudioDriver>;
    } else    if (varType == qMetaTypeId<ClippingMethod>()) {
        toSql = _EnumVariantToSql<ClippingMethod>;
        fromSql = _EnumVariantFromSql<ClippingMethod>;
    } else    if (varType == qMetaTypeId<StaysOnTop>()) {
        toSql = _EnumVariantToSql<StaysOnTop>;
        fromSql = _EnumVariantFromSql<StaysOnTop>;
    } else    if (varType == qMetaTypeId<SeekingStep>()) {
        toSql = _EnumVariantToSql<SeekingStep>;
        fromSql = _EnumVariantFromSql<SeekingStep>;
    } else    if (varType == qMetaTypeId<GeneratePlaylist>()) {
        toSql = _EnumVariantToSql<GeneratePlaylist>;
        fromSql = _EnumVariantFromSql<GeneratePlaylist>;
    } else    if (varType == qMetaTypeId<PlaylistBehaviorWhenOpenMedia>()) {
        toSql = _EnumVariantToSql<PlaylistBehaviorWhenOpenMedia>;
        fromSql = _EnumVariantFromSql<PlaylistBehaviorWhenOpenMedia>;
    } else    if (varType == qMetaTypeId<SubtitleAutoload>()) {
        toSql = _EnumVariantToSql<SubtitleAutoload>;
        fromSql = _EnumVariantFromSql<SubtitleAutoload>;
    } else    if (varType == qMetaTypeId<SubtitleAutoselect>()) {
        toSql = _EnumVariantToSql<SubtitleAutoselect>;
        fromSql = _EnumVariantFromSql<SubtitleAutoselect>;
    } else    if (varType == qMetaTypeId<OsdScalePolicy>()) {
        toSql = _EnumVariantToSql<OsdScalePolicy>;
        fromSql = _EnumVariantFromSql<OsdScalePolicy>;
    } else    if (varType == qMetaTypeId<KeyModifier>()) {
        toSql = _EnumVariantToSql<KeyModifier>;
        fromSql = _EnumVariantFromSql<KeyModifier>;
    } else    if (varType == qMetaTypeId<VerticalAlignment>()) {
        toSql = _EnumVariantToSql<VerticalAlignment>;
        fromSql = _EnumVariantFromSql<VerticalAlignment>;
    } else    if (varType == qMetaTypeId<HorizontalAlignment>()) {
        toSql = _EnumVariantToSql<HorizontalAlignment>;
        fromSql = _EnumVariantFromSql<HorizontalAlignment>;
    } else    if (varType == qMetaTypeId<MoveToward>()) {
        toSql = _EnumVariantToSql<MoveToward>;
        fromSql = _EnumVariantFromSql<MoveToward>;
    } else    if (varType == qMetaTypeId<ChangeValue>()) {
        toSql = _EnumVariantToSql<ChangeValue>;
        fromSql = _EnumVariantFromSql<ChangeValue>;
    } else    if (varType == qMetaTypeId<VideoEffect>()) {
        toSql = _EnumVariantToSql<VideoEffect>;
        fromSql = _EnumVariantFromSql<VideoEffect>;
    } else
        return false;
    return true;
}