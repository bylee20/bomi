#ifndef HWACCEL_HPP
#define HWACCEL_HPP

#include <QtCore/QLinkedList>
#include <QtCore/QList>

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
int is_hwaccel_available(AVCodecContext *avctx);
#ifdef None
#undef None
#endif
}

class HwAccelInfo {
public:
	HwAccelInfo(const HwAccelInfo&) = delete;
	HwAccelInfo &operator = (const HwAccelInfo&) = delete;
	static HwAccelInfo &get() {return *obj;}
	~HwAccelInfo();
	bool isAvailable() const {return m_ok;}
	AVCodecContext *avctx() const {return m_avctx;}
	QList<CodecID> fullCodecList() const;
	bool supports(CodecID codec) const;
#ifdef Q_WS_X11
	VADisplay display() const {return m_display;}
	VAProfile find(CodecID codec, int &surfaceCount) const;
	static const PixelFormat PixFmt = PIX_FMT_VAAPI_VLD;
	static const VAProfile NoProfile = (VAProfile)(-1);
#endif
private:
	HwAccelInfo();
	static HwAccelInfo *obj;
#ifdef Q_WS_X11
	VAProfile find(AVCodecContext *avctx, int &surfaceCount) const;
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
	HwAccel(AVCodecContext *avctx);
	~HwAccel();
	void *context() {return &m_ctx;}
	bool setBuffer(mp_image_t *mpi);
	bool isUsable() const {return m_usable;}
	bool isCompatibleWith(const AVCodecContext *avctx) const;
	bool createSurface(GLuint *texture);
	bool copySurface(mp_image_t *mpi);
	VideoFormat format() const;
private:
#ifdef Q_WS_MAC
	vda_context m_ctx;
#endif
#ifdef Q_WS_X11
	vaapi_context m_ctx;
	QVector<VASurfaceID> m_ids;
	QLinkedList<VASurfaceID> m_freeIds;
	QLinkedList<VASurfaceID> m_usingIds;
	void *m_glSurface = nullptr;
	VAProfile m_profile = HwAccelInfo::NoProfile;
#endif
	GLuint *m_textures;
	AVCodecContext *m_avctx = nullptr;
	int m_width = 0, m_height = 0;
	bool m_usable = false;
};

#endif // HWACCEL_HPP
