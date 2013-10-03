#include "vaapipostprocessor.hpp"
#include "deintinfo.hpp"

#ifdef USE_VAVPP

template<VAProcFilterType type>
struct FilterInfo {
	typedef VAProcFilterParameterBuffer ParamType;
	constexpr static const int ParamNum = 1;
};

template<>
struct FilterInfo<VAProcFilterDeinterlacing> {
	typedef VAProcFilterParameterBufferDeinterlacing ParamType;
	constexpr static const int ParamNum = 1;
};

template<>
struct FilterInfo<VAProcFilterColorBalance> {
	typedef VAProcFilterParameterBufferColorBalance ParamType;
	constexpr static const int ParamNum = 4;
};

class BufferMan {
public:
	template<typename T>
	BufferMan(VABufferID id, T *&buf): m_id(id) { m_mapped = map(id, buf); }
	~BufferMan() { if (m_mapped) unmap(m_id); }
	bool isMapped() const { return m_mapped; }
	VABufferID id() const { return m_id; }

	template<typename T>
	static VABufferID create(VAContextID context, VABufferType type, T *param, int num = 1) {
		VABufferID id = VA_INVALID_ID;
		if (vaCreateBuffer(VaApi::glx(), context, type, sizeof(T), num, param, &id) != VA_STATUS_SUCCESS)
			return VA_INVALID_ID;
		return id;
	}
	template<typename T>
	static VABufferID create(VAContextID context, VABufferType type, int num = 1) {
		return create(context, type, (T*)nullptr, num);
	}
	void unmap() { if (m_mapped) { unmap(m_id); m_mapped = false; } }
	template<typename T>
	static bool map(VABufferID id, T *&buf) {
		return id != VA_INVALID_ID && vaMapBuffer(VaApi::glx(), id, (void**)&buf) == VA_STATUS_SUCCESS;
	}
	static void unmap(VABufferID id) { vaUnmapBuffer(VaApi::glx(), id); }
private:
	VABufferID m_id = VA_INVALID_ID;
	bool m_mapped = false;
};

class VaApiFilter : public VaApiStatusChecker {
public:
	template<typename T>
	VaApiFilter(int order, VAContextID context, VAProcFilterType type, T *&param, int num = 1)
	: m_pbuf((void**)&param), m_order(order) {
		if (!VaApi::filter(type))
			return;
		m_id = BufferMan::create<T>(context, VAProcFilterParameterBufferType, num);
		if (isValid() && map()) {
			((VAProcFilterParameterBufferBase*)param)->type = type;
			unmap();
		}
	}
	~VaApiFilter() {
		delete m_mapper;
		if (m_id != VA_INVALID_ID)
			vaDestroyBuffer(VaApi::glx(), m_id);
	}
	bool isValid() const { return m_id != VA_INVALID_ID; }
	bool map() {
		if (!m_mapper)
			m_mapper = new BufferMan(m_id, *m_pbuf);
		return m_mapper->isMapped();
	}
	void unmap() { _Delete(m_mapper); }
	VABufferID id() const { return m_id; }
	bool isMapped() const { return m_mapper && m_mapper->isMapped(); }
	int order() const { return m_order; }
private:
	VABufferID m_id = VA_INVALID_ID;
	void **m_pbuf = nullptr;
	BufferMan *m_mapper = nullptr;
	int m_order = 0;
};

class VaApiDeint : public VaApiFilter {
public:
	static constexpr const VAProcFilterType type = VAProcFilterDeinterlacing;
	VaApiDeint(int order, VAContextID context)
	: VaApiFilter(order, context, type, m_param) { }
	~VaApiDeint() { }
	VAProcFilterParameterBufferDeinterlacing *param() { return m_param; }
	void setMethod(VAProcDeinterlacingType type) {
		if (m_type != type && map()) {
			m_type = m_param->algorithm = type;
			unmap();
		}
	}
	void setFlags(int flags) {
		if (m_flags != flags && map()) {
			m_flags = m_param->flags = flags;
			unmap();
		}
	}
	bool isAvailable() const { return m_type != VAProcDeinterlacingNone; }
	VAProcDeinterlacingType algorithm() const { return m_type; }
private:
	VAProcFilterParameterBufferDeinterlacing *m_param = nullptr;
	VAProcDeinterlacingType m_type = VAProcDeinterlacingNone;
	int m_flags = 0;
};

