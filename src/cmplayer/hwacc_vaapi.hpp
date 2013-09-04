#ifndef HWACC_VAAPI_HPP
#define HWACC_VAAPI_HPP

#include "hwacc.hpp"

#ifdef Q_OS_LINUX

#include <va/va.h>
#include <va/va_vpp.h>
#if VA_CHECK_VERSION(0, 34, 0)
#include <va/va_compat.h>
#endif

struct VaApiStatusChecker {
	virtual ~VaApiStatusChecker() {}
	bool isSuccess(bool status) { return (m_status = status) == VA_STATUS_SUCCESS; }
	bool isSuccess() const { return m_status == VA_STATUS_SUCCESS; }
	VAStatus status() const { return m_status; }
	const char *text() const { return vaErrorStr(m_status); }
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

struct VaApiFilterCapability {
	int algorithm = 0;
	VAProcFilterValueRange range = {0, 0, 0, 0};
};

struct VaApiFilterInfo : public VaApiStatusChecker {
	VaApiFilterInfo() {}
	VaApiFilterInfo(VAContextID context, VAProcFilterType type);
	VAProcFilterType type() const {return m_type;}
	const VaApiFilterCapability *capability(int algorithm) {
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
	QVector<VaApiFilterCapability> m_caps;
};

typedef HwAccCodec<VAProfile> VaApiCodec;

struct VaApi : public VaApiStatusChecker {
	static const VaApiCodec *codec(AVCodecID id) { return find(id, get().m_supported); }
	static VADisplay glx() {return m_display;}
	static const VaApiFilterInfo *filter(VAProcFilterType type) { return find(type, get().m_filters); }
	static QList<VaApiFilterInfo> filters() { return get().m_filters.values(); }
	static QList<int> algorithms(VAProcFilterType type);
	static int surfaceFormat() {return get().m_surfaceFormat;}
private:
	void setSurfaceFormat(int format) { m_surfaceFormat = format; }
	bool hasEntryPoint(VAEntrypoint point, VAProfile profile = VAProfileNone) {
		auto entries = find(profile, m_entries); return entries && entries->contains(point);
	}
	template<typename Map>
	static const typename Map::mapped_type *find(typename Map::key_type key, const Map &map) {
		const auto it = map.find(key); return (it != map.end()) ? &(*it) : nullptr;
	}
	static VaApi &get();
	VaApi();
	void finalize();
	QMap<AVCodecID, VaApiCodec> m_supported;
	QMap<VAProfile, QVector<VAEntrypoint>> m_entries;
	QMap<VAProcFilterType, VaApiFilterInfo> m_filters;
	int m_surfaceFormat = 0;
	static VADisplay m_display;
	static bool init;
	friend void initialize_vaapi();
	friend void finalize_vaapi();
	friend class HwAccVaApi;
};

#endif

#endif // HWACC_VAAPI_HPP
