//#include "interpolator.hpp"
//#include "enum/interpolatortype.hpp"
//#include "opengl/opengltexture1d.hpp"
//#include "opengl/opengltexturebinder.hpp"

//static constexpr int IntSamples = 256;
//static constexpr int IntLutSize = IntSamples*4;

//static QVector<double> interpolatorArray() {
//    QVector<double> array(IntSamples);
//    for (int i=0; i<array.size(); ++i)
//        array[i] = (double)i/(IntSamples-1);
//    Q_ASSERT(array.front() == 0.0);
//    Q_ASSERT(array.last() == 1.0);
//    array.front() = 1e-10;
//    array.last() = 1.0-1e-10;
//    return array;
//}

//static const QVector<double> as = interpolatorArray();

//template<class T>
//static QVector<T> convertToIntegerVector(const QVector<GLfloat> &v, float &mul) {
//    static_assert(std::is_unsigned<T>::value, "wrong type");
//    const int size = v.size();
//    QVector<T> ret(size);
//    auto s = v.data();
//    auto d = ret.data();
//    mul = 0.0;
//    for (auto f : v)
//        mul = qMax(mul, qAbs(f));
//    mul *= 2.0f;
//    for (int i=0; i<size; ++i)
//        *d++ = qBound<quint64>(0, ((*s++)/mul+0.5f)*_Max<T>(), _Max<T>());
//    return ret;
//}


//static double bicubic(double x, double b, double c) {
//    x = qAbs(x);
//    if (x < 1.0)
//        return ((12.0 - 9.0*b - 6.0*c)*x*x*x + (-18.0 + 12.0*b +  6.0*c)*x*x                        + (6.0 - 2.0*b         ))/6.0;
//    if (x < 2.0)
//        return ((          -b - 6.0*c)*x*x*x + (         6.0*b + 30.0*c)*x*x + (-12.0*b - 48.0*c)*x + (      8.0*b + 24.0*c))/6.0;
//    return 0.0;
//}

//static double lanczos(double x, double a) {
//    static constexpr auto pi = 3.14159265358979323846;
//    static constexpr auto e = std::numeric_limits<float>::epsilon();
//    x = qAbs(x);
//    if (x < e)
//        return 1.0;
//    const double pix = pi*x;
//    if (x <= a)
//        return qSin(pix)*qSin(pix/a)/(pix*(pix/a));
//    return 0.0;
//}

//static double spline16(double x) {
//    x = qAbs(x);
//    if (x < 1.0)
//        return ((         (x      ) - 9.0/5.0)*(x      ) - 1.0/5.0 )*(x      )+1.0;
//    if (x < 2.0)
//        return ((-1.0/3.0*(x - 1.0) + 4.0/5.0)*(x - 1.0) - 7.0/15.0)*(x - 1.0);
//    return 0.0;
//}

//static double spline36(double x) {
//    x = qAbs(x);
//    if (x < 1.0)
//        return ((13.0/11.0*(x      ) - 453.0/209.0)*(x      ) -   3.0/209.0)*(x      )+1.0;
//    if (x < 2.0)
//        return ((-6.0/11.0*(x - 1.0) + 270.0/209.0)*(x - 1.0) - 156.0/209.0)*(x - 1.0);
//    if (x < 3.0)
//        return (( 1.0/11.0*(x - 2.0) -  45.0/209.0)*(x - 2.0) +  26.0/209.0)*(x - 2.0);
//    return 0.0;
//}

//static double spline64(double x) {
//    x = qAbs(x);
//    if(x < 1.0)
//        return (( 49.0/41.0*(x      ) - 6387.0/2911.0)*(x      ) -    3.0/2911.0)*(x      ) + 1.0;
//    if(x < 2.0)
//        return ((-24.0/41.0*(x - 1.0) + 4032.0/2911.0)*(x - 1.0) - 2328.0/2911.0)*(x - 1.0);
//    if(x < 3.0)
//        return ((  6.0/41.0*(x - 2.0) - 1008.0/2911.0)*(x - 2.0) +  582.0/2911.0)*(x - 2.0);
//    if(x < 4.0)
//        return ((- 1.0/41.0*(x - 3.0) +  168.0/2911.0)*(x - 3.0) -   97.0/2911.0)*(x - 3.0);
//    return 0.0;
//}

