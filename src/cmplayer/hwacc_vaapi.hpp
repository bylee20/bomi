#ifndef HWACC_VAAPI_HPP
#define HWACC_VAAPI_HPP

#include "hwacc.hpp"
#include "hwacc_helper.hpp"

#ifdef Q_OS_LINUX

#include <va/va.h>
#if VA_CHECK_VERSION(0, 34, 0)
//#define USE_VAVPP
#include <va/va_compat.h>
#else
static constexpr VAProfile VAProfileNone = (VAProfile)-1;
#endif

#ifdef USE_VAVPP
#include <va/va_vpp.h>
#endif

template<> struct HwAccX11Trait<IMGFMT_VAAPI> {
	using Profile = VAProfile;
	using Status = VAStatus;
	static constexpr Status success = VA_STATUS_SUCCESS;
	static constexpr const char *name = "VA-API";
	static const char *error(Status status) { return vaErrorStr(status); }
	using SurfaceID = VASurfaceID;
	static constexpr SurfaceID invalid = VA_INVALID_SURFACE;
	static void destroySurface(SurfaceID id);
	static bool createSurfaces(int width, int height, int format, QVector<SurfaceID> &ids);
};

using VaApiStatusChecker = HwAccX11StatusChecker<IMGFMT_VAAPI>;
using VaApiCodec = HwAccX11Codec<IMGFMT_VAAPI> ;
using VaApiSurface = HwAccX11Surface<IMGFMT_VAAPI>;
using VaApiSurfacePool = HwAccX11SurfacePool<IMGFMT_VAAPI>;

class HwAccVaApi : public HwAcc, public VaApiStatusChecker {
public:
	HwAccVaApi(AVCodecID codec);
	virtual ~HwAccVaApi();
	virtual bool isOk() const override;
	virtual void *context() const override;
	virtual mp_image *getSurface() override;
	virtual Type type() const override {return VaApiGLX;}
	virtual mp_image *getImage(mp_image *mpi);
private:
	void freeContext();
	bool fillContext(AVCodecContext *avctx) override;
private:
	struct Data;
	Data *d;
};

#ifdef USE_VAVPP
struct VaApiFilterCap {
	int algorithm = 0;
	VAProcFilterValueRange range = {0, 0, 0, 0};
};

struct VaApiFilterInfo : public VaApiStatusChecker {
	VaApiFilterInfo() {}
	VaApiFilterInfo(VAContextID context, VAProcFilterType type);
	VAProcFilterType type() const {return m_type;}
	const VaApiFilterCap *cap(int algorithm) const {
		for (const auto &cap : m_caps) {
			if (cap.algorithm == algorithm)
				return &cap;
		}
		return nullptr;
	}
	const QVector<int> &algorithms() const { return m_algorithms; }
	static QString description(VAProcFilterType type, int algorithm);
	bool supports(int algorithm) const { return m_algorithms.contains(algorithm); }
private:
	QVector<int> m_algorithms;
	VAProcFilterType m_type = VAProcFilterNone;
	QVector<VaApiFilterCap> m_caps;
};
#endif

struct VaApi : public VaApiStatusChecker {
	static const VaApiCodec *codec(AVCodecID id) { return find(id, get().m_supported); }
	static VADisplay glx() {return m_display;}
#ifdef USE_VAVPP
	static VAProcDeinterlacingType toVAType(DeintMethod method);
	static const VaApiFilterInfo *filter(VAProcFilterType type) { return find(type, get().m_filters); }
	static QList<VaApiFilterInfo> filters() { return get().m_filters.values(); }
	static QList<int> algorithms(VAProcFilterType type);
#endif
	static int surfaceFormat() {return get().m_surfaceFormat;}
	static int toVAType(int mp_fields, bool first);
	static void finalize();
	static void initialize();
	static bool isAvailable() { return ok; }
private:
	void setSurfaceFormat(int format) { m_surfaceFormat = format; }
	bool hasEntryPoint(VAEntrypoint point, VAProfile profile = VAProfileNone) {
		auto entries = find(profile, m_entries); return entries && entries->contains(point);
	}
	template<typename Map>
	static const typename Map::mapped_type *find(typename Map::key_type key, const Map &map) {
		const auto it = map.find(key); return (it != map.end()) ? &(*it) : nullptr;
	}
	void initCodecs();
#ifdef USE_VAVPP
	void initFilters();
	QMap<VAProcFilterType, VaApiFilterInfo> m_filters;
#endif
	static VaApi &get();
	VaApi();
	QVector<VAProfile> m_profiles;
	QMap<AVCodecID, VaApiCodec> m_supported;
	QMap<VAProfile, QVector<VAEntrypoint>> m_entries;
	int m_surfaceFormat = 0;
	static VADisplay m_display;
	static bool init, ok;
	friend class HwAccVaApi;
};

class VaApiMixer : public HwAccMixer, public VaApiStatusChecker{
public:
	~VaApiMixer();
	bool upload(const VideoFrame &frame, bool deint) override;
	bool directRendering() const override { return true; }
private:
	VaApiMixer(const QList<OpenGLTexture2D> &textures, const VideoFormat &format);
	static void adjust(VideoFormatData *data, const mp_image *mpi);
	void *m_glSurface = nullptr;
	friend class HwAcc;
};

#endif

#endif // HWACC_VAAPI_HPP
