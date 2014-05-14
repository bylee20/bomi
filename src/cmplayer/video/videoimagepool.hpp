#ifndef VIDEOIMAGEPOOL_HPP
#define VIDEOIMAGEPOOL_HPP

#include "stdafx.hpp"

template<class Image>
class VideoImagePool;

template<class Image>
class VideoImageCache {
    struct Data;
public:
    VideoImageCache() = default;
    VideoImageCache(const VideoImageCache &rhs)
        : VideoImageCache(rhs.d) { }
    VideoImageCache(VideoImageCache &&rhs) { std::swap(d, rhs.d); }
    ~VideoImageCache() { deref(); }
    operator bool() const { return !isNull(); }
    auto operator = (const VideoImageCache &rhs) -> VideoImageCache&
        { if (d != rhs.d) { rhs.ref(); deref(); d = rhs.d; } return *this; }
    auto operator * () const -> const Image& { return d->image; }
    auto operator -> () const -> const Image* { return &d->image; }
    auto operator = (VideoImageCache &&rhs) -> VideoImageCache&
        { std::swap(d, rhs.d); return *this; }
    auto isNull() const -> bool { return !d; }
    auto image() const -> const Image& { return d->image; }
    auto swap(VideoImageCache &other) -> void { std::swap(d, other.d); }
private:
    VideoImageCache(Data *data)
        : d(data) { ref(); }
    auto deref() const -> void
    {
        if (d && !d->ref.deref() && d->orphan.load())
            delete d;
    }
    auto ref() const -> void { if (d) d->ref.ref(); }
    struct Data { QAtomicInt ref, orphan; Image image; };
    Data *d = nullptr;
    friend class VideoImagePool<Image>;
};

template<class Image>
class VideoImagePool {
public:
    using Cache = VideoImageCache<Image>;
    using Data = typename Cache::Data;
    virtual ~VideoImagePool() { for (auto d : m_pool) free(d); }
    auto count() const -> int { return m_pool.size(); }
protected:
    template<class F>
    auto reserve(int count, F f) -> void;
    auto reserve(int count) -> void;
    auto recycle() -> Cache;
    auto getUnusedCache() -> Cache;
    auto getCache(int max = -1) -> Cache;
private:
    auto free(Data *d) -> void;
    QLinkedList<Data*> m_pool;
};

template<class Image>
template<class F>
inline auto VideoImagePool<Image>::reserve(int count, F f) -> void
{
    reserve(count);
    for (auto d : m_pool)
        f(d->image);
}

template<class Image>
inline auto VideoImagePool<Image>::reserve(int count) -> void
{
    while (m_pool.size() > count)
        free(m_pool.takeLast());
    while (m_pool.size() < count)
        m_pool.push_front(new Data);
}

template<class Image>
inline auto VideoImagePool<Image>::recycle() -> Cache
{
    Q_ASSERT(!m_pool.isEmpty());
    auto data = m_pool.takeFirst();
    m_pool.push_back(data);
    return Cache(data);
}

template<class Image>
inline auto VideoImagePool<Image>::getUnusedCache() -> Cache
{
    for (auto it = m_pool.begin(); it != m_pool.end(); ++it) {
        const int ref = (*it)->ref.load();
        Q_ASSERT(ref >= 0);
        if (!ref) {
            auto data = *it;
            m_pool.erase(it);
            m_pool.push_back(data);
            return Cache(data);
        }
    }
    return Cache();
}

template<class Image>
inline auto VideoImagePool<Image>::getCache(int max) -> Cache
{
    auto cache = getUnusedCache();
    if (!cache.isNull() || (max > 0 && m_pool.size() >= max))
        return cache;
    auto data = new Data;
    m_pool.push_back(data);
    return Cache(data);
}

template<class Image>
inline auto VideoImagePool<Image>::free(Data *d) -> void
{
    if (d->ref.load())
        d->orphan = true;
    else
        delete d;
}

#endif // VIDEOIMAGEPOOL_HPP