//struct Objs {
//    QMap<InterpolatorType, Interpolator*> map;
//    ~Objs() { qDeleteAll(map); }
//};

//static Objs objs;


//struct Interpolator::Data {
//    Type type = Type::Bilinear;
//    Category category = None;
//    QVector<GLfloat> lut1, lut2;
//    template<class Func>
//    void fill(Func func, int n) {
//        auto p1 = lut1.data();
//        auto p2 = lut2.data();
//        QVector<double> ws(8, 0.0);
//        for (int i=0; i<IntSamples; ++i) {
//            const auto a = as[i];
//            double sum = 0.0;
//            for (int i=0; i<8; ++i) {
//                ws[i] = func(a + double((n/2) - 1 - i));
//                sum += ws[i];
//            }
//            for (int i=0; i<4; ++i) {
//                *p1++ = ws[i]/sum;
//                *p2++ = ws[i+4]/sum;
//            }
//        }
//    }

//    template<class Func>
//    void fillFast4(Func func) {
//        auto p = lut1.data();
//        for (int i=0; i<IntSamples; ++i) {
//            const auto a = as[i];
//            const auto w0 = func(a + 1.0);
//            const auto w1 = func(a + 0.0);
//            const auto w2 = func(a - 1.0);
//            const auto w3 = func(a - 2.0);
//            const auto g0 = w0 + w1;
//            const auto g1 = w2 + w3;
//            const auto h0 = 1.0 + a - w1/g0;
//            const auto h1 = 1.0 - a + w3/g1;
//            const auto f1 = g1/(g0 + g1);
//            *p++ = h0;    *p++ = h1;
//            *p++ = f1;    *p++ = .0;
//        }
//    }
//    template<class Func>
//    void fillFast9(Func func) {
//        auto p1 = lut1.data();
//        auto p2 = lut2.data();
//        for (int i=0; i<IntSamples; ++i) {
//            const auto a = as[i];
//            const auto w0 = func(a + 2.0);
//            const auto w1 = func(a + 1.0);
//            const auto w2 = func(a + 0.0);
//            const auto w3 = func(a - 1.0);
//            const auto w4 = func(a - 2.0);
//            const auto w5 = func(a - 3.0);
//            const auto g0 = w0 + w1;
//            const auto g1 = w2 + w3;
//            const auto g2 = w4 + w5;
//            const auto h0 = 2.0 + a - w1/g0;
//            const auto h1 = 0.0 + a - w3/g1;
//            const auto h2 = 2.0 - a + w5/g2;
//            const auto f0 = g1/(g0 + g1);
//            const auto f1 = g2/(g0 + g1 + g2);
//            *p1++ = h0;    *p1++ = h1;    *p1++ = h2;    *p1++ = .0;
//            *p2++ = f0;    *p2++ = f1;    *p2++ = .0;    *p2++ = .0;
//        }
//    }
//};

//Interpolator::Interpolator(Type type)
//: d(new Data) {
//    d->type = type;
//    d->category = category(d->type);

