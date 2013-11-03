#include "openglcompat.hpp"
#include "videocolor.hpp"
#include <cmath>
extern "C" {
#include <video/out/dither.h>
}

OpenGLCompat OpenGLCompat::c;

void OpenGLCompat::fill(QOpenGLContext *ctx) {
	if (m_init)
		return;
	m_init = true;
	m_profile = QOpenGLVersionProfile{ctx->format()};
	const auto version = m_profile.version();
	m_major = version.first;
	m_minor = version.second;
	m_hasRG = m_major >= 3 || ctx->hasExtension("GL_ARB_texture_rg");
	m_hasFloat = m_major >= 3 || ctx->hasExtension("GL_ARB_texture_float");

	m_formats[0][GL_RED] = {GL_R8, GL_RED, GL_UNSIGNED_BYTE};
	m_formats[0][GL_RG] = {GL_RG8, GL_RG, GL_UNSIGNED_BYTE};
	m_formats[0][GL_LUMINANCE] = {GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE};
	m_formats[0][GL_LUMINANCE_ALPHA] = {GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE};
	m_formats[0][GL_RGB] = {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE};
	m_formats[0][GL_BGR] = {GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE};
	m_formats[0][GL_BGRA] = {GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV};
	m_formats[0][GL_RGBA] = {GL_RGBA8, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV};

	m_formats[1][GL_RED] = {GL_R16, GL_RED, GL_UNSIGNED_SHORT};
	m_formats[1][GL_RG] = {GL_RG16, GL_RG, GL_UNSIGNED_SHORT};
	m_formats[1][GL_LUMINANCE] = {GL_LUMINANCE16, GL_LUMINANCE, GL_UNSIGNED_SHORT};
	m_formats[1][GL_LUMINANCE_ALPHA] = {GL_LUMINANCE16_ALPHA16, GL_LUMINANCE_ALPHA, GL_UNSIGNED_SHORT};
	m_formats[1][GL_RGB] = {GL_RGB16, GL_RGB, GL_UNSIGNED_SHORT};
	m_formats[1][GL_BGR] = {GL_RGB16, GL_BGR, GL_UNSIGNED_SHORT};
	m_formats[1][GL_BGRA] = {GL_RGBA16, GL_BGRA, GL_UNSIGNED_SHORT};
	m_formats[1][GL_RGBA] = {GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT};
	for (auto &formats : m_formats) {
		formats[1] = m_hasRG ? formats[GL_RED] : formats[GL_LUMINANCE];
		formats[2] = m_hasRG ? formats[GL_RG]  : formats[GL_LUMINANCE_ALPHA];
		formats[3] = formats[GL_BGR];
		formats[4] = formats[GL_BGRA];
	}

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);

	m_bicubicParams[(int)InterpolatorType::BicubicBS] = {1.0, 0.0};
	m_bicubicParams[(int)InterpolatorType::BicubicCR] = {0.0, 0.5};
	m_bicubicParams[(int)InterpolatorType::BicubicMN] = {1./3., 1./3.};
}

template<typename T>
static QVector<T> convertToIntegerVector(const QVector<GLfloat> &v, float &mul) {
	static_assert(std::is_unsigned<T>::value, "wrong type");
	const int size = v.size();
	QVector<T> ret(size);
	auto s = v.data();
	auto d = ret.data();
	mul = 0.0;
	for (auto f : v)
		mul = qMax(mul, qAbs(f));
	mul *= 2.0f;
	for (int i=0; i<size; ++i)
		*d++ = qBound<quint64>(0, ((*s++)/mul+0.5f)*_Max<T>(), _Max<T>());
	return ret;
}

static QVector<double> interpolatorArray() {
	QVector<double> array(OpenGLCompat::IntSamples);
	for (int i=0; i<array.size(); ++i)
		array[i] = (double)i/(OpenGLCompat::IntSamples-1);
	Q_ASSERT(array.front() == 0.0);
	Q_ASSERT(array.last() == 1.0);
	array.front() = 1e-10;
	array.last() = 1.0-1e-10;
	return array;
}

bool isNormalized(float v) { return 0.0 <= v && v <= 1.0; }

