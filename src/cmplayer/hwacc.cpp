#include "hwacc.hpp"
#include "videooutput.hpp"
#include "stdafx.hpp"
extern "C" {
#include <video/decode/dec_video.h>
#include <video/decode/lavc.h>
#include <video/mp_image.h>
#ifdef Q_OS_LINUX
#include <video/mp_image_pool.h>
#include <va/va.h>
#include <va/va_glx.h>
#if VA_CHECK_VERSION(0, 34, 0)
#include <va/va_compat.h>
#endif
#include <libavcodec/vaapi.h>
#endif
}

#ifdef Q_OS_LINUX
#include <QX11Info>

struct VaApiInfo {
	struct Codec { AVCodecID id = AV_CODEC_ID_NONE; QVector<VAProfile> profiles; int surfaceCount = 0; };
	static VaApiInfo &get();
	const Codec *find(AVCodecID id) const { auto it = m_supported.find(id); return it != m_supported.end() && !it->profiles.isEmpty() ? &(*it) : 0; }
	void *display() const { return m_display; }
	void finalize() {if (m_display) {vaTerminate(m_display); m_display = nullptr; init = false;}}
private:
	VaApiInfo() {
		m_xdpy = QX11Info::display();
		if (!m_xdpy || !(m_display = vaGetDisplayGLX(m_xdpy)))
			return;
		int major, minor;
		if (vaInitialize(m_display, &major, &minor) != VA_STATUS_SUCCESS)
			return;
		init = true;
		auto size = vaMaxNumProfiles(m_display);
		QVector<VAProfile> profiles;
		profiles.resize(size);
		if (vaQueryConfigProfiles(m_display, profiles.data(), &size) != VA_STATUS_SUCCESS)
			return;
		profiles.resize(size);
		auto supports = [this, &profiles](const QVector<VAProfile> &candidates, int count, AVCodecID id) {
			Codec codec;
			for (auto one : candidates) {
				if (profiles.contains(one))
					codec.profiles.push_back(one);
			}
			if (!codec.profiles.isEmpty()) {
				codec.surfaceCount = count;
				codec.id = id;
			}
			return codec;
		};
	#define NUM_VIDEO_SURFACES_MPEG2  3 /* 1 decode frame, up to  2 references */
	#define NUM_VIDEO_SURFACES_MPEG4  3 /* 1 decode frame, up to  2 references */
	#define NUM_VIDEO_SURFACES_H264  21 /* 1 decode frame, up to 20 references */
	#define NUM_VIDEO_SURFACES_VC1    3 /* 1 decode frame, up to  2 references */
		const QVector<VAProfile> mpeg2s = {VAProfileMPEG2Main, VAProfileMPEG2Simple};
		const QVector<VAProfile> mpeg4s = {VAProfileMPEG4Main, VAProfileMPEG4AdvancedSimple, VAProfileMPEG4Simple};
		const QVector<VAProfile> h264s = {VAProfileH264High, VAProfileH264Main, VAProfileH264Baseline};
		const QVector<VAProfile> wmv3s = {VAProfileVC1Main, VAProfileVC1Simple};
		const QVector<VAProfile> vc1s = {VAProfileVC1Advanced};
		m_supported[AV_CODEC_ID_MPEG1VIDEO] = supports(mpeg2s, NUM_VIDEO_SURFACES_MPEG2, AV_CODEC_ID_MPEG1VIDEO);
		m_supported[AV_CODEC_ID_MPEG2VIDEO] = supports(mpeg2s, NUM_VIDEO_SURFACES_MPEG2, AV_CODEC_ID_MPEG2VIDEO);
		m_supported[AV_CODEC_ID_MPEG4] = supports(mpeg4s, NUM_VIDEO_SURFACES_MPEG4, AV_CODEC_ID_MPEG4);
		m_supported[AV_CODEC_ID_WMV3] = supports(wmv3s, NUM_VIDEO_SURFACES_VC1, AV_CODEC_ID_WMV3);
		m_supported[AV_CODEC_ID_VC1] = supports(vc1s, NUM_VIDEO_SURFACES_VC1, AV_CODEC_ID_VC1);
		m_supported[AV_CODEC_ID_H264] = supports(h264s, NUM_VIDEO_SURFACES_H264, AV_CODEC_ID_H264);
	}
	QMap<AVCodecID, Codec> m_supported;
	Display *m_xdpy = nullptr;
	VADisplay m_display = nullptr;
	static bool init;
	friend void initialize_vaapi();
	friend void finalize_vaapi();

};