//    d->lut1.resize(IntLutSize);
//    d->lut2.resize(IntLutSize);
//    switch (d->type) {
//    case InterpolatorType::Bilinear:
//        break;
//    case InterpolatorType::BicubicBS:
//        d->fillFast4([] (double x) { return bicubic(x, 1.0, 0.0); });
//        break;
//    case InterpolatorType::BicubicCR:
//        d->fill([] (double x) { return bicubic(x, 0.0, 0.5); }, 4);
//        break;
//    case InterpolatorType::BicubicMN:
//        d->fill([] (double x) { return bicubic(x, 1.0/3.0, 1.0/3.0); }, 4);
//        break;
//    case InterpolatorType::Spline16:
//        d->fill(spline16, 4);
//        break;
//    case InterpolatorType::Lanczos2:
//        d->fill([] (double x) { return lanczos(x, 2.0); }, 4);
//        break;
//    case InterpolatorType::Spline36:
//        d->fill(spline36, 6);
//        break;
//    case InterpolatorType::Lanczos3:
//        d->fill([] (double x) { return lanczos(x, 3.0); }, 6);
//        break;
//    case InterpolatorType::Spline64:
//        d->fill(spline64, 8);
//        break;
//    case InterpolatorType::Lanczos4:
//        d->fill([] (double x) { return lanczos(x, 4.0); }, 8);
//        break;
////    case InterpolatorType::LanczosFast:
////        d->fillFast9([] (double x) { return lanczos(x, 3.0); });
////        break;
//    }
//}

//Interpolator::~Interpolator() {
//    delete d;
//}

//auto Interpolator::get(Type type) -> const Interpolator*
//{
//    auto &obj = objs.map[type];
//    if (!obj)
//        obj = new Interpolator(type);
//    return obj;
//}

//auto Interpolator::type() const -> InterpolatorType
//{
//    return d->type;
//}

//auto Interpolator::category() const -> Interpolator::Category
//{
//    return d->category;
//}

//auto Interpolator::category(Type type) -> Interpolator::Category
//{
//    switch (type) {
//    case InterpolatorType::Bilinear:
//        return None;
//    case InterpolatorType::BicubicBS:
//        return Fast4;
//    case InterpolatorType::BicubicCR:
//    case InterpolatorType::BicubicMN:
//    case InterpolatorType::Lanczos2:
//    case InterpolatorType::Spline16:
//        return Fetch16;
//    case InterpolatorType::Spline36:
//    case InterpolatorType::Lanczos3:
//        return Fetch36;
//    case InterpolatorType::Spline64:
//    case InterpolatorType::Lanczos4:
//        return Fetch64;
////    case InterpolatorType::LanczosFast:
////        return Fast9;
//    }
//    return None;
//}

//auto Interpolator::textures(Category category) -> int
//{
//    switch (category) {
//    case None:
//        return 0;
//    case Fetch16:
//    case Fast4:
//        return 1;
//    default:
//        return 2;
//    }
//}

//auto Interpolator::shader(Category category) -> QByteArray
//{
//    if (category == None)
//        return R"(
//#ifdef DEC_UNIFORM_DXY
//uniform vec2 dxy;
//uniform vec2 tex_size;
//#endif
//#ifdef FRAGMENT
//vec4 interpolated(const in sampler2Dg tex, const in vec2 coord) {
//    return texture2Dg(tex, coord);
//}
//#endif

//#ifdef VERTEX
//#define setLutIntCoord(a)
//#endif
//)";
//    QByteArray code;
//    code += R"(
//varying vec2 lutIntCoord;
//#ifdef DEC_UNIFORM_DXY
//uniform vec2 dxy;
//uniform vec2 tex_size;
//#endif

//#ifdef FRAGMENT
//uniform sampler1D lut_int1, lut_int2;
//vec4 mix3(const in vec4 v1, const in vec4 v2, const in vec4 v3, const in float a, const in float b) {
//    return mix(mix(v1, v2, a), v3, b);
//}

//vec4 interpolated(const in sampler2Dg tex, const in vec2 coord) {
//    const float N = 256.0;
//    const float scale = (N-1.0)/N;
//    const float offset = 1.0/(2.0*N);
//    vec2 lut_int_coord = fract(lutIntCoord);
//    vec2 lutCoord = scale*lut_int_coord + offset;
//    vec2 c = coord - lut_int_coord*dxy;
//    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
//    __SET_COLOR__
//    return color;
//}
//#endif

//#ifdef VERTEX
//void setLutIntCoord(const in vec2 vCoord) { lutIntCoord = vCoord/dxy - vec2(0.5, 0.5); }
//#endif
//)";

