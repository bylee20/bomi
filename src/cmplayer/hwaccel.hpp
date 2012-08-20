#ifndef HWACCEL_H
#define HWACCEL_H

#include <QtCore/QDebug>
#include <QtOpenGL/QGLWidget>
#ifndef UINT64_C
#define UINT64_C Q_UINT64_C
#endif
extern "C" {
#ifdef Q_WS_MAC
#include <libavcodec/vda.h>
#else
#include <libavcodec/vaapi.h>
#include <va/va_glx.h>
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

extern QGLWidget *glContext;

class HwAccel {
#ifdef Q_WS_MAC
typedef struct vda_context context;
#else
typedef struct vaapi_context context;
#endif
public:
	HwAccel(AVCodecContext *avctx);
	~HwAccel();
	bool setup();
	bool fill(mp_image_t *m_vaImage, AVFrame *frame);
	bool getBuffer(AVFrame *frame);
	void releaseBuffer(AVFrame *frame);
	bool isUsable() const {return m_usable;}
private:
	void makeCurrent() {
		if (!(m_wasCurrent = QGLContext::currentContext() == glContext->context()))
			glContext->makeCurrent();
	}
	void doneCurrent() {
		if (!m_wasCurrent)
			glContext->doneCurrent();
	}

	void initializeContext();
	bool ctorContext(AVCodecContext *avctx);
	void dtorContext();
	static sh_video_t *_sh(AVCodecContext *avctx) {return reinterpret_cast<sh_video_t*>(avctx->opaque);}
	static HwAccel *get(AVCodecContext *avctx) {return reinterpret_cast<HwAccel*>(cmplayer_hwaccel_get(avctx));}
	static PixelFormat getFormat(AVCodecContext *avctx, const PixelFormat *pixFmt);
	static int getBuffer(AVCodecContext *avctx, AVFrame *frame);
	static int regetBuffer(AVCodecContext *avctx, AVFrame *frame);
	static void releaseBuffer(AVCodecContext *avctx, AVFrame *frame);
#ifdef Q_WS_X11
	struct VaSurface {
		VaSurface() {image.image_id = image.buf = VA_INVALID_ID;}
		VASurfaceID id = VA_INVALID_SURFACE;
		VAImage image;
		bool bound = false; /* Flag: image bound to the surface? */
	};
	QVector<VaSurface> m_surfaces;
	Display *m_x11 = nullptr;
	QByteArray m_buffer[3];
	VADisplay m_display = 0;
	VAConfigID m_configId = VA_INVALID_ID;
	GLuint m_texture;
	bool m_wasCurrent = false;
	bool m_derived = false;
	void *m_glSurface = nullptr;
#endif
	uint32_t m_imgFmt = {0};
	AVCodecContext *m_avctx = nullptr;
	context m_ctx;
	int m_width = {0};
	int m_height = {0};
	bool initCtx = {false};
	friend uint32_t cmplayer_hwaccel_setup(void*);
	bool m_usable = false;
};

#endif // HWACCEL_H

//#ifndef HWACCEL_H
//#define HWACCEL_H

//#include <QtCore/QDebug>
//#include <QtOpenGL/QGLWidget>

//extern QGLWidget *glContext;

//#ifndef UINT64_C
//#define UINT64_C Q_UINT64_C
//#endif
//extern "C" {
//#ifdef Q_WS_MAC
//#include <libavcodec/vda.h>
//#else
//#include <libavcodec/vaapi.h>
//#include <va/va_x11.h>
//#endif
//#include <libavcodec/avcodec.h>
//#include <libmpcodecs/img_format.h>
//#include <libmpdemux/stheader.h>
//#include <libmpcodecs/mp_image.h>
//#include "codec-cfg.h"

//int cmplaer_hwaccel_available(sh_video_t *sh);
//void *cmplayer_hwaccel_create(AVCodecContext *avctx);
//void cmplayer_hwaccel_delete(void **hwaccel);
//int cmplayer_hwaccel_fill_image(void *hwaccel, mp_image_t *image, AVFrame *frame);
//extern void *cmplayer_hwaccel_get(AVCodecContext *avctx);
//extern uint32_t cmplayer_hwaccel_setup(void *hwaccel);
//extern int init_vo(sh_video_t *sh, enum PixelFormat pix_fmt);
//}

//class HwAccel {
//#ifdef Q_WS_MAC
//typedef struct vda_context context;
//#else
//typedef struct vaapi_context context;
//#endif
//public:
//	~HwAccel();
//	static HwAccel *create(AVCodecContext *avctx);
//	bool setup();
//	bool fill(mp_image_t *image, AVFrame *frame);
//	GLuint copyTexture(VASurfaceID id);
//private:
//	void makeCurrent() {
//		if (QGLContext::currentContext() != glContext->context()) {
//			glContext->makeCurrent();
//			wasCurrent = false;
//		} else
//			wasCurrent = true;
//	}
//	void doneCurrent() {
//		if (!wasCurrent) {
//			glContext->doneCurrent();
//		}
//	}

//	bool ctorContext(AVCodecContext *avctx);
//	void dtorContext();
//	static sh_video_t *_sh(AVCodecContext *avctx) {return reinterpret_cast<sh_video_t*>(avctx->opaque);}
//	static HwAccel *get(AVCodecContext *avctx) {return reinterpret_cast<HwAccel*>(cmplayer_hwaccel_get(avctx));}
//	static PixelFormat getFormat(AVCodecContext *avctx, const PixelFormat *pixFmt);
//	static int getBuffer(AVCodecContext *avctx, AVFrame *frame) {return get(avctx)->getBuffer(frame);}
//	static int regetBuffer(AVCodecContext *avctx, AVFrame *frame);
//	static void releaseBuffer(AVCodecContext *avctx, AVFrame *frame);
//	int getBuffer(AVFrame *frame);

//	HwAccel(AVCodecContext *avctx);
//#ifndef Q_WS_MAC
////	struct VaSurface {VASurfaceID  id{VA_INVALID_SURFACE}; int ref{0}; uint order{0};};
//	struct VaSurface {
//		VaSurface(VASurfaceID id): id(id) {
//			image.image_id = image.buf = VA_INVALID_ID;
//		}
//		VASurfaceID id = VA_INVALID_SURFACE;
//		VAImage image;
//		bool bound = false; /* Flag: image bound to the surface? */
//	};

//	QVector<VaSurface*> m_surfaces;
//	QVector<VASurfaceID> m_surfaceIds;
//	QVector<VAImageFormat> m_formats;
//	QVector<VAProfile> m_profiles;
//	QVector<VAEntrypoint> m_entries;
//	Display *m_x11 = nullptr;
////	VAImage image;
//	QByteArray m_buffer[3];
//	int totalOrder = 0;
//	GLuint m_texture = GL_NONE;
//	void *m_glSurface = nullptr;
//	bool wasCurrent = false;
//	int m_surfaceHead = 0, m_surfaceTail = 0;
//	VaSurface *m_currentSurface = nullptr;
//	bool m_dr = false;
////	QVector<VaSurface*> m_outputSurfaces;
////	int m_outputSurface = 0;
//#endif
//	uint32_t imgFmt = {0};
//	AVCodecContext *avctx = nullptr;
//	context m_ctx;
//	int m_width{0};
//	int m_height{0};
//	bool initCtx{false};
//	friend uint32_t cmplayer_hwaccel_setup(void*);
//};

//#endif // HWACCEL_H
