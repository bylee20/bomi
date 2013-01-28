#ifndef HWACCEL_HPP
#define HWACCEL_HPP

#include "stdafx.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}

class HwAccel {
public:
	static QList<AVCodecID> fullCodecList();
	static bool supports(AVCodecID codec);
	static const char *codecName(AVCodecID id);
	~HwAccel();
private:
	HwAccel();
	friend class PlayEngine;
};

#endif // HWACCEL_HPP