//    QByteArray interpolated;
//    switch (category) {
//    case Fetch16:
//        interpolated = R"(
//    vec4 w_x = texture1D(lut_int1, lutCoord.x);
//    vec4 w_y = texture1D(lut_int1, lutCoord.y);
//#define FETCH(a, b, i, j) (w_x.a*w_y.b)*texture2Dg(tex, c + vec2(i.0, j.0)*dxy)
//    color += FETCH(b, b,-1,-1);
//    color += FETCH(b, g,-1, 0);
//    color += FETCH(b, r,-1, 1);
//    color += FETCH(b, a,-1, 2);

//    color += FETCH(g, b, 0,-1);
//    color += FETCH(g, g, 0, 0);
//    color += FETCH(g, r, 0, 1);
//    color += FETCH(g, a, 0, 2);

//    color += FETCH(r, b, 1,-1);
//    color += FETCH(r, g, 1, 0);
//    color += FETCH(r, r, 1, 1);
//    color += FETCH(r, a, 1, 2);

//    color += FETCH(a, b, 2,-1);
//    color += FETCH(a, g, 2, 0);
//    color += FETCH(a, r, 2, 1);
//    color += FETCH(a, a, 2, 2);
//#undef FETCH
//    )";
//        break;
//    case Fetch36:
//    case Fetch64:
//        interpolated = R"(
//    vec4 w_x[2], w_y[2];
//    w_x[0] = texture1D(lut_int1, lutCoord.x);
//    w_x[1] = texture1D(lut_int2, lutCoord.x);
//    w_y[0] = texture1D(lut_int1, lutCoord.y);
//    w_y[1] = texture1D(lut_int2, lutCoord.y);
//#define FETCH(n, m, a, b, i, j) (w_x[n].a*w_y[m].b)*texture2Dg(tex, c + vec2(i.0, j.0)*dxy)
//)";
//        if (category == Fetch36)
//            interpolated += R"(
//    color += FETCH(0, 0, b, b,-2,-2);
//    color += FETCH(0, 0, b, g,-2,-1);
//    color += FETCH(0, 0, b, r,-2, 0);
//    color += FETCH(0, 0, b, a,-2, 1);
//    color += FETCH(0, 1, b, b,-2, 2);
//    color += FETCH(0, 1, b, g,-2, 3);

//    color += FETCH(0, 0, g, b,-1,-2);
//    color += FETCH(0, 0, g, g,-1,-1);
//    color += FETCH(0, 0, g, r,-1, 0);
//    color += FETCH(0, 0, g, a,-1, 1);
//    color += FETCH(0, 1, g, b,-1, 2);
//    color += FETCH(0, 1, g, g,-1, 3);

//    color += FETCH(0, 0, r, b, 0,-2);
//    color += FETCH(0, 0, r, g, 0,-1);
//    color += FETCH(0, 0, r, r, 0, 0);
//    color += FETCH(0, 0, r, a, 0, 1);
//    color += FETCH(0, 1, r, b, 0, 2);
//    color += FETCH(0, 1, r, g, 0, 3);

//    color += FETCH(0, 0, a, b, 1,-2);
//    color += FETCH(0, 0, a, g, 1,-1);
//    color += FETCH(0, 0, a, r, 1, 0);
//    color += FETCH(0, 0, a, a, 1, 1);
//    color += FETCH(0, 1, a, b, 1, 2);
//    color += FETCH(0, 1, a, g, 1, 3);

//    color += FETCH(1, 0, b, b, 2,-2);
//    color += FETCH(1, 0, b, g, 2,-1);
//    color += FETCH(1, 0, b, r, 2, 0);
//    color += FETCH(1, 0, b, a, 2, 1);
//    color += FETCH(1, 1, b, b, 2, 2);
//    color += FETCH(1, 1, b, g, 2, 3);

