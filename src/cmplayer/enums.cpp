#include "enums.hpp"

namespace Enum {
StaysOnTop::Map StaysOnTop::_map;
const StaysOnTop StaysOnTop::Always(0, "Always");
const StaysOnTop StaysOnTop::Playing(1, "Playing");
const StaysOnTop StaysOnTop::Never(2, "Never");
}

namespace Enum {
SeekingStep::Map SeekingStep::_map;
const SeekingStep SeekingStep::Step1(0, "Step1");
const SeekingStep SeekingStep::Step2(1, "Step2");
const SeekingStep SeekingStep::Step3(2, "Step3");
}

namespace Enum {
Overlay::Map Overlay::_map;
const Overlay Overlay::Auto(0, "Auto");
const Overlay Overlay::FramebufferObject(1, "FramebufferObject");
const Overlay Overlay::Pixmap(2, "Pixmap");
}

namespace Enum {
GeneratePlaylist::Map GeneratePlaylist::_map;
const GeneratePlaylist GeneratePlaylist::Similar(0, "Similar");
const GeneratePlaylist GeneratePlaylist::Folder(1, "Folder");
}

namespace Enum {
SubtitleAutoload::Map SubtitleAutoload::_map;
const SubtitleAutoload SubtitleAutoload::Matched(0, "Matched");
const SubtitleAutoload SubtitleAutoload::Contain(1, "Contain");
const SubtitleAutoload SubtitleAutoload::Folder(2, "Folder");
}

namespace Enum {
SubtitleAutoselect::Map SubtitleAutoselect::_map;
const SubtitleAutoselect SubtitleAutoselect::Matched(0, "Matched");
const SubtitleAutoselect SubtitleAutoselect::First(1, "First");
const SubtitleAutoselect SubtitleAutoselect::All(2, "All");
const SubtitleAutoselect SubtitleAutoselect::EachLanguage(3, "EachLanguage");
}

namespace Enum {
OsdScalePolicy::Map OsdScalePolicy::_map;
const OsdScalePolicy OsdScalePolicy::Width(0, "Width");
const OsdScalePolicy OsdScalePolicy::Height(1, "Height");
const OsdScalePolicy OsdScalePolicy::Diagonal(2, "Diagonal");
}

namespace Enum {
ClickAction::Map ClickAction::_map;
const ClickAction ClickAction::OpenFile(0, "OpenFile");
const ClickAction ClickAction::Fullscreen(1, "Fullscreen");
const ClickAction ClickAction::Pause(2, "Pause");
const ClickAction ClickAction::Mute(3, "Mute");
}

namespace Enum {
WheelAction::Map WheelAction::_map;
const WheelAction WheelAction::Seek1(0, "Seek1");
const WheelAction WheelAction::Seek2(1, "Seek2");
const WheelAction WheelAction::Seek3(2, "Seek3");
const WheelAction WheelAction::PrevNext(3, "PrevNext");
const WheelAction WheelAction::Volume(4, "Volume");
const WheelAction WheelAction::Amp(5, "Amp");
}

namespace Enum {
KeyModifier::Map KeyModifier::_map;
const KeyModifier KeyModifier::None(Qt::NoModifier, "None");
const KeyModifier KeyModifier::Ctrl(Qt::ControlModifier, "Ctrl");
const KeyModifier KeyModifier::Shift(Qt::ShiftModifier, "Shift");
const KeyModifier KeyModifier::Alt(Qt::AltModifier, "Alt");
}

namespace Enum {
Position::Map Position::_map;
const Position Position::CC(Qt::AlignVCenter|Qt::AlignHCenter, "CC");
const Position Position::TL(Qt::AlignTop|Qt::AlignLeft, "TL");
const Position Position::TC(Qt::AlignTop|Qt::AlignHCenter, "TC");
const Position Position::TR(Qt::AlignTop|Qt::AlignRight, "TR");
const Position Position::CL(Qt::AlignVCenter|Qt::AlignLeft, "CL");
const Position Position::CR(Qt::AlignVCenter|Qt::AlignRight, "CR");
const Position Position::BL(Qt::AlignBottom|Qt::AlignLeft, "BL");
const Position Position::BC(Qt::AlignBottom|Qt::AlignHCenter, "BC");
const Position Position::BR(Qt::AlignBottom|Qt::AlignRight, "BR");
}

