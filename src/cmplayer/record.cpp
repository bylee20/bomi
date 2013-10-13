#include "record.hpp"

template struct RecordIoOne<int>;
template struct RecordIoOne<bool>;
template struct RecordIoOne<double>;
template struct RecordIoOne<float>;
template struct RecordIoOne<QString>;
template struct RecordIoOne<QStringList>;
template struct RecordIoOne<QVariant>;
template struct RecordIoOne<QLocale>;
template struct RecordIoOne<QKeySequence>;