//    color += FETCH(1, 0, g, b, 3,-2);
//    color += FETCH(1, 0, g, g, 3,-1);
//    color += FETCH(1, 0, g, r, 3, 0);
//    color += FETCH(1, 0, g, a, 3, 1);
//    color += FETCH(1, 1, g, b, 3, 2);
//    color += FETCH(1, 1, g, g, 3, 3);
//)";
//        else
//            interpolated += R"(
//    color += FETCH(0, 0, b, b,-3,-3);
//    color += FETCH(0, 0, b, g,-3,-2);
//    color += FETCH(0, 0, b, r,-3,-1);
//    color += FETCH(0, 0, b, a,-3, 0);
//    color += FETCH(0, 1, b, b,-3, 1);
//    color += FETCH(0, 1, b, g,-3, 2);
//    color += FETCH(0, 1, b, r,-3, 3);
//    color += FETCH(0, 1, b, a,-3, 4);

//    color += FETCH(0, 0, g, b,-2,-3);
//    color += FETCH(0, 0, g, g,-2,-2);
//    color += FETCH(0, 0, g, r,-2,-1);
//    color += FETCH(0, 0, g, a,-2, 0);
//    color += FETCH(0, 1, g, b,-2, 1);
//    color += FETCH(0, 1, g, g,-2, 2);
//    color += FETCH(0, 1, g, r,-2, 3);
//    color += FETCH(0, 1, g, a,-2, 4);

//    color += FETCH(0, 0, r, b,-1,-3);
//    color += FETCH(0, 0, r, g,-1,-2);
//    color += FETCH(0, 0, r, r,-1,-1);
//    color += FETCH(0, 0, r, a,-1, 0);
//    color += FETCH(0, 1, r, b,-1, 1);
//    color += FETCH(0, 1, r, g,-1, 2);
//    color += FETCH(0, 1, r, r,-1, 3);
//    color += FETCH(0, 1, r, a,-1, 4);

//    color += FETCH(0, 0, a, b, 0,-3);
//    color += FETCH(0, 0, a, g, 0,-2);
//    color += FETCH(0, 0, a, r, 0,-1);
//    color += FETCH(0, 0, a, a, 0, 0);
//    color += FETCH(0, 1, a, b, 0, 1);
//    color += FETCH(0, 1, a, g, 0, 2);
//    color += FETCH(0, 1, a, r, 0, 3);
//    color += FETCH(0, 1, a, a, 0, 4);

//    color += FETCH(1, 0, b, b, 1,-3);
//    color += FETCH(1, 0, b, g, 1,-2);
//    color += FETCH(1, 0, b, r, 1,-1);
//    color += FETCH(1, 0, b, a, 1, 0);
//    color += FETCH(1, 1, b, b, 1, 1);
//    color += FETCH(1, 1, b, g, 1, 2);
//    color += FETCH(1, 1, b, r, 1, 3);
//    color += FETCH(1, 1, b, a, 1, 4);

//    color += FETCH(1, 0, g, b, 2,-3);
//    color += FETCH(1, 0, g, g, 2,-2);
//    color += FETCH(1, 0, g, r, 2,-1);
//    color += FETCH(1, 0, g, a, 2, 0);
//    color += FETCH(1, 1, g, b, 2, 1);
//    color += FETCH(1, 1, g, g, 2, 2);
//    color += FETCH(1, 1, g, r, 2, 3);
//    color += FETCH(1, 1, g, a, 2, 4);

//    color += FETCH(1, 0, r, b, 3,-3);
//    color += FETCH(1, 0, r, g, 3,-2);
//    color += FETCH(1, 0, r, r, 3,-1);
//    color += FETCH(1, 0, r, a, 3, 0);
//    color += FETCH(1, 1, r, b, 3, 1);
//    color += FETCH(1, 1, r, g, 3, 2);
//    color += FETCH(1, 1, r, r, 3, 3);
//    color += FETCH(1, 1, r, a, 3, 4);

