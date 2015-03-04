#ifndef SOFTWAREDEINTERLACER_HPP
#define SOFTWAREDEINTERLACER_HPP

#include "videofilter.hpp"

struct DeintOption;                     class MpImage;

class SoftwareDeinterlacer : public VideoFilter {
public:
    enum Type {Pass, Graph, Bob, Mark};
    SoftwareDeinterlacer();
    ~SoftwareDeinterlacer();
    SoftwareDeinterlacer(const SoftwareDeinterlacer &other) = delete;
    SoftwareDeinterlacer &operator = (const SoftwareDeinterlacer &rhs) = delete;
    auto setOption(const DeintOption &deint) -> void;
    auto push(MpImage &&mpi) -> void;
    auto pop() -> MpImage;
    auto clear() -> void;
    auto needsMore() const -> bool final { return false; }
    auto type() const -> Type;
    auto pass() const -> bool;
    auto fpsManipulation() const -> double final;
private:
    struct Data;
    Data *d;
};

#endif // SOFTWAREDEINTERLACER_HPP
