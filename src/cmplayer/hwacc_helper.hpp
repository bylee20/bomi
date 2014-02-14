#ifndef HWACC_HELPER_HPP
#define HWACC_HELPER_HPP

#include "stdafx.hpp"
#include "log.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
#include <video/img_format.h>
#include <video/mp_image.h>
}

mp_image *null_mp_image(void *arg = nullptr, void(*free)(void*) = nullptr);
mp_image *null_mp_image(uint imgfmt, int width, int height, void *arg = nullptr, void(*free)(void*) = nullptr);

#ifdef Q_OS_LINUX

template<mp_imgfmt imgfmt> struct HwAccX11Trait {
	static_assert(imgfmt == IMGFMT_VAAPI || imgfmt == IMGFMT_VDPAU, "wrong format");
	using Profile = char*;
	using Status = char*;
	using SurfaceID = char*;
	static constexpr SurfaceID invalid = (SurfaceID)0;
	static constexpr Status success = (Status)0;
	static constexpr const char *name = "";
	static void destroySurface(SurfaceID id);
	static bool createSurfaces(int w, int h, int f, QVector<SurfaceID> &ids);
	static const char *error(Status status) { return ""; }
};

template<mp_imgfmt imgfmt>
struct HwAccX11StatusChecker {
	using Trait = HwAccX11Trait<imgfmt>;
	using Status = typename Trait::Status;
	static const char *getLogContext() { return Trait::name; }
	virtual ~HwAccX11StatusChecker() {}
	bool isSuccess(Status status) { m_status = status; return isSuccess(); }
	bool isSuccess() const { return m_status == Trait::success; }
	Status status() const { return m_status; }
	bool check(Status status, const QString &onError = QString()) {
		if (isSuccess(status))
			return true;
		_Error("Error %%(0x%%): %%", Trait::error(status), QString::number(m_status, 16), onError);
		return false;
	}
	bool check(Status status, const char *onError = "") { return check(status, _L(onError)); }
private:
	Status m_status = Trait::success;
};

template<mp_imgfmt imgfmt>
struct HwAccX11Codec {
	using Trait = HwAccX11Trait<imgfmt>;
	using Profile = typename Trait::Profile;
	HwAccX11Codec() {}
	HwAccX11Codec(const QVector<Profile> &available, const QVector<Profile> &p, const QVector<int> &av, int surfaces, AVCodecID id) {
		Q_ASSERT(p.size() == av.size());
		for (int i=0; i<p.size(); ++i) {
			if (available.contains(p[i])) {
				profiles.push_back(p[i]);
				avProfiles.push_back(av[i]);
			}
		}
		if (!profiles.isEmpty()) {
			this->surfaces = surfaces;
			this->id = id;
		}
	}
	Profile profile(int av) const {
		const int idx = avProfiles.indexOf(av);
		return profiles[idx < 0 || av == FF_PROFILE_UNKNOWN ? 0 : idx];
	}
	AVCodecID id = AV_CODEC_ID_NONE;
	int surfaces = 0, level = 0;
	QVector<Profile> profiles;
	QVector<int> avProfiles;
};

template<mp_imgfmt imgfmt> class HwAccX11SurfacePool;

template<mp_imgfmt imgfmt>
class HwAccX11Surface {
public:
	using Trait = HwAccX11Trait<imgfmt>;
	using Pool = HwAccX11SurfacePool<imgfmt>;
	using SurfaceID = typename Trait::SurfaceID;
	SurfaceID id() const { return m_id; }
	int format() const { return m_format; }
	Pool *pool() const { return m_pool; }
	virtual ~HwAccX11Surface() {
		Q_ASSERT(!m_ref);
		if (m_id != Trait::invalid)
			Trait::destroySurface(m_id);
	}
//private:
	HwAccX11Surface() = default;
	friend class HwAccX11SurfacePool<imgfmt>;
	SurfaceID m_id = Trait::invalid;
	bool m_ref = false, m_orphan = false;
	quint64 m_order = 0;
	int m_format = 0;
	Pool *m_pool = nullptr;
};

template<mp_imgfmt imgfmt>
class HwAccX11SurfacePool {
public:
	static const char *getLogContext() { return Trait::name; }
	using Trait = HwAccX11Trait<imgfmt>;
	using Surface = HwAccX11Surface<imgfmt>;
	using SurfaceID = typename Trait::SurfaceID;
	HwAccX11SurfacePool() = default;
	virtual ~HwAccX11SurfacePool() { clear(); }
	bool create(int size, int width, int height, uint format) {
		if (m_width == width && m_height == height && m_format == format && m_surfaces.size() == size)
			return true;
		clear();
		m_width = width; m_height = height;
		m_format = format; m_ids.resize(size);
		if (!Trait::createSurfaces(m_width, m_height, m_format, m_ids)) {
			_Error("Cannot create hwacc surfaces. Decoding will fail.");
			m_ids.clear();
			return false;
		}
		m_surfaces.resize(size);
		for (int i=0; i<size; ++i) {
			auto surface = new Surface;
			surface->m_id = m_ids[i];
			surface->m_format = format;
			surface->m_pool = this;
			m_surfaces[i] = surface;
		}
		return true;
	}
	static Surface *getSurface(mp_image *mpi) {
		return mpi->imgfmt == imgfmt ? (Surface*)(quintptr)mpi->planes[1] : nullptr;
	}
	mp_image *getMpImage() {
		auto surface = getSurface();
		if (!surface)
			return nullptr;
		auto release = [](void *arg) {
			m_mutex.lock();
			auto surface = static_cast<Surface*>(arg);
			surface->m_ref = false;
			if (surface->m_orphan)
				delete surface;
			m_mutex.unlock();
		};
		auto mpi = null_mp_image(IMGFMT_VAAPI, m_width, m_height, surface, release);
		mpi->planes[1] = (uchar*)(quintptr)surface;
		mpi->planes[0] = mpi->planes[3] = (uchar*)(quintptr)surface->id();
		return mpi;
	}
	void clear() {
		m_mutex.lock();
		for (auto surface : m_surfaces) {
			if (surface->m_ref)
				surface->m_orphan = true;
			else
				delete surface;
		}
		m_surfaces.clear();
		m_ids.clear();
		m_mutex.unlock();
	}
	QVector<SurfaceID> ids() const {return m_ids;}
	uint format() const {return m_format;}
	int width() const { return m_width; }
	int height() const { return m_height; }
private:
	static QMutex m_mutex;
	Surface *getSurface() {
		Surface *best = nullptr, *oldest = nullptr;
		for (Surface *s : m_surfaces) {
			if (!oldest || s->m_order < oldest->m_order)
				oldest = s;
			if (s->m_ref)
				continue;
			if (!best || best->m_order > s->m_order)
				best = s;
		}
		if (!best) {
			_Warn(_ByteArrayLiteral("No usable SurfaceID. Decoding could fail"));
			best = oldest;
		}
		best->m_ref = true;
		best->m_order = ++m_order;
		return best;
	}
	QVector<SurfaceID> m_ids;
	QVector<Surface*> m_surfaces;
	uint m_format = 0;
	int m_width = 0, m_height = 0;
	quint64 m_order = 0LL;
};

template<mp_imgfmt imgfmt> QMutex HwAccX11SurfacePool<imgfmt>::m_mutex;

#endif

#endif // HWACC_HELPER_HPP