template<typename Func>
static void makeInterpolatorLut4(QVector<GLfloat> &lut, Func func) {
	lut.resize(OpenGLCompat::IntLutSize);
	auto p = lut.data();
	auto as = interpolatorArray();

	for (int i=0; i<OpenGLCompat::IntSamples; ++i) {
		const auto a = as[i];
		const auto w0 = func(a + 1.0);
		const auto w1 = func(a + 0.0);
		const auto w2 = func(a - 1.0);
		const auto w3 = func(a - 2.0);
		const auto div = w0 + w1 + w2 + w3;
		*p++ = w0/div;
		*p++ = w1/div;
		*p++ = w2/div;
		*p++ = w3/div;
	}
}

template<typename Func>
static void makeInterpolatorLut6(QVector<GLfloat> &lut1, QVector<GLfloat> &lut2, Func func) {
	lut1.resize(OpenGLCompat::IntLutSize);
	lut2.resize(OpenGLCompat::IntLutSize);
	auto p1 = lut1.data();
	auto p2 = lut2.data();
	auto as = interpolatorArray();
	for (int i=0; i<OpenGLCompat::IntSamples; ++i) {
		const auto a = as[i];
		const auto w0 = func(a + 2.0);
		const auto w1 = func(a + 1.0);
		const auto w2 = func(a + 0.0);
		const auto w3 = func(a - 1.0);
		const auto w4 = func(a - 2.0);
		const auto w5 = func(a - 3.0);
		const auto div = w0 + w1 + w2 + w3 + w4 + w5;
		*p1++ = w0/div;
		*p1++ = w1/div;
		*p1++ = w2/div;
		*p1++ = w3/div;

		*p2++ = w4/div;
		*p2++ = w5/div;
		*p2++ = 0.0;
		*p2++ = 0.0;

		qDebug() << a << w1/(w0+w1) << w3/(w2+w3) << w5/(w4+w5) << "w" << w0 << w1 << w4 << w5;
	}
}

template<typename Func>
static void makeInterpolatorLut8(QVector<GLfloat> &lut1, QVector<GLfloat> &lut2, Func func) {
	lut1.resize(OpenGLCompat::IntLutSize);
	lut2.resize(OpenGLCompat::IntLutSize);
	auto p1 = lut1.data();
	auto p2 = lut2.data();
	auto as = interpolatorArray();
	for (int i=0; i<OpenGLCompat::IntSamples; ++i) {
		const auto a = as[i];
		const auto w0 = func(a + 3.0);
		const auto w1 = func(a + 2.0);
		const auto w2 = func(a + 1.0);
		const auto w3 = func(a + 0.0);
		const auto w4 = func(a - 1.0);
		const auto w5 = func(a - 2.0);
		const auto w6 = func(a - 3.0);
		const auto w7 = func(a - 4.0);
		const auto div = w0 + w1 + w2 + w3 + w4 + w5 + w6 + w7;
		*p1++ = w0/div;
		*p1++ = w1/div;
		*p1++ = w2/div;
		*p1++ = w3/div;
		*p2++ = w4/div;
		*p2++ = w5/div;
		*p2++ = w6/div;
		*p2++ = w7/div;
	}
}

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

static double bicubic(double x, double b, double c) {
	x = qAbs(x);
	if (x < 1.0)
		return ((12.0 - 9.0*b - 6.0*c)*x*x*x + (-18.0 + 12.0*b +  6.0*c)*x*x                        + (6.0 - 2.0*b         ))/6.0;
	if (x < 2.0)
		return ((          -b - 6.0*c)*x*x*x + (         6.0*b + 30.0*c)*x*x + (-12.0*b - 48.0*c)*x + (      8.0*b + 24.0*c))/6.0;
	return 0.0;
}

static double lanczos(double x, double a) {
	static constexpr auto e = std::numeric_limits<float>::epsilon();
	x = qAbs(x);
	if (x < e)
		return 1.0;
	const double pix = M_PI*x;
	if (x <= a)
		return qSin(pix)*qSin(pix/a)/(pix*(pix/a));
	return 0.0;
}

static double spline16(double x) {
	x = qAbs(x);
	if (x < 1.0)
		return ((         (x      ) - 9.0/5.0)*(x      ) - 1.0/5.0 )*(x      )+1.0;
	if (x < 2.0)
		return ((-1.0/3.0*(x - 1.0) + 4.0/5.0)*(x - 1.0) - 7.0/15.0)*(x - 1.0);
	return 0.0;
}