bool VaApiInfo::init = false;
void initialize_vaapi() {if (!VaApiInfo::init) {VaApiInfo::get().display(); VaApiInfo::init = true;}}
void finalize_vaapi() {if (VaApiInfo::init) VaApiInfo::get().finalize();}

VaApiInfo &VaApiInfo::get() {static VaApiInfo info; return info;}

struct VaApi {
	struct Texture {
		Texture(VADisplay display, int width, int height): display(display) {
			glGenTextures(1, &id);
			glBindTexture(GL_TEXTURE_2D, id);
			glTexImage2D(GL_TEXTURE_2D, 0, 4, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
			status = vaCreateSurfaceGLX(display, GL_TEXTURE_2D, id, &surface);
			if (status != VA_STATUS_SUCCESS)
				qDebug() << "vaCreateSurfaceGLX():" << vaErrorStr(status);
		}
		~Texture() {
			if (surface)
				vaDestroySurfaceGLX(display, &surface);
			glDeleteTextures(1, &id);
		}
		VADisplay display = 0;
		GLuint id = GL_NONE;
		void *surface = nullptr;
		int status = VA_STATUS_ERROR_UNKNOWN;
	};
	struct VaApiSurface { VASurfaceID  id = VA_INVALID_ID; bool ref = false; quint64 order = 0; };
	mp_image nullImage;
	VaApi(AVCodecID cid) {
		memset(&nullImage, 0, sizeof(nullImage));
		memset(&m_context, 0, sizeof(m_context));
		m_context.context_id = m_context.config_id = VA_INVALID_ID;
		m_context.display = VaApiInfo::get().display();
		auto thread = QThread::currentThread();
		m_window.moveToThread(thread);
		m_gl.moveToThread(thread);
		m_window.setSurfaceType(QWindow::OpenGLSurface);
		m_window.setGeometry(-10, -10, 1, 1);
		m_window.create();
		m_gl.create();
		m_pool = mp_image_pool_new(5);
		auto codec = VaApiInfo::get().find(cid);
		if (codec) {
			m_profile = codec->profiles.front();
			VAConfigAttrib attr = { VAConfigAttribRTFormat, 0 };
			if((m_status = vaGetConfigAttributes(m_context.display, m_profile, VAEntrypointVLD, &attr, 1)) != VA_STATUS_SUCCESS)
				return;
			if(!(attr.value & VA_RT_FORMAT_YUV420) && !(attr.value & VA_RT_FORMAT_YUV422))
				{ m_status = VA_STATUS_ERROR_FLAG_NOT_SUPPORTED; return; }
			if((m_status = vaCreateConfig(m_context.display, m_profile, VAEntrypointVLD, &attr, 1, &m_context.config_id)) != VA_STATUS_SUCCESS)
				return;
			m_surfaces.resize(codec->surfaceCount);
		}
	}
	VAStatus m_status = VA_STATUS_ERROR_UNKNOWN;
	VAStatus status() const { return m_status; }
	~VaApi() {
		freeContext();
		if (m_context.config_id != VA_INVALID_ID)
			qDebug() << vaDestroyConfig(m_context.display, m_context.config_id);
		mp_image_pool_clear(m_pool);
		talloc_free(m_pool);
		m_gl.doneCurrent();
		m_window.destroy();
	}
	static void releaseSurface(void *ref) { *(bool*)ref = false; }
	mp_image *getImage() {
		int i_old, i;
		for (i=0, i_old=0; i<m_surfaces.size(); ++i) {
			if (!m_surfaces[i].ref)
				break;
			if (m_surfaces[i].order < m_surfaces[i_old].order)
				i_old = i;
		}
		if (i >= m_surfaces.size())
			i = i_old;
		m_surfaces[i].ref = true;
		m_surfaces[i].order = ++m_surfaceOrder;
		Q_ASSERT(m_surfaces[i].id != VA_INVALID_ID);
		auto mpi = mp_image_new_custom_ref(&nullImage, &m_surfaces[i].ref, releaseSurface);
		mpi->planes[1] = mpi->planes[2] = nullptr;
		mpi->planes[0] = mpi->planes[3] = (uchar*)(quintptr)m_surfaces[i].id;
		return mpi;
	}
	void freeContext() {
		if (m_texture) {
			m_gl.makeCurrent(&m_window);
			delete m_texture;
		}
		if (m_context.display) {
			if (m_context.context_id != VA_INVALID_ID)
				qDebug() << vaDestroyContext(m_context.display, m_context.context_id);
			for (auto &surface : m_surfaces) {
				if (surface.id != VA_INVALID_SURFACE)
					vaDestroySurfaces(m_context.display, &surface.id, 1);
				surface = VaApiSurface();
			}
		}
		m_context.context_id = VA_INVALID_ID;
		m_surfaceOrder = 0;
		m_texture = nullptr;
	}
	bool fillContext(int width, int height) {
		if (m_status != VA_STATUS_SUCCESS)
			return false;
		freeContext();
		QVector<VASurfaceID> ids(m_surfaces.size(), VA_INVALID_SURFACE);
		if ((m_status = vaCreateSurfaces(m_context.display, width, height, VA_RT_FORMAT_YUV420, ids.size(), ids.data())) != VA_STATUS_SUCCESS
				&& (m_status = vaCreateSurfaces(m_context.display, width, height, VA_RT_FORMAT_YUV422, ids.size(), ids.data())) != VA_STATUS_SUCCESS)
			return false;
		for (int i=0; i<ids.size(); ++i)
			m_surfaces[i].id = ids[i];
		if ((m_status = vaCreateContext(m_context.display, m_context.config_id, width, height, VA_PROGRESSIVE, ids.data(), ids.size(), &m_context.context_id)) != VA_STATUS_SUCCESS)
			return false;
		auto img = mp_image_pool_get(m_pool, IMGFMT_BGRA, width, height);
		m_gl.makeCurrent(&m_window);
		m_texture = new Texture(m_context.display, img->stride[0]/4, img->h);
		talloc_free(img);
		if ((m_status = m_texture->status) != VA_STATUS_SUCCESS)
			return false;
		return true;
	}
public:
	vaapi_context m_context;
	QVector<VaApiSurface> m_surfaces;
	quint64 m_surfaceOrder = 0;
	VAProfile m_profile = (VAProfile)(-1);
	QWindow m_window;
	QOpenGLContext m_gl;
	Texture *m_texture = nullptr;
	mp_image_pool *m_pool = nullptr;
};
using Backend = VaApi;

bool HwAcc::supports(AVCodecID codec) { return VaApiInfo::get().find(codec) != nullptr; }
const char *HwAcc::codecName(AVCodecID id) {
	switch (id) {
	case AV_CODEC_ID_H264:
		return "h264";
	case AV_CODEC_ID_MPEG1VIDEO:
		return "mpeg1video";
	case AV_CODEC_ID_MPEG2VIDEO:
		return "mpeg2video";
	case AV_CODEC_ID_MPEG4:
		return "mpeg4";
	case AV_CODEC_ID_WMV3:
		return "wmv3";
	case AV_CODEC_ID_VC1:
		return "vc1";
	default:
		return nullptr;
	}
}
#endif

QList<AVCodecID> HwAcc::fullCodecList() {
	static const QList<AVCodecID> list = QList<AVCodecID>()
		<< AV_CODEC_ID_MPEG1VIDEO << AV_CODEC_ID_MPEG2VIDEO << AV_CODEC_ID_MPEG4
		<< AV_CODEC_ID_WMV3 << AV_CODEC_ID_VC1 << AV_CODEC_ID_H264;
	 return list;
}

struct HwAcc::Data { VaApi *vaapi = nullptr; int width = 0, height = 0; };

HwAcc::HwAcc(AVCodecID codec, int width, int height)
: d(new Data) {
	d->vaapi = new VaApi(codec);
	reinitialize(width, height);
}

HwAcc::~HwAcc() {
	delete d->vaapi;
	delete d;
}

bool HwAcc::reinitialize(int width, int height) {
	d->width = width;
	d->height = height;
	return d->vaapi->fillContext(width, height);
}

bool HwAcc::isOk() const { return d->vaapi->status() == VA_STATUS_SUCCESS; }

bool HwAcc::isOk(int width, int height) const { return d->width == width && d->height == height && isOk(); }

static VideoOutput *vo(lavc_ctx *ctx) {
	return static_cast<VideoOutput*>((void*)ctx->hwdec_info->vdpau_ctx);
}

int HwAcc::init(lavc_ctx *ctx) {
	if (!ctx->hwdec_info || !ctx->hwdec_info->vdpau_ctx)
		return -1;
	auto acc = new HwAcc(ctx->avctx->codec_id, 32, 32);
	if (!acc->isOk()) {
		qDebug() << "vaapi:" << vaErrorStr(acc->d->vaapi->status());
		delete acc;
		return -1;
	}
	vo(ctx)->setHwAcc(acc);
	ctx->hwdec_priv = acc;
	ctx->avctx->hwaccel_context = &acc->d->vaapi->m_context;
	return 0;
}

void HwAcc::uninit(lavc_ctx *ctx) {
	if (ctx->hwdec_info && ctx->hwdec_info->vdpau_ctx)
		vo(ctx)->setHwAcc(nullptr);
	delete static_cast<HwAcc*>(ctx->hwdec_priv);
}

mp_image *HwAcc::allocateImage(struct lavc_ctx *ctx, AVFrame *frame) {
	auto acc = static_cast<HwAcc*>(ctx->hwdec_priv);
	if (frame->format != AV_PIX_FMT_VAAPI_VLD)
		return nullptr;
	int w = ctx->avctx->width;
	int h = ctx->avctx->height;
	if (!acc->isOk(w, h) && !acc->reinitialize(w, h))
		return nullptr;
	return acc->d->vaapi->getImage();
}

mp_image *HwAcc::getFrame(mp_image *mpi) {
	if (mpi->fmt.id == IMGFMT_VAAPI) {
		const auto id = (VASurfaceID)(uintptr_t)mpi->planes[3];
		vaSyncSurface(d->vaapi->m_context.display, id);
		glBindTexture(GL_TEXTURE_2D, d->vaapi->m_texture->id);
		const auto status = vaCopySurfaceGLX(d->vaapi->m_context.display, d->vaapi->m_texture->surface, id, VA_SRC_BT601);
		if (status != VA_STATUS_SUCCESS)
			qDebug() << "vaCopySurfaceGLX():" << vaErrorStr(status);
		mpi = mp_image_pool_get(d->vaapi->m_pool, IMGFMT_BGRA, d->width, d->height);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_BGRA, GL_UNSIGNED_BYTE, mpi->planes[0]);
	}
	return mpi;
}

vd_lavc_hwdec_functions create_vaapi_functions() {
	vd_lavc_hwdec_functions funcs;
	funcs.allocate_image = HwAcc::allocateImage;
	funcs.init = HwAcc::init;
	funcs.uninit = HwAcc::uninit;
	funcs.fix_image = nullptr;
	funcs.image_formats = (const int[]) {IMGFMT_VAAPI, 0};
	return funcs;
}

vd_lavc_hwdec_functions mp_vd_lavc_vaapi = create_vaapi_functions();
