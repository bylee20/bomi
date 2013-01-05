#ifndef HWACCEL_HPP
#define HWACCEL_HPP

#include "stdafx.hpp"

extern "C" {
#ifdef Q_WS_MAC
#include <libavcodec/vda.h>
#endif
#ifdef Q_WS_X11
#include <libavcodec/vaapi.h>
#include <va/va_glx.h>
#endif
#include <libavcodec/avcodec.h>
#include <libmpcodecs/mp_image.h>
#include <libmpcodecs/img_format.h>
#ifdef None
#undef None
#endif
int is_hwaccel_available(AVCodecContext*);
}

class HwAccelInfo {
public:
	HwAccelInfo(const HwAccelInfo&) = delete;
	HwAccelInfo &operator = (const HwAccelInfo&) = delete;
	static HwAccelInfo &get() {return *obj;}
	~HwAccelInfo();
	bool isAvailable() const {return m_ok;}
	AVCodecContext *avctx() const {return m_avctx;}
	QList<AVCodecID> fullCodecList() const;
	bool supports(AVCodecID codec) const;
#ifdef Q_WS_X11
	VADisplay display() const {return m_display;}
	VAProfile find(CodecID codec, int &surfaceCount) const;
	static const PixelFormat PixFmt = PIX_FMT_VAAPI_VLD;
	static const VAProfile NoProfile = (VAProfile)(-1);
#endif
private:
	HwAccelInfo();
	static HwAccelInfo *obj;
	bool supports(AVCodecContext *avctx) const {return (m_avctx = (supports(avctx->codec_id) ? avctx : nullptr));}
#ifdef Q_WS_X11
	VAProfile findMatchedProfile(const QVector<VAProfile> &needs) const;
	VADisplay m_display = 0;
	QVector<VAProfile> m_profiles;
#endif
	mutable AVCodecContext *m_avctx = nullptr;
	bool m_ok = false;
	friend int is_hwaccel_available(AVCodecContext *avctx);
	friend int main(int, char**);
};

class HwAccel {
public:
	struct Context {
#ifdef Q_WS_MAC
		vda_context ctx;
#endif
#ifdef Q_WS_X11
		vaapi_context ctx;
#endif
		HwAccel *hwaccel;
	};
	HwAccel(AVCodecContext *avctx);
	~HwAccel();
	void *context() {return &m_ctx;}
	bool setBuffer(mp_image_t *mpi);
	void releaseBuffer(void *data);
	bool isUsable() const {return m_usable;}
	bool isCompatibleWith(const AVCodecContext *avctx) const;
	bool createSurface(GLuint *texture);
	bool copySurface(mp_image_t *mpi);
	VideoFormat format() const;
private:
	Context m_ctx;
	GLuint *m_textures = nullptr;
	AVCodecContext *m_avctx = nullptr;
	int m_width = 0, m_height = 0;
	bool m_usable = false;
#ifdef Q_WS_X11
	QVector<VASurfaceID> m_ids;
	QLinkedList<VASurfaceID> m_freeIds;
	QLinkedList<VASurfaceID> m_usingIds;
	void *m_glSurface = nullptr;
	VAProfile m_profile = HwAccelInfo::NoProfile;
#endif
};

#endif // HWACCEL_HPP