static double spline36(double x) {
	x = qAbs(x);
	if (x < 1.0)
		return ((13.0/11.0*(x    ) - 453.0/209.0)*(x    ) -   3.0/209.0)*(x    )+1.0;
	if (x < 2.0)
		return ((-6.0/11.0*(x - 1) + 270.0/209.0)*(x - 1) - 156.0/209.0)*(x - 1);
	if (x < 3.0)
		return (( 1.0/11.0*(x - 2) -  45.0/209.0)*(x - 2) +  26.0/209.0)*(x - 2);
	return 0.0;
}

static double spline64(double x) {
	x = qAbs(x);
	if(x < 1.0)
		return (( 49.0/41.0*(x      ) - 6387.0/2911.0)*(x      ) -    3.0/2911.0)*(x      ) + 1.0;
	if(x < 2.0)
		return ((-24.0/41.0*(x - 1.0) + 4032.0/2911.0)*(x - 1.0) - 2328.0/2911.0)*(x - 1.0);
	if(x < 3.0)
		return ((  6.0/41.0*(x - 2.0) - 1008.0/2911.0)*(x - 2.0) +  582.0/2911.0)*(x - 2.0);
	if(x < 4.0)
		return ((- 1.0/41.0*(x - 3.0) +  168.0/2911.0)*(x - 3.0) -   97.0/2911.0)*(x - 3.0);
	return 0.0;
}

void OpenGLCompat::fillInterpolatorLut(InterpolatorType interpolator) {
	const int type = (int)interpolator;
	auto &lut1 = m_intLuts1[type];
	auto &lut2 = m_intLuts2[type];
	switch (interpolator) {
	case InterpolatorType::Bilinear:
		break;
	case InterpolatorType::BicubicBS:
	case InterpolatorType::BicubicCR:
	case InterpolatorType::BicubicMN: {
		const double b = m_bicubicParams[type].first;
		const double c = m_bicubicParams[type].second;
		makeInterpolatorLut4(lut1, [b, c] (double x) { return bicubic(x, b, c); });
		break;
	} case InterpolatorType::Spline16:
		makeInterpolatorLut4(lut1, spline16);
		break;
	case InterpolatorType::Lanczos2:
		makeInterpolatorLut4(lut1, [] (double x) { return lanczos(x, 2.0); });
		break;
	case InterpolatorType::Spline36:
		makeInterpolatorLut6(lut1, lut2, spline36);
		break;
	case InterpolatorType::Lanczos3:
		makeInterpolatorLut6(lut1, lut2, [] (double x) { return lanczos(x, 3.0); });
		break;
	case InterpolatorType::Spline64:
		makeInterpolatorLut8(lut1, lut2, spline64);
		break;
	case InterpolatorType::Lanczos4:
		makeInterpolatorLut8(lut1, lut2, [] (double x) { return lanczos(x, 4.0); });
		break;
	}
}

void OpenGLCompat::allocateInterpolatorLutTexture(InterpolatorLutTexture &texture1, InterpolatorLutTexture &texture2, InterpolatorType interpolator) {
	if (interpolator == InterpolatorType::Bilinear)
		return;
	Q_ASSERT(texture1.id != GL_NONE && texture2.id != GL_NONE);
	auto &lut1 = c.m_intLuts1[(int)interpolator];
	auto &lut2 = c.m_intLuts2[(int)interpolator];
	if (lut1.isEmpty())
		c.fillInterpolatorLut(interpolator);
	Q_ASSERT(!lut1.isEmpty());
	texture1.target = GL_TEXTURE_1D;
	texture1.width = IntSamples;
	texture1.height = 0;
	texture1.format.pixel = GL_BGRA;
	if (c.m_hasFloat) {
		texture1.format.internal = GL_RGBA16F;
		texture1.format.type = GL_FLOAT;
		texture1.allocate(GL_LINEAR, GL_CLAMP_TO_EDGE, lut1.data());
		if (!lut2.isEmpty()) {
			texture2.copyAttributesFrom(texture1);
			texture2.allocate(GL_LINEAR, GL_CLAMP_TO_EDGE, lut2.data());
		}
	} else {
		texture1.format.internal = GL_RGBA16;
		texture1.format.type = GL_UNSIGNED_SHORT;
		auto data = convertToIntegerVector<GLushort>(lut1, texture1.multiply);
		texture1.allocate(GL_LINEAR, GL_CLAMP_TO_EDGE, data.data());
		if (!lut2.isEmpty()) {
			texture2.copyAttributesFrom(texture1);
			data = convertToIntegerVector<GLushort>(lut2, texture2.multiply);
			texture2.allocate(GL_LINEAR, GL_CLAMP_TO_EDGE, data.data());
		}
	}
}

