#include "videoframe.hpp"
#include <stdlib.h>
extern "C" {
#include <libswscale/swscale.h>
#include <video/fmt-conversion.h>
#include <video/img_format.h>
#include <video/mp_image.h>
#include <video/img_fourcc.h>
}

VideoFrame::VideoFrame::Data::Data(mp_image *mpi, const VideoFormat &format)
: mpi(mp_image_new_ref(mpi)), format(format) {
}

VideoFrame::VideoFrame::Data::Data(const QImage &image)
: mpi(nullptr), format(image) {
	if (image.format() != QImage::Format_ARGB32_Premultiplied && image.format() != QImage::Format_ARGB32)
		this->image = image.convertToFormat(QImage::Format_ARGB32);
	else
		this->image = image;
}

VideoFrame::VideoFrame::Data::Data(const Data &other)
: QSharedData(other) {
	image = other.image;
	format = other.format;
	if (other.mpi)
		mpi = mp_image_new_ref(other.mpi);
}

VideoFrame::Data::~Data() {
	if (mpi)
		mp_image_unrefp(&mpi);
}

const uchar *VideoFrame::data(int i) const {
	return d->mpi->planes[i];
}

QImage VideoFrame::toImage() const {
	if (!d->image.isNull())
		return d->image;
	if (!d->mpi || d->format.isEmpty())
		return QImage();
	SwsContext *sws = sws_getCachedContext(nullptr
		, d->format.width(), d->format.height(), d->format.pixfmt()
		, d->format.width(), d->format.height(), AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
	QImage image(d->format.size(), QImage::Format_RGB888);
	const uchar *srcData[] = {data(0), data(1), data(2)};
	const int srcStride[] = {d->format.byteWidth(0), d->format.byteWidth(1), d->format.byteWidth(2)};
	uchar *destData[] = {image.bits()}; const int destStride[] = {image.bytesPerLine()};
	sws_scale(sws, srcData, srcStride, 0, d->format.height(), destData, destStride);
	sws_freeContext(sws);
	return image;
}
