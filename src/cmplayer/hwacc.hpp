#ifndef HWACC_HPP
#define HWACC_HPP

#include "stdafx.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
}

struct lavc_ctx;		struct AVFrame;
struct mp_image;		struct vd_lavc_hwdec_functions;

class HwAcc {
public:
	static QList<AVCodecID> fullCodecList();
	static bool supports(AVCodecID codec);
	static const char *codecName(AVCodecID id);
	static const char *name();
	mp_image *getFrame(mp_image *mpi);
	~HwAcc();
private:
	HwAcc(AVCodecID codec, int width, int height);
	bool isOk() const;
	bool isOk(int width, int height) const;
	bool reinitialize(int width, int height);
	static int init(lavc_ctx *ctx);
	static void uninit(lavc_ctx *ctx);
	static mp_image *allocateImage(struct lavc_ctx *ctx, AVFrame *frame);
	friend class PlayEngine;
	friend vd_lavc_hwdec_functions create_vaapi_functions();
	struct Data;
	Data *d;
};

#endif // HWACC_HPP
