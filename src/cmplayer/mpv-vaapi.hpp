#ifndef MPV_VAAPI_HPP
#define MPV_VAAPI_HPP

struct VaApiCodec;

struct VaApiInfo {
	static const VaApiCodec *find(AVCodecID id);
	static void *display();
	static void initialize();
	static void finalize();
private:
	static VaApiInfo &get();
	VaApiInfo();
	~VaApiInfo();
	struct Data;
	Data *d;
};

#endif // MPV_VAAPI_HPP
