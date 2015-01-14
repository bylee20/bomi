#ifndef SOFTWAREDEINTERLACER_HPP
#define SOFTWAREDEINTERLACER_HPP

class DeintOption;        class MpImage;

class SoftwareDeinterlacer {
public:
    enum Type {Graph, PP, Mark, Pass};
    SoftwareDeinterlacer();
    ~SoftwareDeinterlacer();
    SoftwareDeinterlacer(const SoftwareDeinterlacer &other) = delete;
    SoftwareDeinterlacer &operator = (const SoftwareDeinterlacer &rhs) = delete;
    auto setOption(const DeintOption &deint) -> void;
    auto push(MpImage &&mpi) -> void;
    auto pop() -> MpImage;
    auto clear() -> void;
    auto type() const -> Type;
    auto pass() const -> bool;
private:
    struct Data;
    Data *d;
};

#endif // SOFTWAREDEINTERLACER_HPP
