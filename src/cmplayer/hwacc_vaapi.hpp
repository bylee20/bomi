#ifndef HWACC_VAAPI_HPP
#define HWACC_VAAPI_HPP

#include "hwacc.hpp"

#ifdef Q_OS_LINUX

extern "C" {
#include <va/va.h>
}

struct vaapi_context;

class HwAccVaApi : public HwAcc {
public:
	HwAccVaApi(AVCodecID codec, Type type);
	virtual ~HwAccVaApi();
	virtual bool isOk() const override;
	virtual void *context() const override;
	virtual mp_image *getSurface() override;
	virtual Type type() const override {return m_type;}
protected:
	vaapi_context *vaapi() const;
	int &status() {return m_status;}
	int status() const {return m_status;}
	QSize &size() {return m_size;}
	const QSize &size() const {return m_size;}
	void freeContext();
	bool fillContext(AVCodecContext *avctx) override;
	bool isSuccess(int result);
	void *surface(int i) const;
private:
	struct Data;
	Data *d;
	int m_status;
	QSize m_size;
	Type m_type;
};

class HwAccVaApiGLX : public HwAccVaApi {
public:
	HwAccVaApiGLX(AVCodecID codec): HwAccVaApi(codec, VaApiGLX) {}
	virtual mp_image *getImage(mp_image *mpi) {return mpi;}
};

struct VaApiImage;

class HwAccVaApiX11 : public HwAccVaApi {
public:
	HwAccVaApiX11(AVCodecID codec);
	virtual ~HwAccVaApiX11();
	bool fillContext(AVCodecContext *avctx) override;
	virtual mp_image *getImage(mp_image *mpi) override;
private:
	VaApiImage *newImage();
	struct Data;
	Data *d;
};

typedef HwAccCodec<VAProfile> VaApiCodec;

struct VaApi {
	static const VaApiCodec *find(AVCodecID id) {
		const auto &supported = get().m_supported;
		auto it = supported.find(id);
		return it != supported.end() && !it->profiles.isEmpty() ? &(*it) : 0;
	}
	static VADisplay glx() {return get().m_glxDisplay;}
	static VADisplay x11() {return get().m_x11Display;}
	static VADisplay display(HwAcc::Type type) {
		if (type == HwAcc::VaApiGLX)
			return glx();
		if (type == HwAcc::VaApiX11)
			return x11();
		return nullptr;
	}
private:
	static VaApi &get();
	VaApi();
	void finalize();
	QMap<AVCodecID, VaApiCodec> m_supported;
	VADisplay m_glxDisplay = nullptr, m_x11Display = nullptr;
	static bool init;
	friend void initialize_vaapi();
	friend void finalize_vaapi();
};

#endif

#endif // HWACC_VAAPI_HPP
