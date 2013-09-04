#ifndef VIDEOFILTER_H
#define VIDEOFILTER_H

#include <QLinkedList>
#include "videoframe.hpp"

extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
#include <libavfilter/avfiltergraph.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
#include <libavcodec/avcodec.h>
#include <video/fmt-conversion.h>
#include <libpostproc/postprocess.h>
#include <mpvcore/cpudetect.h>
#include <video/mp_image_pool.h>
}

class VideoFilter {
public:
	virtual ~VideoFilter() = default;
	bool apply(mp_image *mpi, QLinkedList<VideoFrame> &queue, double prevPts) {
		if (!m_format.compare(mpi))
			reconfigure(VideoFormat(mpi));
		if (m_built && pass(mpi, queue, prevPts))
			return true;
		queue.push_back(VideoFrame(mpi));
		return false;
	}
	const VideoFormat &format() const { return m_format; }
	void setFrameFlags(int flags) { m_flags = flags; }
	int frameFlags() const { return m_flags; }
	void setOptions(const QString &options) {
		if (_Change(m_options, options))
			m_built = build();
	}
	QString options() const { return m_options; }
protected:
	virtual bool pass(mp_image *mpi, QLinkedList<VideoFrame> &queue, double prevPts) = 0;
	virtual bool build() = 0;
private:
	bool reconfigure(const VideoFormat &format) {
		m_format = format;
		return m_built = build();
	}
	VideoFormat m_format;
	int m_flags = 0;
	QString m_options;
	bool m_built = false;
};

class PassThroughVideoFilter : public VideoFilter {
protected:
	virtual bool pass(mp_image *mpi, QLinkedList<VideoFrame> &queue, double prevPts) override {
		return VideoFilter::pass(mpi, queue, prevPts);
	}
	virtual bool build() override { return VideoFilter::build(); }
};

class FFmpegAvFilter : public VideoFilter {
public:
	~FFmpegAvFilter() { release(); }
protected:
	bool pass(mp_image *mpi, QLinkedList<VideoFrame> &queue, double prevPts) override;
	bool build();
private:
	void release() { avfilter_graph_free(&m_graph); m_src = m_sink = nullptr; }
	AVFilterGraph *m_graph = nullptr;
	AVFilterContext *m_src = nullptr, *m_sink = nullptr;
};

class FFmpegPostProcDeint : public VideoFilter {
public:
	FFmpegPostProcDeint() {
		m_pool = mp_image_pool_new(10);
	}
	~FFmpegPostProcDeint() { release(); mp_image_pool_clear(m_pool); }
protected:
	bool pass(mp_image *mpi, QLinkedList<VideoFrame> &queue, double prevPts) override;
	bool build() override;
private:
	void paintOut(mp_image *dst, const mp_image *src) const;
	VideoFrame topField(mp_image *mpi) const;
	VideoFrame bottomField(mp_image *mpi) const;
	mp_image *newImage(const mp_image *mpi) const;
	void process(mp_image *dest, const mp_image *src) const;
	void release() {
		if (m_context)
			pp_free_context(m_context);
		if (m_mode)
			pp_free_mode(m_mode);
	}
	pp_context *m_context = nullptr;
	pp_mode *m_mode = nullptr;
	mp_image_pool *m_pool = nullptr;
	bool m_doubler = false;
};

// this marks the flags only
class HardwareDeintFilter : public VideoFilter {
public:

protected:
	bool pass(mp_image *mpi, QLinkedList<VideoFrame> &queue, double prevPts) override;
	bool build() override;
private:
	bool m_doubler = false;
};

#endif // VIDEOFILTER_H