static QVector3D operator*(const QMatrix3x3 &mat, const QVector3D &vec) {
	QVector3D ret;
	ret.setX(mat(0, 0)*vec.x() + mat(0, 1)*vec.y() + mat(0, 2)*vec.z());
	ret.setY(mat(1, 0)*vec.x() + mat(1, 1)*vec.y() + mat(1, 2)*vec.z());
	ret.setZ(mat(2, 0)*vec.x() + mat(2, 1)*vec.y() + mat(2, 2)*vec.z());
	return ret;
}

constexpr static const int GLushortMax = std::numeric_limits<GLushort>::max();

void OpenGLCompat::upload3dLutTexture(const OpenGLTexture &texture, const QVector3D &sub, const QMatrix3x3 &mul, const QVector3D &add) {
	const int length = texture.width*texture.height*texture.depth*4;
	if (length != c.m_3dLut.size() || c.m_subLut != sub || c.m_addLut != add || c.m_mulLut != mul) {
		c.m_subLut = sub;
		c.m_addLut = add;
		c.m_mulLut = mul;
		c.m_3dLut.resize(length);
		auto p = c.m_3dLut.data();
		auto conv = [] (float v) { v = qBound(0.f, v, 1.f); return (GLushort)(v*GLushortMax); };
		for (int z=0; z<texture.depth; ++z) {
			for (int y=0; y<texture.height; ++y) {
				for (int x=0; x<texture.width; ++x) {
					QVector3D color(x/float(texture.width-1), y/float(texture.height-1), z/float(texture.depth-1));
					color -= sub;
					color = mul*color;
					color += add;
					*p++ = conv(color.z());
					*p++ = conv(color.y());
					*p++ = conv(color.x());
					*p++ = GLushortMax;
				}
			}
		}
	}
	texture.upload(c.m_3dLut.data());
}

OpenGLTexture OpenGLCompat::allocate3dLutTexture(GLuint id) {
	OpenGLTexture texture;
	texture.target = GL_TEXTURE_3D;
	texture.depth = texture.height = texture.width = 32;
	texture.format.internal = GL_RGBA16;
	texture.format.pixel = GL_BGRA;
	texture.format.type = GL_UNSIGNED_SHORT;
	texture.id = id;
	texture.bind();
	texture.allocate(GL_LINEAR, GL_CLAMP_TO_EDGE, nullptr);
	return texture;
}

// copied from mpv's gl_video.c
OpenGLTexture OpenGLCompat::allocateDitheringTexture(GLuint id, Dithering type) {
	OpenGLTexture texture;
	texture.id = id;
	if (type == Dithering::None)
		return texture;
	const int sizeb = 6;
	int size = 0;
	QByteArray data;
	if (type == Dithering::Fruit) {
		size = 1 << 6;
		auto &fruit = c.m_fruit;
		if (fruit.size() != size*size) {
			fruit.resize(size*size);
			mp_make_fruit_dither_matrix(fruit.data(), sizeb);
		}
		texture.format.internal = c.m_hasRG ? GL_R16 : GL_LUMINANCE16;
		texture.format.pixel = c.m_hasRG ? GL_RED : GL_LUMINANCE;
		if (c.m_hasFloat) {
			texture.format.type = GL_FLOAT;
			data.resize(sizeof(GLfloat)*fruit.size());
			memcpy(data.data(), fruit.data(), data.size());
		} else {
			texture.format.type = GL_UNSIGNED_SHORT;
			data.resize(sizeof(GLushort)*fruit.size());
			auto p = (GLushort*)data.data();
			for (auto v : fruit)
				*p++ = v*GLushortMax;
		}
	} else {
		size = 8;
		data.resize(size*size);
		mp_make_ordered_dither_matrix((uchar*)data.data(), size);
		texture.format = textureFormat(1);
	}
	texture.width = texture.height = size;
	texture.target = GL_TEXTURE_2D;
	//	 gl->PixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//	 gl->PixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	texture.allocate(GL_NEAREST, GL_REPEAT, data.data());
	return texture;
}

