#ifndef VIDEOFILTER_H
#define VIDEOFILTER_H

#include <QLinkedList>
#include "videoframe.hpp"
#include "deintinfo.hpp"

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
	bool apply(VideoFrame &in, QLinkedList<VideoFrame> &queue, double prevPts) {
		if (m_format != in.format())
			reconfigure(in.format());
		if (m_built && pass(in, queue, prevPts))
			return true;
		queue.push_back(in);
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
	virtual bool pass(VideoFrame &in, QLinkedList<VideoFrame> &queue, double prevPts) = 0;
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
	virtual bool pass(VideoFrame &in, QLinkedList<VideoFrame> &queue, double prevPts) override {
		return VideoFilter::pass(in, queue, prevPts);
	}
	virtual bool build() override { return VideoFilter::build(); }
};

class FFmpegFilterGraph {
public:
	virtual ~FFmpegFilterGraph() { release(); }
	bool push(mp_image *mpi);
	mp_image *pull();
	bool initialize(const QString &option, const QSize &size, mp_imgfmt imgfmt);
private:
	void release() { avfilter_graph_free(&m_graph); m_src = m_sink = nullptr; }
	bool linkGraph(AVFilterInOut *&in, AVFilterInOut *&out);
	QString m_option;
	mp_imgfmt m_imgfmt = IMGFMT_NONE;
	QSize m_size = {0, 0};
	AVFilterGraph *m_graph = nullptr;
	AVFilterContext *m_src = nullptr, *m_sink = nullptr;
};

class FFmpegPostProc {
public:
	FFmpegPostProc() { m_pool = mp_image_pool_new(10); }
	virtual ~FFmpegPostProc() { release(); mp_image_pool_clear(m_pool); }
	bool process(mp_image *dest, const mp_image *src) const;
	bool initialize(const QString &option, const QSize &size, mp_imgfmt imgfmt);
	mp_image *newImage(const mp_image *mpi) const;
private:
	void release() {
		if (m_context) pp_free_context(m_context);
		if (m_mode)    pp_free_mode(m_mode);
	}
	QString m_option;
	mp_imgfmt m_imgfmt = IMGFMT_NONE;
	QSize m_size = {0, 0};
	pp_context *m_context = nullptr;
	pp_mode *m_mode = nullptr;
	mp_image_pool *m_pool = nullptr;
};

class SoftwareDeinterlacer {
	enum Type {Graph, PP, Mark, Pass};
public:
	void setOption(DeintOption deint);
	void process(const VideoFrame &in, QLinkedList<VideoFrame> &queue, double prev);
private:
	void split(const VideoFrame &in, QLinkedList<VideoFrame> &outs, double prev);
	bool tryGraph(const VideoFrame &in, QList<mp_image*> &outs);
	bool tryPostProc(const VideoFrame &in, QList<mp_image*>& outs);
	QString m_option;
	bool m_rebuild = true;
	DeintOption m_deint;
	FFmpegFilterGraph m_graph;
	FFmpegPostProc m_pp;
	Type m_type = Pass;
};

#endif // VIDEOFILTER_H
