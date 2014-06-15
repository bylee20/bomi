#include "quicksnapshotsave.hpp"

const std::array<QuickSnapshotSaveInfo::Item, 3> QuickSnapshotSaveInfo::info{{
    {QuickSnapshotSave::Fixed, u"Fixed"_q, u""_q, (int)0},
    {QuickSnapshotSave::Current, u"Current"_q, u""_q, (int)1},
    {QuickSnapshotSave::Ask, u"Ask"_q, u""_q, (int)2}
}};