class VaApiPipeline : public VaApiStatusChecker {
public:
	VaApiPipeline(VAContextID context): m_context(context) { }
	void add(const VaApiFilter *filter) {
		if (!m_filters.contains(filter->order())) {
			m_filters[filter->order()] = filter->id();
			m_rebuild = true;
		}
	}
	void remove(const VaApiFilter *filter) {
		if (m_filters.remove(filter->order()) > 0)
			m_rebuild = true;
	}
	void set(const VaApiFilter *filter, bool on) {
		if (on) add(filter); else remove(filter);
	}
	mp_image *render(const VideoFrame &frame, int flags) {
		if (m_rebuild)
			update();
		auto in = VaApiSurfacePool::getSurface(frame.mpi());
		if (!in)
			return nullptr;
		m_pool.create(5, frame.width(), frame.height(), in->format());
		auto ret = m_pool.getMpImage();
		if (!ret)
			return nullptr;
		auto out = VaApiSurfacePool::getSurface(ret);
		if (!out)
			return nullptr;
		VAProcPipelineParameterBuffer *param = nullptr;
		VABufferID buffer = BufferMan::create(m_context, VAProcPipelineParameterBufferType, param, 1);
		if (buffer == VA_INVALID_ID)
			return nullptr;
		enum {Begun = 1, Rendered = 2};
		int state = 0;
		auto pass = [this, out, &ret, &buffer, &state, &frame] () -> mp_image* {
			if (state & Begun)
				vaEndPicture(VaApi::glx(), m_context);
			if (state & Rendered) {
				mp_image_copy_attributes(ret, frame.mpi());
			} else
				mp_image_unrefp(&ret);
			vaDestroyBuffer(VaApi::glx(), buffer);
			vaSyncSurface(VaApi::glx(), out->id());
			return ret;
		};
		if (!isSuccess(vaBeginPicture(VaApi::glx(), m_context, out->id())))
			return pass();
		state |= Begun;
		if (!BufferMan::map(buffer, param))
			return pass();
		memset(param, 0, sizeof(*param));
		param->surface = in->id();
		param->filter_flags = flags;
		param->filters = &m_buffers.first();
		param->num_filters = m_buffers.size();
		param->forward_references = m_forward_refs.data();
		param->backward_references = m_backward_refs.data();
		param->num_forward_references = m_caps.num_forward_references;
		param->num_backward_references = m_caps.num_backward_references;
		BufferMan::unmap(buffer);
		if (!isSuccess(vaRenderPicture(VaApi::glx(), m_context, &buffer, 1)))
			return pass();
		state |= Rendered;
		return pass();
	}
	void update() {
		m_rebuild = false;
		if (m_buffers.size() == m_filters.size()) {
			int i = 0;
			for (auto it = m_filters.begin(); it != m_filters.end(); ++it, ++i) {
				if (m_buffers[i] != *it)
					break;
			}
			if (i == m_filters.size())
				return;
		}
		m_available = false;
		m_buffers.resize(m_filters.size());
		if (m_buffers.isEmpty())
			return;
		auto it = m_filters.begin();
		for (int i=0; i<m_buffers.size(); ++i, ++it)
			m_buffers[i] = *it;
		m_caps.num_input_color_standards = VAProcColorStandardCount;
		m_caps.num_output_color_standards = VAProcColorStandardCount;
		if (!isSuccess(vaQueryVideoProcPipelineCaps(VaApi::glx(), m_context, m_buffers.data(), m_buffers.size(), &m_caps)))
			return;
		if ((int)m_caps.num_forward_references > m_forward_refs.size())
			m_forward_refs.resize(m_caps.num_forward_references);
		if ((int)m_caps.num_backward_references > m_backward_refs.size())
			m_backward_refs.resize(m_caps.num_backward_references);
		m_available = true;
	}

	bool m_rebuild = true;
	bool m_available = false;
	VAContextID m_context = VA_INVALID_ID;
	VAProcColorStandardType m_input_colors[VAProcColorStandardCount];
	VAProcColorStandardType m_output_colors[VAProcColorStandardCount];
	QVector<VASurfaceID> m_forward_refs, m_backward_refs;
	VAProcPipelineCaps m_caps;
	VaApiSurfacePool m_pool;
	QMap<int, VABufferID> m_filters;
	QVector<VABufferID> m_buffers;
};

