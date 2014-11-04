#ifndef SOFTWAREDEINTERLACER_HPP
#define SOFTWAREDEINTERLACER_HPP

class DeintOption;        class MpImage;

static constexpr int MP_IMGFIELD_ADDITIONAL = 0x100000;

class SoftwareDeinterlacer {
    enum Type {Graph, PP, Mark, Pass};
public:
    SoftwareDeinterlacer();
    ~SoftwareDeinterlacer();
    SoftwareDeinterlacer(const SoftwareDeinterlacer &other) = delete;
    SoftwareDeinterlacer &operator = (const SoftwareDeinterlacer &rhs) = delete;
    auto setOption(const DeintOption &deint) -> void;
    auto push(MpImage &&mpi) -> void;
    auto pop() -> MpImage;
    auto clear() -> void;
private:
    struct Data;
    Data *d;
};

#endif // SOFTWAREDEINTERLACER_HPP
