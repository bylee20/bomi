#ifndef HWACC_HPP
#define HWACC_HPP

#include "stdafx.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}

class HwAcc {
public:
	static QList<AVCodecID> fullCodecList();
	static bool supports(AVCodecID codec);
	static const char *codecName(AVCodecID id);
	static const char *name();
private:
	HwAcc();
	friend class PlayEngine;
};

#endif // HWACC_HPP
