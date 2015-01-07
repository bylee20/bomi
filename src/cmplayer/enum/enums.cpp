#include "enums.hpp"
#include "textthemestyle.hpp"
#include "speakerid.hpp"
#include "channellayout.hpp"
#include "colorrange.hpp"
#include "colorspace.hpp"
#include "subtitledisplay.hpp"
#include "videoratio.hpp"
#include "dithering.hpp"
#include "decoderdevice.hpp"
#include "deintmode.hpp"
#include "deintdevice.hpp"
#include "deintmethod.hpp"
#include "interpolator.hpp"
#include "audiodriver.hpp"
#include "clippingmethod.hpp"
#include "staysontop.hpp"
#include "seekingstep.hpp"
#include "generateplaylist.hpp"
#include "openmediabehavior.hpp"
#include "subtitleautoload.hpp"
#include "subtitleautoselect.hpp"
#include "osdscalepolicy.hpp"
#include "keymodifier.hpp"
#include "verticalalignment.hpp"
#include "horizontalalignment.hpp"
#include "movetoward.hpp"
#include "changevalue.hpp"
#include "videoeffect.hpp"
#include "quicksnapshotsave.hpp"
#include "mousebehavior.hpp"
bool _IsEnumTypeId(int userType) {
    return userType == qMetaTypeId<TextThemeStyle>()
        || userType == qMetaTypeId<SpeakerId>()
        || userType == qMetaTypeId<ChannelLayout>()
        || userType == qMetaTypeId<ColorRange>()
        || userType == qMetaTypeId<ColorSpace>()
        || userType == qMetaTypeId<SubtitleDisplay>()
        || userType == qMetaTypeId<VideoRatio>()
        || userType == qMetaTypeId<Dithering>()
        || userType == qMetaTypeId<DecoderDevice>()
        || userType == qMetaTypeId<DeintMode>()
        || userType == qMetaTypeId<DeintDevice>()
        || userType == qMetaTypeId<DeintMethod>()
        || userType == qMetaTypeId<Interpolator>()
        || userType == qMetaTypeId<AudioDriver>()
        || userType == qMetaTypeId<ClippingMethod>()
        || userType == qMetaTypeId<StaysOnTop>()
        || userType == qMetaTypeId<SeekingStep>()
        || userType == qMetaTypeId<GeneratePlaylist>()
        || userType == qMetaTypeId<OpenMediaBehavior>()
        || userType == qMetaTypeId<SubtitleAutoload>()
        || userType == qMetaTypeId<SubtitleAutoselect>()
        || userType == qMetaTypeId<OsdScalePolicy>()
        || userType == qMetaTypeId<KeyModifier>()
        || userType == qMetaTypeId<VerticalAlignment>()
        || userType == qMetaTypeId<HorizontalAlignment>()
        || userType == qMetaTypeId<MoveToward>()
        || userType == qMetaTypeId<ChangeValue>()
        || userType == qMetaTypeId<VideoEffect>()
        || userType == qMetaTypeId<QuickSnapshotSave>()
        || userType == qMetaTypeId<MouseBehavior>()
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
    } else    if (varType == qMetaTypeId<ColorSpace>()) {
        toSql = _EnumVariantToSql<ColorSpace>;
        fromSql = _EnumVariantFromSql<ColorSpace>;
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
    } else    if (varType == qMetaTypeId<Interpolator>()) {
        toSql = _EnumVariantToSql<Interpolator>;
        fromSql = _EnumVariantFromSql<Interpolator>;
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
    } else    if (varType == qMetaTypeId<OpenMediaBehavior>()) {
        toSql = _EnumVariantToSql<OpenMediaBehavior>;
        fromSql = _EnumVariantFromSql<OpenMediaBehavior>;
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
    } else    if (varType == qMetaTypeId<QuickSnapshotSave>()) {
        toSql = _EnumVariantToSql<QuickSnapshotSave>;
        fromSql = _EnumVariantFromSql<QuickSnapshotSave>;
    } else    if (varType == qMetaTypeId<MouseBehavior>()) {
        toSql = _EnumVariantToSql<MouseBehavior>;
        fromSql = _EnumVariantFromSql<MouseBehavior>;
    } else
        return false;
    return true;
}
auto _EnumNameVariantConverter(int metaType) -> EnumNameVariantConverter
{
    EnumNameVariantConverter conv;
    if (metaType == qMetaTypeId<TextThemeStyle>()) {
        conv.variantToName = _EnumVariantToEnumName<TextThemeStyle>;
        conv.nameToVariant = _EnumNameToEnumVariant<TextThemeStyle>;
    } else    if (metaType == qMetaTypeId<SpeakerId>()) {
        conv.variantToName = _EnumVariantToEnumName<SpeakerId>;
        conv.nameToVariant = _EnumNameToEnumVariant<SpeakerId>;
    } else    if (metaType == qMetaTypeId<ChannelLayout>()) {
        conv.variantToName = _EnumVariantToEnumName<ChannelLayout>;
        conv.nameToVariant = _EnumNameToEnumVariant<ChannelLayout>;
    } else    if (metaType == qMetaTypeId<ColorRange>()) {
        conv.variantToName = _EnumVariantToEnumName<ColorRange>;
        conv.nameToVariant = _EnumNameToEnumVariant<ColorRange>;
    } else    if (metaType == qMetaTypeId<ColorSpace>()) {
        conv.variantToName = _EnumVariantToEnumName<ColorSpace>;
        conv.nameToVariant = _EnumNameToEnumVariant<ColorSpace>;
    } else    if (metaType == qMetaTypeId<SubtitleDisplay>()) {
        conv.variantToName = _EnumVariantToEnumName<SubtitleDisplay>;
        conv.nameToVariant = _EnumNameToEnumVariant<SubtitleDisplay>;
    } else    if (metaType == qMetaTypeId<VideoRatio>()) {
        conv.variantToName = _EnumVariantToEnumName<VideoRatio>;
        conv.nameToVariant = _EnumNameToEnumVariant<VideoRatio>;
    } else    if (metaType == qMetaTypeId<Dithering>()) {
        conv.variantToName = _EnumVariantToEnumName<Dithering>;
        conv.nameToVariant = _EnumNameToEnumVariant<Dithering>;
    } else    if (metaType == qMetaTypeId<DecoderDevice>()) {
        conv.variantToName = _EnumVariantToEnumName<DecoderDevice>;
        conv.nameToVariant = _EnumNameToEnumVariant<DecoderDevice>;
    } else    if (metaType == qMetaTypeId<DeintMode>()) {
        conv.variantToName = _EnumVariantToEnumName<DeintMode>;
        conv.nameToVariant = _EnumNameToEnumVariant<DeintMode>;
    } else    if (metaType == qMetaTypeId<DeintDevice>()) {
        conv.variantToName = _EnumVariantToEnumName<DeintDevice>;
        conv.nameToVariant = _EnumNameToEnumVariant<DeintDevice>;
    } else    if (metaType == qMetaTypeId<DeintMethod>()) {
        conv.variantToName = _EnumVariantToEnumName<DeintMethod>;
        conv.nameToVariant = _EnumNameToEnumVariant<DeintMethod>;
    } else    if (metaType == qMetaTypeId<Interpolator>()) {
        conv.variantToName = _EnumVariantToEnumName<Interpolator>;
        conv.nameToVariant = _EnumNameToEnumVariant<Interpolator>;
    } else    if (metaType == qMetaTypeId<AudioDriver>()) {
        conv.variantToName = _EnumVariantToEnumName<AudioDriver>;
        conv.nameToVariant = _EnumNameToEnumVariant<AudioDriver>;
    } else    if (metaType == qMetaTypeId<ClippingMethod>()) {
        conv.variantToName = _EnumVariantToEnumName<ClippingMethod>;
        conv.nameToVariant = _EnumNameToEnumVariant<ClippingMethod>;
    } else    if (metaType == qMetaTypeId<StaysOnTop>()) {
        conv.variantToName = _EnumVariantToEnumName<StaysOnTop>;
        conv.nameToVariant = _EnumNameToEnumVariant<StaysOnTop>;
    } else    if (metaType == qMetaTypeId<SeekingStep>()) {
        conv.variantToName = _EnumVariantToEnumName<SeekingStep>;
        conv.nameToVariant = _EnumNameToEnumVariant<SeekingStep>;
    } else    if (metaType == qMetaTypeId<GeneratePlaylist>()) {
        conv.variantToName = _EnumVariantToEnumName<GeneratePlaylist>;
        conv.nameToVariant = _EnumNameToEnumVariant<GeneratePlaylist>;
    } else    if (metaType == qMetaTypeId<OpenMediaBehavior>()) {
        conv.variantToName = _EnumVariantToEnumName<OpenMediaBehavior>;
        conv.nameToVariant = _EnumNameToEnumVariant<OpenMediaBehavior>;
    } else    if (metaType == qMetaTypeId<SubtitleAutoload>()) {
        conv.variantToName = _EnumVariantToEnumName<SubtitleAutoload>;
        conv.nameToVariant = _EnumNameToEnumVariant<SubtitleAutoload>;
    } else    if (metaType == qMetaTypeId<SubtitleAutoselect>()) {
        conv.variantToName = _EnumVariantToEnumName<SubtitleAutoselect>;
        conv.nameToVariant = _EnumNameToEnumVariant<SubtitleAutoselect>;
    } else    if (metaType == qMetaTypeId<OsdScalePolicy>()) {
        conv.variantToName = _EnumVariantToEnumName<OsdScalePolicy>;
        conv.nameToVariant = _EnumNameToEnumVariant<OsdScalePolicy>;
    } else    if (metaType == qMetaTypeId<KeyModifier>()) {
        conv.variantToName = _EnumVariantToEnumName<KeyModifier>;
        conv.nameToVariant = _EnumNameToEnumVariant<KeyModifier>;
    } else    if (metaType == qMetaTypeId<VerticalAlignment>()) {
        conv.variantToName = _EnumVariantToEnumName<VerticalAlignment>;
        conv.nameToVariant = _EnumNameToEnumVariant<VerticalAlignment>;
    } else    if (metaType == qMetaTypeId<HorizontalAlignment>()) {
        conv.variantToName = _EnumVariantToEnumName<HorizontalAlignment>;
        conv.nameToVariant = _EnumNameToEnumVariant<HorizontalAlignment>;
    } else    if (metaType == qMetaTypeId<MoveToward>()) {
        conv.variantToName = _EnumVariantToEnumName<MoveToward>;
        conv.nameToVariant = _EnumNameToEnumVariant<MoveToward>;
    } else    if (metaType == qMetaTypeId<ChangeValue>()) {
        conv.variantToName = _EnumVariantToEnumName<ChangeValue>;
        conv.nameToVariant = _EnumNameToEnumVariant<ChangeValue>;
    } else    if (metaType == qMetaTypeId<VideoEffect>()) {
        conv.variantToName = _EnumVariantToEnumName<VideoEffect>;
        conv.nameToVariant = _EnumNameToEnumVariant<VideoEffect>;
    } else    if (metaType == qMetaTypeId<QuickSnapshotSave>()) {
        conv.variantToName = _EnumVariantToEnumName<QuickSnapshotSave>;
        conv.nameToVariant = _EnumNameToEnumVariant<QuickSnapshotSave>;
    } else    if (metaType == qMetaTypeId<MouseBehavior>()) {
        conv.variantToName = _EnumVariantToEnumName<MouseBehavior>;
        conv.nameToVariant = _EnumNameToEnumVariant<MouseBehavior>;
    } else
        return EnumNameVariantConverter();
    return conv;
}
