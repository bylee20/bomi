#ifndef HWACCEL_H
#define HWACCEL_H

#include <QtCore/QDebug>
#ifndef UINT64_C
#define UINT64_C Q_UINT64_C
#endif
extern "C" {
#ifdef Q_WS_MAC
#include <libavcodec/vda.h>
#else
#include <libavcodec/vaapi.h>
#include <va/va.h>
#include <va/va_x11.h>
#endif
#include <libavcodec/avcodec.h>
#include <libmpcodecs/img_format.h>
#include <libmpdemux/stheader.h>
#include <libmpcodecs/mp_image.h>
#include "codec-cfg.h"

int cmplaer_hwaccel_available(sh_video_t *sh);
void *cmplayer_hwaccel_create(AVCodecContext *avctx);
void cmplayer_hwaccel_delete(void **hwaccel);
int cmplayer_hwaccel_fill_image(void *hwaccel, mp_image_t *image, AVFrame *frame);
extern void *cmplayer_hwaccel_get(AVCodecContext *avctx);
extern uint32_t cmplayer_hwaccel_setup(void *hwaccel);
extern int init_vo(sh_video_t *sh, enum PixelFormat pix_fmt);
}

class HwAccel {
#ifdef Q_WS_MAC
typedef struct vda_context context;
#else
typedef struct vaapi_context context;
#endif
public:
	~HwAccel();
	static HwAccel *create(AVCodecContext *avctx);
	bool setup();
	bool fill(mp_image_t *image, AVFrame *frame);
private:
	bool ctorContext(AVCodecContext *avctx);
	void dtorContext();
	static sh_video_t *_sh(AVCodecContext *avctx) {return reinterpret_cast<sh_video_t*>(avctx->opaque);}
	static HwAccel *get(AVCodecContext *avctx) {return reinterpret_cast<HwAccel*>(cmplayer_hwaccel_get(avctx));}
	static PixelFormat getFormat(AVCodecContext *avctx, const PixelFormat *pixFmt);
	static int getBuffer(AVCodecContext *avctx, AVFrame *frame);
	static int regetBuffer(AVCodecContext *avctx, AVFrame *frame);
	static void releaseBuffer(AVCodecContext *avctx, AVFrame *frame);

	HwAccel() {memset(&ctx, 0, sizeof(ctx));}
#ifndef Q_WS_MAC
	struct VaSurface {VASurfaceID  id{VA_INVALID_SURFACE}; int ref{0}; uint order{0};};
	QVector<VaSurface> surfaces;
	Display *x11 = nullptr;
	VAImage image;
	QByteArray buffer[3];
	int totalOrder{0};
#endif
	uint32_t imgFmt{0};
	AVCodecContext *avctx = nullptr;
	context ctx;
	int width{0};
	int height{0};
	bool initCtx{false};
	friend uint32_t cmplayer_hwaccel_setup(void*);
};

#endif // HWACCEL_H
