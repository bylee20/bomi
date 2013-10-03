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
	bool apply(const VideoFrame &in, QLinkedList<VideoFrame> &queue);
protected:
	double nextPTS(int split = 2) const;
	virtual bool process(const VideoFrame &in, QLinkedList<VideoFrame> &queue) {
		Q_UNUSED(in); Q_UNUSED(queue); return 0;
	}
	struct Data;
	Data *d;
};

#endif // VIDEOFILTER_H
