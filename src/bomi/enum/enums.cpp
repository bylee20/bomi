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
#include "autoloadmode.hpp"
#include "subtitleautoselect.hpp"
#include "keymodifier.hpp"
#include "verticalalignment.hpp"
#include "horizontalalignment.hpp"
#include "movetoward.hpp"
#include "changevalue.hpp"
#include "videoeffect.hpp"
#include "quicksnapshotsave.hpp"
#include "mousebehavior.hpp"
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
    } else    if (metaType == qMetaTypeId<AutoloadMode>()) {
        conv.variantToName = _EnumVariantToEnumName<AutoloadMode>;
        conv.nameToVariant = _EnumNameToEnumVariant<AutoloadMode>;
    } else    if (metaType == qMetaTypeId<SubtitleAutoselect>()) {
        conv.variantToName = _EnumVariantToEnumName<SubtitleAutoselect>;
        conv.nameToVariant = _EnumNameToEnumVariant<SubtitleAutoselect>;
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