//    color += FETCH(1, 0, a, b, 4,-3);
//    color += FETCH(1, 0, a, g, 4,-2);
//    color += FETCH(1, 0, a, r, 4,-1);
//    color += FETCH(1, 0, a, a, 4, 0);
//    color += FETCH(1, 1, a, b, 4, 1);
//    color += FETCH(1, 1, a, g, 4, 2);
//    color += FETCH(1, 1, a, r, 4, 3);
//    color += FETCH(1, 1, a, a, 4, 4);
//)";
//        interpolated += "#undef FETCH\n";
//        break;
//    case Fast4:
//        interpolated += R"(
//    vec4 hg_x = texture1D(lut_int1, lutCoord.x);
//    vec4 hg_y = texture1D(lut_int1, lutCoord.y);

//    vec4 tex00 = texture2Dg(tex, coord + vec2(-hg_x.b, -hg_y.b)*dxy);
//    vec4 tex10 = texture2Dg(tex, coord + vec2( hg_x.g, -hg_y.b)*dxy);
//    vec4 tex01 = texture2Dg(tex, coord + vec2(-hg_x.b,  hg_y.g)*dxy);
//    vec4 tex11 = texture2Dg(tex, coord + vec2( hg_x.g,  hg_y.g)*dxy);

//    tex00 = mix(tex00, tex10, hg_x.r);
//    tex01 = mix(tex01, tex11, hg_x.r);
//    color = mix(tex00, tex01, hg_y.r);
//)";
//        break;
//    case Fast9:
//        interpolated += R"(
//    vec4 h_x = texture1D(lut_int1, lutCoord.x)*dxy.x;
//    vec4 h_y = texture1D(lut_int1, lutCoord.y)*dxy.y;
//    vec4 f_x = texture1D(lut_int2, lutCoord.x);
//    vec4 f_y = texture1D(lut_int2, lutCoord.y);

//    vec4 tex00 = texture2Dg(tex, coord + vec2(-h_x.b, -h_y.b));
//    vec4 tex01 = texture2Dg(tex, coord + vec2(-h_x.b, -h_y.g));
//    vec4 tex02 = texture2Dg(tex, coord + vec2(-h_x.b,  h_y.r));
//    tex00 = mix3(tex00, tex01, tex02, f_y.b, f_y.g);

//    vec4 tex10 = texture2Dg(tex, coord + vec2(-h_x.g, -h_y.b));
//    vec4 tex11 = texture2Dg(tex, coord + vec2(-h_x.g, -h_y.g));
//    vec4 tex12 = texture2Dg(tex, coord + vec2(-h_x.g,  h_y.r));
//    tex10 = mix3(tex10, tex11, tex12, f_y.b, f_y.g);

//    vec4 tex20 = texture2Dg(tex, coord + vec2( h_x.r, -h_y.b));
//    vec4 tex21 = texture2Dg(tex, coord + vec2( h_x.r, -h_y.g));
//    vec4 tex22 = texture2Dg(tex, coord + vec2( h_x.r,  h_y.r));
//    tex20 = mix3(tex20, tex21, tex22, f_y.b, f_y.g);
//    color = mix3(tex00, tex10, tex20, f_x.b, f_x.g);
//)";
//        break;
//    default:
//        break;
//    }
//    code.replace("__SET_COLOR__", interpolated);
//    return code;
//}

//void Interpolator::allocate(OpenGLTexture1D *tex1, OpenGLTexture1D *tex2) const
//{
//    if (d->type == InterpolatorType::Bilinear)
//        return;
//    Q_ASSERT(tex1->id() != GL_NONE && tex2->id() != GL_NONE);
//    OpenGLTextureTransferInfo info(OGL::RGBA16F, OGL::BGRA, OGL::Float32);
//    const void *data1 = d->lut1.data();
//    const void *data2 = d->lut2.data();
//    OpenGLTextureBinder<OGL::Target1D> binder;
//    auto alloc = [&] (OpenGLTexture1D *tex, const void *data) {
//        tex->bind(); tex->initialize(IntSamples, info, data);
//    };
//    alloc(tex1, data1); alloc(tex2, data2);
//}
