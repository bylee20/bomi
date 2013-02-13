#include "hwacc.hpp"

QList<AVCodecID> HwAcc::fullCodecList() {
	static const QList<AVCodecID> list = QList<AVCodecID>()
		<< AV_CODEC_ID_MPEG1VIDEO << AV_CODEC_ID_MPEG2VIDEO << AV_CODEC_ID_MPEG4
		<< AV_CODEC_ID_WMV3 << AV_CODEC_ID_VC1 << AV_CODEC_ID_H264;
	 return list;
}

#ifdef Q_OS_MAC
HwAcc::HwAcc() {}
HwAcc::~HwAcc() {}
bool HwAcc::supports(AVCodecID codec) { return codec == AV_CODEC_ID_H264; }
const char *HwAcc::codecName(AVCodecID id) {
	switch (id) {
	case AV_CODEC_ID_H264:
		return "ffh264vda";
	default:
		return nullptr;
	}
}
#endif

#ifdef Q_OS_LINUX
#include "mpv-vaapi.hpp"
HwAcc::HwAcc() { VaApiInfo::initialize(); }
HwAcc::~HwAcc() { VaApiInfo::finalize(); }
bool HwAcc::supports(AVCodecID codec) { return VaApiInfo::find(codec) != nullptr; }
const char *HwAcc::codecName(AVCodecID id) {
	switch (id) {
	case AV_CODEC_ID_H264:
		return "vaapi:h264";
	case AV_CODEC_ID_MPEG1VIDEO:
	case AV_CODEC_ID_MPEG2VIDEO:
		return "vaapi:mpegvideo";
	case AV_CODEC_ID_MPEG4:
		return "vaapi:mpeg4";
	case AV_CODEC_ID_WMV3:
		return "vaapi:wmv3";
	case AV_CODEC_ID_VC1:
		return "vaapi:vc1";
	default:
		return nullptr;
	}
}
#endif
