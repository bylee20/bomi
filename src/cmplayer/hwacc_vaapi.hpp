#ifndef HWACC_VAAPI_HPP
#define HWACC_VAAPI_HPP

#include "hwacc.hpp"
#include "log.hpp"

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

struct VaApiStatusChecker {
	DECLARE_LOG_CONTEXT(VA-API)
	virtual ~VaApiStatusChecker() {}
	bool isSuccess(VAStatus status) { m_status = status; return isSuccess(); }
	bool isSuccess() const { return m_status == VA_STATUS_SUCCESS; }
	VAStatus status() const { return m_status; }
	const char *error() const { return vaErrorStr(m_status); }
	bool check(VAStatus status, const QString &onError = QString()) {
		if (isSuccess(status))
			return true;
		_Error("Error %%(0x%%): %%", error(), QString::number(m_status, 16), onError);
		return false;
	}
	bool check(VAStatus status, const char *onError = "") { return check(status, _L(onError)); }
private:
	VAStatus m_status = VA_STATUS_SUCCESS;
};

struct vaapi_context;

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

typedef HwAccCodec<VAProfile> VaApiCodec;

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
	void finalize();
	QVector<VAProfile> m_profiles;
	QMap<AVCodecID, VaApiCodec> m_supported;
	QMap<VAProfile, QVector<VAEntrypoint>> m_entries;
	int m_surfaceFormat = 0;
	static VADisplay m_display;
	static bool init;
	friend void initialize_vaapi();
	friend void finalize_vaapi();
	friend class HwAccVaApi;
};

class VaApiSurface {
public:
	~VaApiSurface();
	VASurfaceID id() const { return m_id; }
	int format() const { return m_format; }
private:
	VaApiSurface() = default;
	friend class VaApiSurfacePool;
	VASurfaceID m_id = VA_INVALID_SURFACE;
	bool m_ref = false, m_orphan = false;
	quint64 m_order = 0;
	int m_format = 0;
};

class VaApiSurfacePool : public VaApiStatusChecker {
public:
	VaApiSurfacePool() {  }
	~VaApiSurfacePool() { clear(); }
	VAStatus create(int size, int width, int height, uint format);
	mp_image *getMpImage();
	void clear();
	QVector<VASurfaceID> ids() const {return m_ids;}
	uint format() const {return m_format;}
	static VaApiSurface *getSurface(mp_image *mpi);
private:
	VaApiSurface *getSurface();
	QVector<VASurfaceID> m_ids;
	QVector<VaApiSurface*> m_surfaces;
	uint m_format = 0;
	int m_width = 0, m_height = 0;
	quint64 m_order = 0LL;
};

#endif

void initialize_vaapi();
void finalize_vaapi();

#endif // HWACC_VAAPI_HPP
