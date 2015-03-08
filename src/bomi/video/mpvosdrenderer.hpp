#ifndef MPVOSDRENDERER_HPP
#define MPVOSDRENDERER_HPP

#include "opengl/openglframebufferobject.hpp"

struct sub_bitmaps;

class MpvOsdRenderer {
public:
    MpvOsdRenderer();
    ~MpvOsdRenderer();
    auto prepare(OpenGLFramebufferObject *fbo) -> void;
    auto draw(const sub_bitmaps *imgs) -> void;
    auto end() -> void;
    auto initialize() -> void;
    auto finalize() -> void;
    static auto callback(void *ctx, struct sub_bitmaps *imgs) -> void;
private:
    struct Data;
    Data *d;
};

#endif // MPVOSDRENDERER_HPP
