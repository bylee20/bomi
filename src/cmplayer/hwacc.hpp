#ifndef HWACCEL_HPP
#define HWACCEL_HPP

#include "stdafx.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}

class HwAcc {
public:
	static QList<AVCodecID> fullCodecList();
	static bool supports(AVCodecID codec);
	static const char *codecName(AVCodecID id);
	~HwAcc();
private:
	HwAcc();
	friend class PlayEngine;
};

#endif // HWACCEL_HPP