QByteArray OpenGLCompat::interpolatorCodes(int category) {
	QByteArray code;
	if (c.m_hasFloat)
		code += R"(
#ifndef HAS_FLOAT_TEXTURE
#define HAS_FLOAT_TEXTURE 1
#endif
)";
	if (category >= 0)
		code += "#define USE_INTERPOLATOR " + QByteArray::number(category) + "\n";
	code += R"(
#if USE_INTERPOLATOR
varying vec2 lutIntCoord;
#ifdef DEC_UNIFORM_DXY
uniform vec2 dxy;
uniform vec2 tex_size;
#endif
#endif

#ifdef FRAGMENT
#if USE_INTERPOLATOR
uniform sampler1D lut_int1;
uniform float lut_int1_mul;
#if HAS_FLOAT_TEXTURE
#define renormalize(a, b) (a)
#else
vec4 renormalize(const in vec4 v, float mul) {
	return v*mul-0.5*mul;
}
#endif
#if USE_INTERPOLATOR > 1
uniform sampler1D lut_int2;
uniform float lut_int2_mul;

vec4 fetch(const in sampler2D tex, const in vec2 c) {
	vec2 coord = floor(c);
	vec4 tex00 = texture2D(tex, coord);
	vec4 tex10 = texture2D(tex, coord + vec2(dxy.x, 0.0));
	vec4 tex01 = texture2D(tex, coord + vec2(0.0, dxy.y));
	vec4 tex11 = texture2D(tex, coord + dxy);
	vec2 a = fract(c);
	tex00 = mix(tex00, tex10, a.x);
	tex01 = mix(tex01, tex11, a.x);
	return mix(tex00, tex01, a.y);
}

#if USE_INTERPOLATOR == 2
vec4 mix3(const in vec4 v1, const in vec4 v2, const in vec4 v3, const in float a, const in float b) {
	return mix(mix(v1, v2, a), v3, b);
}
#elif USE_INTERPOLATOR == 3
vec4 mix4(const in vec4 v1, const in vec4 v2, const in vec4 v3, const in vec4 v4, const in float a, const in float b, const in float c) {
	return mix(mix(v1, v2, a), mix(v3, v4, b), c);
}
#endif
#endif
#endif

