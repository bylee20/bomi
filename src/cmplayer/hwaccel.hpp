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

class VideoFormat;		class VideoFrame;

class HwAccel {
public:
	static QList<AVCodecID> fullCodecList();
	static bool supports(AVCodecID codec);
#ifdef Q_OS_LINUX
	static void finalize();
	struct Context { vaapi_context ctx; HwAccel *hwaccel = nullptr; };
	HwAccel();
	~HwAccel() { free(); }
	bool isUsable() const {return m_usable;}
	bool setBuffer(mp_image_t *mpi);
	void releaseBuffer(void *data);
	mp_image &extract(mp_image *mpi);
	void clean();
	void afterCopy();
	bool copyTo(mp_image_t *mpi, VideoFrame &frame);
	bool set(AVCodecContext *avctx);
private:
	struct ProfileInfo { QVector<VAProfile> profiles; int surfaceCount = 0; };
	struct Info {
		static Info &get(); Info(); bool ok = false;
		QVector<VAProfile> profiles; QMap<AVCodecID, ProfileInfo> supported;
		Display *xdpy = nullptr; VADisplay display = nullptr;
	};
	static const ProfileInfo *find(AVCodecID codec);
	void free();
	Context m_ctx;
	int m_width = 0, m_height = 0;
	bool m_usable = false, m_init = false;
	QVector<VASurfaceID> m_ids;
	QLinkedList<VASurfaceID> m_freeIds, m_usingIds;
	VAProfile m_profile = (VAProfile)(-1);
	VAImage m_vaImage;
	mp_image m_mpi;
#endif
};

#endif // HWACCEL_HPP
