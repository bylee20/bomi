#ifndef SOFTWAREDEINTERLACER_HPP
#define SOFTWAREDEINTERLACER_HPP

#include "stdafx.hpp"

class DeintOption;        struct mp_image;

static constexpr int MP_IMGFIELD_ADDITIONAL = 0x100000;

class SoftwareDeinterlacer {
    enum Type {Graph, PP, Mark, Pass};
public:
    SoftwareDeinterlacer();
    ~SoftwareDeinterlacer();
    SoftwareDeinterlacer(const SoftwareDeinterlacer &other) = delete;
    SoftwareDeinterlacer &operator = (const SoftwareDeinterlacer &rhs) = delete;
    auto setOption(const DeintOption &deint) -> void;
    auto push(mp_image *mpi) -> void;
    auto pop() -> mp_image*;
    auto clear() -> void;
    auto peekNext() -> const mp_image* const;
private:
    struct Data;
    Data *d;
};

#endif // SOFTWAREDEINTERLACER_HPP