struct VaApiPostProcessor::Data : public VaApiStatusChecker {
	DeintOption deint;
	QList<VFType> filters;
	VAConfigID config = VA_INVALID_ID;
	VAContextID context = VA_INVALID_ID;
	VADisplay dpy = VaApi::glx();
	bool rebuild = true;
	VaApiPipeline *pipeline = nullptr;
	VaApiDeint *deinterlacer = nullptr;
	VAProcDeinterlacingType deintType() const {
		if (deint.device != DeintDevice::GPU || !deinterlacer || deinterlacer->id() == VA_INVALID_ID)
			return VAProcDeinterlacingNone;
		return VaApi::toVAType(deint.method);
	}
	int setupDeint(int mp_fields, bool first) {
		const bool deint = this->deint.method != DeintMethod::None && (mp_fields & MP_IMGFIELD_INTERLACED);
		if (deinterlacer && deinterlacer->isAvailable()) {
			pipeline->set(deinterlacer, deint);
			if (!deint)
				return 0;
			int flags = 0;
			if (mp_fields & MP_IMGFIELD_TOP_FIRST) {
				if (!first)
					flags |= VA_DEINTERLACING_BOTTOM_FIELD;
			} else {
				flags |= VA_DEINTERLACING_BOTTOM_FIELD_FIRST;
				if (first)
					flags |= VA_DEINTERLACING_BOTTOM_FIELD;
			}
			if (!this->deint.doubler)
				flags |= VA_DEINTERLACING_ONE_FIELD;
			deinterlacer->setFlags(flags);
			return 0;
		} else {
			if (deinterlacer)
				pipeline->set(deinterlacer, false);
			if (!deint)
				return 0;
			return VaApi::toVAType(mp_fields, first);
		}
	}
};

VaApiPostProcessor::VaApiPostProcessor() : d(new Data) {
	VAConfigID config;
	d->dpy = VaApi::glx();
	if (!isSuccess(vaCreateConfig(d->dpy, VAProfileNone, VAEntrypointVideoProc, nullptr, 0, &config)))
		return;
	d->config = config;
	VAContextID context;
	if (!isSuccess(vaCreateContext(d->dpy, d->config, 0, 0, 0, NULL, 0, &context)))
		return;
	d->context = context;
	const auto filters = VaApi::filters();
	for (int i=0; i<filters.size(); ++i) {
		switch (filters[i].type()) {
		case VAProcFilterDeinterlacing:
			_New(d->deinterlacer, i, d->context);
			break;
		default:
			continue;
		}
	}
	if (!filters.isEmpty())
		_New(d->pipeline, d->context);
}

VaApiPostProcessor::~VaApiPostProcessor() {
	delete d->deinterlacer;
	if (d->context != VA_INVALID_ID)
		vaDestroyContext(d->dpy, d->context);
	if (d->context != VA_INVALID_ID)
		vaDestroyConfig(d->dpy, d->config);
	delete d;
}

void VaApiPostProcessor::setAvaiableList(const QList<VFType> &filters) {
	d->filters = filters;
	// TODO: update
}

void VaApiPostProcessor::setDeintOption(const DeintOption &option) {
	if (_Change(d->deint, option) && d->deinterlacer)
		d->deinterlacer->setMethod(d->deintType());
}

bool VaApiPostProcessor::process(const VideoFrame &in, QLinkedList<VideoFrame> &queue) {
	if (!d->pipeline)
		return false;
	auto surface = VaApiSurfacePool::getSurface(in.mpi());
	if (!surface)
		return false;
	mp_image *out1 = nullptr, *out2 = nullptr;
	out1 = d->pipeline->render(in, d->setupDeint(in.mpi()->fields, true));
	if (d->deint.doubler && d->deint.method != DeintMethod::None)
		out2 = d->pipeline->render(in, d->setupDeint(in.mpi()->fields, false));
	if (out1)
		queue.push_back(VideoFrame(true, out1, nextPTS(), in.field() & ~VideoFrame::Interlaced));
	if (out2)
		queue.push_back(VideoFrame(true, out2, nextPTS(), in.field() & ~VideoFrame::Interlaced));
	return out1 || out2;
}

#endif
