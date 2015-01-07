//#ifndef INTERPOLATOR_HPP
//#define INTERPOLATOR_HPP

//class OpenGLTexture1D;
//enum class InterpolatorType;

//class Interpolator {
//public:
//    enum Category {
//        None = 0,
//        Fetch16 = 1,
//        Fetch36 = 2,
//        Fetch64 = 3,
//        Fast4 = 4,
//        Fast9 = 5,
//        CategoryMax
//    };
//    using Type = InterpolatorType;
//    ~Interpolator();
//    auto type() const -> Type;
//    auto category() const -> Category;
//    static auto category(Type type) -> Category;
//    static auto shader(Category category) -> QByteArray;
//    static auto textures(Category category) -> int;
//    static const Interpolator *get(Type type);
//    auto shader() const -> QByteArray { return shader(category()); }
//    auto textures() const -> int { return textures(category()); }
//    auto allocate(OpenGLTexture1D *tex1, OpenGLTexture1D *tex2) const -> void;
//private:
//    Interpolator(Type type);
//    friend struct Objs;
//    struct Data;
//    Data *d;
//};

//#endif // INTERPOLATOR_HPP
