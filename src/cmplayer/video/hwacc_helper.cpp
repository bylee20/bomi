#include "hwacc_helper.hpp"

static mp_image null;

auto null_mp_image(void *arg, void(*free)(void*)) -> mp_image*
{
    return mp_image_new_custom_ref(&null, arg, free);
}

auto null_mp_image(uint imgfmt, int width, int height,
                   void *arg, void(*free)(void*)) -> mp_image*
{
    auto mpi = null_mp_image(arg, free);
    mp_image_setfmt(mpi, imgfmt);
    mp_image_set_size(mpi, width, height);
    return mpi;
}
