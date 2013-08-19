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
: format(format) {
	data[0] = mpi->planes[0];
	data[1] = mpi->planes[1];
	data[2] = mpi->planes[2];
	data[3] = mpi->planes[3];
	this->mpi = mp_image_new_ref(mpi);
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
	data[0] = other.data[0];
	data[1] = other.data[1];
	data[2] = other.data[2];
	data[3] = other.data[3];
	if (other.mpi)
		mpi = mp_image_new_ref(other.mpi);
}

VideoFrame::Data::~Data() {
	if (mpi)
		mp_image_unrefp(&mpi);
}

QImage VideoFrame::toImage() const {
	if (!d->image.isNull())
		return d->image;
	if (!d->mpi || d->format.isEmpty())
		return QImage();
	SwsContext *sws = sws_getCachedContext(nullptr
		, d->format.width(), d->format.height(), imgfmt2pixfmt(d->format.type())
		, d->format.width(), d->format.height(), AV_PIX_FMT_RGB24, SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);
	QImage image(d->format.size(), QImage::Format_RGB888);
	const uchar *srcData[] = {data(0), data(1), data(2)};
	const int srcStride[] = {d->format.bytesPerLine(0), d->format.bytesPerLine(1), d->format.bytesPerLine(2)};
	uchar *destData[] = {image.bits()}; const int destStride[] = {image.bytesPerLine()};
	sws_scale(sws, srcData, srcStride, 0, d->format.height(), destData, destStride);
	sws_freeContext(sws);
	return image;
}
