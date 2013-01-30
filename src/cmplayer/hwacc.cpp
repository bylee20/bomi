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
HwAccel::HwAccel() { VaApiInfo::initialize(); }
HwAccel::~HwAccel() { VaApiInfo::finalize(); }
bool HwAccel::supports(AVCodecID codec) { return VaApiInfo::find(codec) != nullptr; }
const char *HwAccel::codecName(AVCodecID id) {
	switch (id) {
	case AV_CODEC_ID_H264:
		return "vah264";
	case AV_CODEC_ID_MPEG1VIDEO:
	case AV_CODEC_ID_MPEG2VIDEO:
		return "vampeg12";
	case AV_CODEC_ID_MPEG4:
		return "vampeg4";
	case AV_CODEC_ID_WMV3:
		return "vawmv3";
	case AV_CODEC_ID_VC1:
		return "vavc1";
	default:
		return nullptr;
	}
}
#endif