vec4 interpolated(const in sampler2D tex, const in vec2 coord) {
#if USE_INTERPOLATOR < 1
	return texture2D(tex, coord);
#else
	const float N = 256.0;
	const float scale = (N-1.0)/N;
	const float offset = 1.0/(2.0*N);
	vec2 lut_int_coord = fract(lutIntCoord);
	vec2 lutCoord = scale*lut_int_coord + offset;
	vec2 c = coord - lut_int_coord*dxy;
	vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
# if USE_INTERPOLATOR < 2
	vec4 w_x = renormalize(texture1D(lut_int1, lutCoord.x), lut_int1_mul);
	vec4 w_y = renormalize(texture1D(lut_int1, lutCoord.y), lut_int1_mul);
#define FETCH(a, b, i, j) (w_x.a*w_y.b)*texture2D(tex, c + vec2(i.0, j.0)*dxy)
	color += FETCH(b, b,-1,-1);
	color += FETCH(b, g,-1, 0);
	color += FETCH(b, r,-1, 1);
	color += FETCH(b, a,-1, 2);

	color += FETCH(g, b, 0,-1);
	color += FETCH(g, g, 0, 0);
	color += FETCH(g, r, 0, 1);
	color += FETCH(g, a, 0, 2);

	color += FETCH(r, b, 1,-1);
	color += FETCH(r, g, 1, 0);
	color += FETCH(r, r, 1, 1);
	color += FETCH(r, a, 1, 2);

	color += FETCH(a, b, 2,-1);
	color += FETCH(a, g, 2, 0);
	color += FETCH(a, r, 2, 1);
	color += FETCH(a, a, 2, 2);
#undef FETCH
# else
	vec4 w_x[2], w_y[2];
	w_x[0] = renormalize(texture1D(lut_int1, lutCoord.x), lut_int1_mul);
	w_x[1] = renormalize(texture1D(lut_int2, lutCoord.x), lut_int2_mul);
	w_y[0] = renormalize(texture1D(lut_int1, lutCoord.y), lut_int1_mul);
	w_y[1] = renormalize(texture1D(lut_int2, lutCoord.y), lut_int2_mul);
#define FETCH(n, m, a, b, i, j) (w_x[n].a*w_y[m].b)*texture2D(tex, c + vec2(i.0, j.0)*dxy)
#  if USE_INTERPOLATOR == 2
	color += FETCH(0, 0, b, b,-2,-2);
	color += FETCH(0, 0, b, g,-2,-1);
	color += FETCH(0, 0, b, r,-2, 0);
	color += FETCH(0, 0, b, a,-2, 1);
	color += FETCH(0, 1, b, b,-2, 2);
	color += FETCH(0, 1, b, g,-2, 3);

	color += FETCH(0, 0, g, b,-1,-2);
	color += FETCH(0, 0, g, g,-1,-1);
	color += FETCH(0, 0, g, r,-1, 0);
	color += FETCH(0, 0, g, a,-1, 1);
	color += FETCH(0, 1, g, b,-1, 2);
	color += FETCH(0, 1, g, g,-1, 3);

	color += FETCH(0, 0, r, b, 0,-2);
	color += FETCH(0, 0, r, g, 0,-1);
	color += FETCH(0, 0, r, r, 0, 0);
	color += FETCH(0, 0, r, a, 0, 1);
	color += FETCH(0, 1, r, b, 0, 2);
	color += FETCH(0, 1, r, g, 0, 3);

	color += FETCH(0, 0, a, b, 1,-2);
	color += FETCH(0, 0, a, g, 1,-1);
	color += FETCH(0, 0, a, r, 1, 0);
	color += FETCH(0, 0, a, a, 1, 1);
	color += FETCH(0, 1, a, b, 1, 2);
	color += FETCH(0, 1, a, g, 1, 3);

	color += FETCH(1, 0, b, b, 2,-2);
	color += FETCH(1, 0, b, g, 2,-1);
	color += FETCH(1, 0, b, r, 2, 0);
	color += FETCH(1, 0, b, a, 2, 1);
	color += FETCH(1, 1, b, b, 2, 2);
	color += FETCH(1, 1, b, g, 2, 3);

	color += FETCH(1, 0, g, b, 3,-2);
	color += FETCH(1, 0, g, g, 3,-1);
	color += FETCH(1, 0, g, r, 3, 0);
	color += FETCH(1, 0, g, a, 3, 1);
	color += FETCH(1, 1, g, b, 3, 2);
	color += FETCH(1, 1, g, g, 3, 3);
#  elif USE_INTERPOLATOR == 3
	color += FETCH(0, 0, b, b,-3,-3);
	color += FETCH(0, 0, b, g,-3,-2);
	color += FETCH(0, 0, b, r,-3,-1);
	color += FETCH(0, 0, b, a,-3, 0);
	color += FETCH(0, 1, b, b,-3, 1);
	color += FETCH(0, 1, b, g,-3, 2);
	color += FETCH(0, 1, b, r,-3, 3);
	color += FETCH(0, 1, b, a,-3, 4);

	color += FETCH(0, 0, g, b,-2,-3);
	color += FETCH(0, 0, g, g,-2,-2);
	color += FETCH(0, 0, g, r,-2,-1);
	color += FETCH(0, 0, g, a,-2, 0);
	color += FETCH(0, 1, g, b,-2, 1);
	color += FETCH(0, 1, g, g,-2, 2);
	color += FETCH(0, 1, g, r,-2, 3);
	color += FETCH(0, 1, g, a,-2, 4);

	color += FETCH(0, 0, r, b,-1,-3);
	color += FETCH(0, 0, r, g,-1,-2);
	color += FETCH(0, 0, r, r,-1,-1);
	color += FETCH(0, 0, r, a,-1, 0);
	color += FETCH(0, 1, r, b,-1, 1);
	color += FETCH(0, 1, r, g,-1, 2);
	color += FETCH(0, 1, r, r,-1, 3);
	color += FETCH(0, 1, r, a,-1, 4);

	color += FETCH(0, 0, a, b, 0,-3);
	color += FETCH(0, 0, a, g, 0,-2);
	color += FETCH(0, 0, a, r, 0,-1);
	color += FETCH(0, 0, a, a, 0, 0);
	color += FETCH(0, 1, a, b, 0, 1);
	color += FETCH(0, 1, a, g, 0, 2);
	color += FETCH(0, 1, a, r, 0, 3);
	color += FETCH(0, 1, a, a, 0, 4);

	color += FETCH(1, 0, b, b, 1,-3);
	color += FETCH(1, 0, b, g, 1,-2);
	color += FETCH(1, 0, b, r, 1,-1);
	color += FETCH(1, 0, b, a, 1, 0);
	color += FETCH(1, 1, b, b, 1, 1);
	color += FETCH(1, 1, b, g, 1, 2);
	color += FETCH(1, 1, b, r, 1, 3);
	color += FETCH(1, 1, b, a, 1, 4);

	color += FETCH(1, 0, g, b, 2,-3);
	color += FETCH(1, 0, g, g, 2,-2);
	color += FETCH(1, 0, g, r, 2,-1);
	color += FETCH(1, 0, g, a, 2, 0);
	color += FETCH(1, 1, g, b, 2, 1);
	color += FETCH(1, 1, g, g, 2, 2);
	color += FETCH(1, 1, g, r, 2, 3);
	color += FETCH(1, 1, g, a, 2, 4);

	color += FETCH(1, 0, r, b, 3,-3);
	color += FETCH(1, 0, r, g, 3,-2);
	color += FETCH(1, 0, r, r, 3,-1);
	color += FETCH(1, 0, r, a, 3, 0);
	color += FETCH(1, 1, r, b, 3, 1);
	color += FETCH(1, 1, r, g, 3, 2);
	color += FETCH(1, 1, r, r, 3, 3);
	color += FETCH(1, 1, r, a, 3, 4);

	color += FETCH(1, 0, a, b, 4,-3);
	color += FETCH(1, 0, a, g, 4,-2);
	color += FETCH(1, 0, a, r, 4,-1);
	color += FETCH(1, 0, a, a, 4, 0);
	color += FETCH(1, 1, a, b, 4, 1);
	color += FETCH(1, 1, a, g, 4, 2);
	color += FETCH(1, 1, a, r, 4, 3);
	color += FETCH(1, 1, a, a, 4, 4);
#  endif
#undef FETCH
# endif
	return color;
#endif
}
#endif

