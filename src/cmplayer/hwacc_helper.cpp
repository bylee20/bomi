#include "hwacc_helper.hpp"

static QMutex mutex;

//VaApiSurface::~VaApiSurface() {
//	Q_ASSERT(!m_ref);
//	if (m_id != VA_INVALID_SURFACE)
//		vaDestroySurfaces(VaApi::glx(), &m_id, 1);
//}

//VAStatus VaApiSurfacePool::create(int size, int width, int height, uint format) {
//	if (m_width == width && m_height == height && m_format == format && m_surfaces.size() == size)
//		return isSuccess(VA_STATUS_SUCCESS);
//	clear();
//	m_width = width;
//	m_height = height;
//	m_format = format;
//	m_ids.resize(size);
//	if (!isSuccess(vaCreateSurfaces(VaApi::glx(), width, height, format, size, m_ids.data()))) {
//		m_ids.clear();
//		return status();
//	}
//	m_surfaces.resize(size);
//	for (int i=0; i<size; ++i) {
//		m_surfaces[i] = new VaApiSurface;
//		m_surfaces[i]->m_id = m_ids[i];
//		m_surfaces[i]->m_format = format;
//	}
//	return status();
//}

//mp_image *VaApiSurfacePool::getMpImage() {
//	auto surface = getSurface();
//	if (!surface)
//		return nullptr;
//	auto release = [](void *arg) {
//		mutex.lock();
//		auto surface = static_cast<VaApiSurface*>(arg);
//		surface->m_ref = false;
//		if (surface->m_orphan)
//			delete surface;
//		mutex.unlock();
//	};
//	auto mpi = nullMpImage(IMGFMT_VAAPI, m_width, m_height, surface, release);
//	mpi->planes[1] = (uchar*)(quintptr)surface;
//	mpi->planes[0] = mpi->planes[3] = (uchar*)(quintptr)surface->id();
//	return mpi;
//}

//void VaApiSurfacePool::clear() {
//	mutex.lock();
//	for (auto surface : m_surfaces) {
//		if (surface->m_ref)
//			surface->m_orphan = true;
//		else
//			delete surface;
//	}
//	m_surfaces.clear();
//	m_ids.clear();
//	mutex.unlock();
//}

//VaApiSurface *VaApiSurfacePool::getSurface(mp_image *mpi) {
//	return mpi->imgfmt == IMGFMT_VAAPI ? (VaApiSurface*)(quintptr)mpi->planes[1] : nullptr;
//}

//VaApiSurface *VaApiSurfacePool::getSurface() {
//	VaApiSurface *best = nullptr, *oldest = nullptr;
//	for (VaApiSurface *s : m_surfaces) {
//		if (!oldest || s->m_order < oldest->m_order)
//			oldest = s;
//		if (s->m_ref)
//			continue;
//		if (!best || best->m_order > s->m_order)
//			best = s;
//	}
//	if (!best) {
//		qDebug() << "No usable VASurfaceID!! decoding could fail";
//		best = oldest;
//	}
//	best->m_ref = true;
//	best->m_order = ++m_order;
//	return best;
//}

