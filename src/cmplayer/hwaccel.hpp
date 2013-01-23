#ifndef HWACCEL_HPP
#define HWACCEL_HPP

#include "stdafx.hpp"

extern "C" {
#ifdef Q_OS_LINUX
#include <libavcodec/vaapi.h>
#include <va/va_x11.h>
#endif
#include <libavcodec/avcodec.h>
#include <video/mp_image.h>
#include <video/img_format.h>
#ifdef None
#undef None
#endif
}

class VideoFormat;

class HwAccelInfo {
public:
	HwAccelInfo();
	bool isAvailable() const {return m_ok;}
	QList<AVCodecID> fullCodecList() const;
	bool supports(AVCodecID codec) const;
#ifdef Q_OS_LINUX
	AVCodecContext *avctx() const {return m_avctx;}
    VADisplay display() const {return m_display;}
    VAProfile find(CodecID codec, int &surfaceCount) const;
    static const VAProfile NoProfile = (VAProfile)(-1);
    bool supports(AVCodecContext *avctx) const {return (m_avctx = (supports(avctx->codec_id) ? avctx : nullptr));}
#endif
	static void finalize();
private:
#ifdef Q_OS_LINUX
    static VADisplay m_display;
    static QVector<VAProfile> m_profiles;
    static AVCodecContext *m_avctx;
#endif
	static bool m_ok;
#ifdef Q_OS_MAC
	friend int is_hwaccel_available(const char *dll, AVCodecID codec);
#endif
};

#ifdef Q_OS_LINUX
class VideoFrame;

class HwAccel {
public:
	struct Context {
		vaapi_context ctx;
		HwAccel *hwaccel;
	};
	HwAccel(AVCodecContext *avctx);
	~HwAccel();
	void *context() {return &m_ctx;}
	bool isUsable() const {return m_usable;}
	bool isCompatibleWith(const AVCodecContext *avctx) const;
	bool setBuffer(mp_image_t *mpi);
	void releaseBuffer(void *data);
	bool copyTo(mp_image_t *mpi, VideoFrame &frame);
private:
	Context m_ctx;
	AVCodecContext *m_avctx = nullptr;
	int m_width = 0, m_height = 0;
	bool m_usable = false;
	GLuint *m_textures = nullptr;
	QVector<VASurfaceID> m_ids;
	QLinkedList<VASurfaceID> m_freeIds;
	QLinkedList<VASurfaceID> m_usingIds;
	VAProfile m_profile = HwAccelInfo::NoProfile;
	mp_image m_mpi;
	VAImage m_vaImage;
};

#endif

#endif // HWACCEL_HPP