#ifdef VERTEX
#if USE_INTERPOLATOR
void setLutIntCoord(const in vec2 vCoord) {
	lutIntCoord = vCoord/dxy - vec2(0.5, 0.5);
}
#else
#define setLutIntCoord(a)
#endif
#endif
)";
	return code;
}



template<typename T>
static QImage getImage(const QSize &size, const OpenGLTextureFormat &format) {
	if (size.isEmpty())
		return QImage();
	QImage image(size, QImage::Format_ARGB32);
	QVector<T> data(size.width()*size.height()*4);
	auto src = data.data();
	glReadPixels(0, 0, size.width(), size.height(), format.pixel, format.type, src);
	uchar *dst = image.bits();
	const qreal r = qreal(_Max<uchar>())/qreal(_Max<T>());
	for (int i=0; i<size.width()*size.height()*4; ++i)
		*dst++ = qRound(qreal(*src++)*r);
	return image;
}

QImage OpenGLFramebufferObject::toImage() const {
	bind();
	Q_ASSERT(QOpenGLContext::currentContext() != nullptr);
	switch (m_texture.format.type) {
	case GL_UNSIGNED_BYTE:
	case GL_UNSIGNED_INT_8_8_8_8:
	case GL_UNSIGNED_INT_8_8_8_8_REV:
		return getImage<uchar>(m_texture.size(), m_texture.format);
	case GL_UNSIGNED_SHORT:
		return getImage<GLushort>(m_texture.size(), m_texture.format);
	default:
		return QImage();
	}
	release();
}
