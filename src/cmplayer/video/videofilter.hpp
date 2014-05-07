#ifndef VIDEOFILTER_H
#define VIDEOFILTER_H

#include <QLinkedList>
#include "videoframe.hpp"

enum class VFType { // video filter type
    NoiseReduction
};

class VideoFilter {
public:
    VideoFilter();
    VideoFilter(const VideoFilter &) = delete;
    VideoFilter &operator = (const VideoFilter &) = delete;
    virtual ~VideoFilter();
protected:
    auto setNewPts(double pts) -> void;
    double nextPts(int split = 2) const;
    struct Data;
    Data *d;
};

#endif // VIDEOFILTER_H
