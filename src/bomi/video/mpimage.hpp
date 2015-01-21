#ifndef MPIMAGE_HPP
#define MPIMAGE_HPP

extern "C" {
#include <video/mp_image.h>
}

#ifdef bool
#undef bool
#endif

class MpImage {
public:
    MpImage() { }
    MpImage(const MpImage &rhs): MpImage(makeRef(rhs.m)) { }
    MpImage(MpImage &&rhs): m(rhs.m) { rhs.m = nullptr; }
    ~MpImage() { talloc_free(m); }
    auto operator = (const MpImage &rhs) -> MpImage&
        { if (this != &rhs) { talloc_free(m); m = makeRef(rhs.m); } return *this; }
    auto operator = (MpImage &&rhs) -> MpImage&
        { std::swap(m, rhs.m); return *this; }
    auto operator * () const -> const mp_image& { return *m; }
    auto operator * () -> mp_image& { return *m; }
    auto operator -> () const -> const mp_image* { return m; }
    auto operator -> () -> mp_image* { return m; }
    auto isNull() const -> bool { return !m; }
    auto take() -> mp_image* { auto tmp = m; m = nullptr; return tmp; }
    auto release() -> void { talloc_free(m); m = nullptr; }
    auto swap(MpImage &rhs) -> void { std::swap(m, rhs.m); }
    auto data() const -> const mp_image* { return m; }
    auto data() -> mp_image* { return m; }
    auto isInterlaced() const -> bool
        { return m && (m->fields & MP_IMGFIELD_INTERLACED); }
    auto unset(int fields) -> void { if (m) m->fields &= ~fields; }
    auto set(int fields) -> void { if (m) m->fields |= fields; }
    static auto ref(mp_image *mpi) -> MpImage { return MpImage(makeRef(mpi)); }
    static auto wrap(mp_image *mpi) -> MpImage { return MpImage(mpi); }
    static auto wrap(mp_image *mpi, bool ref) -> MpImage
        { return ref ? MpImage::ref(mpi) : wrap(mpi); }
private:
    static auto makeRef(mp_image *m) -> mp_image*
        { return m ? mp_image_new_ref(m) : nullptr; }
    MpImage(mp_image *mpi): m(mpi) { }
    mp_image *m = nullptr;
};

namespace std {
template <>
inline auto swap(MpImage &lhs, MpImage &rhs) -> void { lhs.swap(rhs); }
}

#endif // MPIMAGE_HPP
