#ifndef HWACC_HELPER_HPP
#define HWACC_HELPER_HPP

#include "misc/log.hpp"
#include "videoimagepool.hpp"
extern "C" {
#include <libavcodec/avcodec.h>
#include <video/img_format.h>
#include <video/mp_image.h>
}

mp_image *null_mp_image(void *arg = nullptr, void(*free)(void*) = nullptr);
mp_image *null_mp_image(uint imgfmt, int width, int height,
                        void *arg = nullptr, void(*free)(void*) = nullptr);

#ifdef Q_OS_LINUX

template<mp_imgfmt imgfmt> struct HwAccX11Trait {
    static_assert(imgfmt == IMGFMT_VAAPI
                  || imgfmt == IMGFMT_VDPAU, "wrong format");
    using Profile = char*;
    using Status = char*;
    using SurfaceID = char*;
    static constexpr SurfaceID invalid = (SurfaceID)0;
    static constexpr Status success = (Status)0;
    static constexpr const char *name = "";
    static auto destroySurface(SurfaceID id) -> void;
    static auto createSurfaces(int w, int h, int f,
                               QVector<SurfaceID> &ids) -> bool;
    static auto error(Status) -> const char* { return ""; }
};

template<mp_imgfmt imgfmt>
struct HwAccX11StatusChecker {
    using Trait = HwAccX11Trait<imgfmt>;
    using Status = typename Trait::Status;
    static const char *getLogContext() { return Trait::name; }
    virtual ~HwAccX11StatusChecker() {}
    auto isSuccess(Status status) -> bool
        { m_status = status; return isSuccess(); }
    auto isSuccess() const -> bool { return m_status == Trait::success; }
    auto status() const -> Status { return m_status; }
    auto check(Status status, const QString &onError) -> bool
    {
        if (isSuccess(status))
            return true;
        _Error("Error: %%(0x%%) %%", Trait::error(status),
               QString::number(m_status, 16), onError);
        return false;
    }
    auto check(Status status, const char *onError = "") -> bool
        { return check(status, _L(onError)); }
private:
    Status m_status = Trait::success;
};

template<mp_imgfmt imgfmt>
struct HwAccX11Codec {
    using Trait = HwAccX11Trait<imgfmt>;
    using Profile = typename Trait::Profile;
    struct ProfilePair {
        ProfilePair() = default;
        ProfilePair(const Profile &hw, int ff): hw(hw), ff(ff) { }
        Profile hw; int ff = FF_PROFILE_UNKNOWN;
    };
    HwAccX11Codec() { }
    HwAccX11Codec(const QVector<Profile> &available,
                  const QVector<ProfilePair> &pairs, int surfaces, AVCodecID id)
    {
        for (auto &pair : pairs) {
            if (available.contains(pair.hw))
                profiles.push_back(pair);
        }
        if (!profiles.isEmpty()) {
            this->surfaces = surfaces;
            this->id = id;
        }
    }
    auto profile(int ff) const -> Profile {
        for (auto &pair : profiles) {
            if (pair.ff == ff)
                return pair.hw;
        }
        return profiles.front().hw;
    }
    AVCodecID id = AV_CODEC_ID_NONE;
    int surfaces = 0, level = 0;
    QVector<ProfilePair> profiles;
};

template<mp_imgfmt imgfmt> class HwAccX11SurfacePool;

template<mp_imgfmt imgfmt>
class HwAccX11Surface {
public:
    using Trait = HwAccX11Trait<imgfmt>;
    using SurfaceID = typename Trait::SurfaceID;
    auto id() const -> SurfaceID { return m_id; }
    auto format() const -> int { return m_format; }
    virtual ~HwAccX11Surface()
        { if (m_id != Trait::invalid) Trait::destroySurface(m_id); }
    HwAccX11Surface() = default;
private:
    friend class HwAccX11SurfacePool<imgfmt>;
    SurfaceID m_id = Trait::invalid;
    int m_format = 0;
};

template<mp_imgfmt imgfmt>
class HwAccX11SurfacePool : public VideoImagePool<HwAccX11Surface<imgfmt>> {
public:
    static const char *getLogContext() { return Trait::name; }
    using Trait = HwAccX11Trait<imgfmt>;
    using Surface = HwAccX11Surface<imgfmt>;
    using SurfaceID = typename Trait::SurfaceID;
    using Cache = VideoImageCache<Surface>;
    HwAccX11SurfacePool() = default;
    auto create(int size, int width, int height, uint format) -> bool
    {
        if (m_width == width && m_height == height
                && m_format == format && this->count() == size)
            return true;
        clear();
        m_width = width; m_height = height;
        m_format = format; m_ids.resize(size);
        if (!Trait::createSurfaces(m_width, m_height, m_format, m_ids)) {
            _Error("Cannot create hwacc surfaces. Decoding will fail.");
            m_ids.clear();
            return false;
        }
        int i = 0;
        this->reserve(size, [&] (Surface &surface) {
            surface.m_id = m_ids[i++];
            surface.m_format = format;
        });
        return true;
    }
    auto getMpImage() -> mp_image*
    {
        auto cache = this->getUnusedCache();
        if (cache.isNull()) {
            _Warn("No usable SurfaceID. Decoding could fail");
            cache = this->recycle();
        }
        auto release = [](void *arg) {
            delete static_cast<Cache*>(arg);
        };
        auto p = new Cache(std::move(cache));
        auto mpi = null_mp_image(IMGFMT_VAAPI, m_width, m_height, p, release);
        mpi->planes[1] = (uchar*)(quintptr)p;
        mpi->planes[0] = mpi->planes[3] = (uchar*)(quintptr)p->image().id();
        return mpi;
    }
    auto clear() -> void { this->reserve(0); m_ids.clear(); }
    auto ids() const -> QVector<SurfaceID> {return m_ids;}
    auto format() const -> uint {return m_format;}
    auto width() const -> int { return m_width; }
    auto height() const -> int { return m_height; }
    static auto getSurface(mp_image *mpi) -> Surface *
    {
        return mpi->imgfmt == imgfmt ? (Surface*)(quintptr)mpi->planes[1]
                                     : nullptr;
    }
private:
    QVector<SurfaceID> m_ids;
    uint m_format = 0;
    int m_width = 0, m_height = 0;
    quint64 m_order = 0LL;
};

#endif

#endif // HWACC_HELPER_HPP
