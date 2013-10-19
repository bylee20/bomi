#include "openglcompat.hpp"
#include "colorproperty.hpp"
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
	m_bicubicParams[(int)InterpolatorType::Lanczos2] = {2.0, 2.0};
	m_bicubicParams[(int)InterpolatorType::Lanczos3Approx] = {3.0, 3.0};
}

template<typename T>
QVector<T> convertToIntegerVector(const QVector<GLfloat> &v, float &mul) {
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

double fract(double v1, double v2) {
	static constexpr auto e = std::numeric_limits<float>::epsilon();
	if (qAbs(v1) < e)
		return qAbs(v2) < e ? 1.0 : 0.0;
	return v1/v2;
}

template<typename Func>
void makeInterpolatorLut4(QVector<GLfloat> &lut, Func func) {
	lut.resize(OpenGLCompat::IntLutSize);
	auto p = lut.data();

	for (int i=0; i<OpenGLCompat::IntSamples; ++i) {
		const auto a = (double)i/(OpenGLCompat::IntSamples-1);
		const auto w0 = func(a + 1.0) + func(a + 2.0);
		const auto w1 = func(a + 0.0);
		const auto w2 = func(a - 1.0);
		const auto w3 = func(a - 2.0) + func(a - 3.0);
		const auto g0 = w0 + w1;
		const auto g1 = w2 + w3;
		const auto h0 = 1.0 + a - fract(w1, g0);
		const auto h1 = 1.0 - a + fract(w3, g1);
		const auto f0 = g0 + g1;
		const auto f1 = fract(g1, f0);
		*p++ = h0;		*p++ = h1;
		*p++ = f0;		*p++ = f1;
	}
}

template<typename Func>
void makeInterpolatorLut6(QVector<GLfloat> &lut1, QVector<GLfloat> &lut2, Func func) {
	lut1.resize(OpenGLCompat::IntLutSize);
	lut2.resize(OpenGLCompat::IntLutSize);
	auto p1 = lut1.data();
	auto p2 = lut2.data();
	for (int i=0; i<OpenGLCompat::IntSamples; ++i) {
		const auto a = (double)i/(OpenGLCompat::IntSamples-1);
		const auto w0 = func(a + 2.0);
		const auto w1 = func(a + 1.0);
		const auto w2 = func(a + 0.0);
		const auto w3 = func(a - 1.0);
		const auto w4 = func(a - 2.0);
		const auto w5 = func(a - 3.0);
		const auto g0 = w0 + w1;
		const auto g1 = w2 + w3;
		const auto g2 = w4 + w5;
		const auto h0 = 2.0 + a - fract(w1, g0);
		const auto h1 = 0.0 + a - fract(w3, g1);
		const auto h2 = 2.0 - a + fract(w5, g2);
		const auto f0 = fract(g1, (g0 + g1));
		const auto f1 = fract(g2, (g0 + g1 + g2));
		*p1++ = h0;		*p1++ = h1;		*p1++ = h2; ++p1;
		*p2++ = f0;		*p2++ = f1;		p2 += 2;
	}
}

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

static double bicubic(double x, double b, double c) {
	x = qAbs(x);
	if (x < 1.0)
		return ((12.0 - 9.0*b - 6.0*c)*x*x*x + (-18.0 + 12.0*b + 6.0*c)*x*x + (6.0 - 2.0*b))/6.0;
	if (x < 2.0)
		return ((-b - 6.0*c)*x*x*x + (6.0*b + 30.0*c)*x*x + (-12.0*b - 48.0*c)*x + (8.0*b + 24.0*c))/6.0;
	return 0.0;
}

static double lanczos(double x, double a) {
	x = qAbs(x);
	if (x == 0.0)
		return 1.0;
	const double pix = M_PI*x;
	if (x < a)
		return a*std::sin(pix)*std::sin(pix/a)/(pix*pix);
	return 0.0;
}

static double spline16(double x) {
	x = qAbs(x);
	if (x < 1.0)
		return ((x-9.0/5.0)*x-1.0/5.0)*x+1.0;
	if (x < 2.0)
		return ((-1.0/3.0*(x-1.0)+4.0/5.0)*(x-1.0)-7.0/15.0)*(x-1.0);
	return 0.0;
}

static double spline36(double x) {
	x = qAbs(x);
	if (x < 1.0)
		return ((13.0/11.0*x-453.0/209.0)*x-3.0/209.0)*x+1.0;
	if (x < 2.0)
		return ((-6.0/11.0*(x-1)+270.0/209.0)*(x-1)-156.0/209.0)*(x-1);
	if (x < 3.0)
		return ((1.0/11.0*(x-2)-45.0/209.0)*(x-2)+26.0/209.0)*(x-2);
	return 0.0;
}

void OpenGLCompat::fillInterpolatorLut(InterpolatorType interpolator) {
	const int type = (int)interpolator;
	const double b = m_bicubicParams[type].first;
	const double c = m_bicubicParams[type].second;
	switch (interpolator) {
	case InterpolatorType::BicubicBS:
	case InterpolatorType::BicubicCR:
	case InterpolatorType::BicubicMN:
		makeInterpolatorLut4(m_intLuts1[type], [b, c] (double x) { return bicubic(x, b, c); });
		break;
	case InterpolatorType::Spline16:
		makeInterpolatorLut4(m_intLuts1[type], spline16);
		break;
	case InterpolatorType::Spline36Approx:
		makeInterpolatorLut4(m_intLuts1[type], spline36);
		break;
	case InterpolatorType::Lanczos2:
	case InterpolatorType::Lanczos3Approx:
		makeInterpolatorLut4(m_intLuts1[type], [b] (double x) { return lanczos(x, b); });
		break;
	case InterpolatorType::Spline36:
		makeInterpolatorLut6(m_intLuts1[type], m_intLuts2[type], spline36);
		break;
	case InterpolatorType::Lanczos3:
		makeInterpolatorLut6(m_intLuts1[type], m_intLuts2[type], [] (double x) { return lanczos(x, 3.0); });
		break;
	default:
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
	texture1.format.internal = GL_RGBA16;
	if (c.m_hasFloat) {
		texture1.format.type = GL_FLOAT;
		texture1.allocate(GL_LINEAR, GL_REPEAT, lut1.data());
		if (!lut2.isEmpty()) {
			texture2.copyAttributesFrom(texture1);
			texture2.allocate(GL_LINEAR, GL_REPEAT, lut2.data());
		}
	} else {
		texture1.format.type = GL_UNSIGNED_SHORT;
		auto data = convertToIntegerVector<GLushort>(lut1, texture1.multiply);
		texture1.allocate(GL_LINEAR, GL_REPEAT, data.data());
		if (!lut2.isEmpty()) {
			texture2.copyAttributesFrom(texture1);
			data = convertToIntegerVector<GLushort>(lut2, texture2.multiply);
			texture2.allocate(GL_LINEAR, GL_REPEAT, data.data());
		}
	}
}

/*	m00 m01 m02  v0
 *  m10 m11 m12  v1
 *  m20 m21 m22  v2
 *   o0  o1  o2   x
 */

QVector3D operator*(const QMatrix3x3 &mat, const QVector3D &vec) {
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
#if USE_INTERPOLATOR == 2
uniform sampler1D lut_int2;
uniform float lut_int2_mul;
vec4 mix3(const in vec4 v1, const in vec4 v2, const in vec4 v3, const in float a, const in float b) {
	return mix(mix(v1, v2, a), v3, b);
}
#endif
#endif

vec4 interpolated(const in sampler2D tex, const in vec2 coord) {
#if USE_INTERPOLATOR == 0
	return texture2D(tex, coord);
#elif USE_INTERPOLATOR == 1
	// b: h0, g: h1, r: g0+g1, a: g1/(g0+g1)
	vec4 hg_x = renormalize(texture1D(lut_int1, lutIntCoord.x), lut_int1_mul);
	vec4 hg_y = renormalize(texture1D(lut_int1, lutIntCoord.y), lut_int1_mul);

	vec4 tex00 = texture2D(tex, coord + vec2(-hg_x.b, -hg_y.b)*dxy);
	vec4 tex10 = texture2D(tex, coord + vec2( hg_x.g, -hg_y.b)*dxy);
	vec4 tex01 = texture2D(tex, coord + vec2(-hg_x.b,  hg_y.g)*dxy);
	vec4 tex11 = texture2D(tex, coord + vec2( hg_x.g,  hg_y.g)*dxy);

	tex00 = mix(tex00, tex10, hg_x.a);
	tex01 = mix(tex01, tex11, hg_x.a);
	return  mix(tex00, tex01, hg_y.a);
#elif USE_INTERPOLATOR == 2
	vec4 h_x = renormalize(texture1D(lut_int1, lutIntCoord.x), lut_int1_mul);
	vec4 h_y = renormalize(texture1D(lut_int1, lutIntCoord.y), lut_int1_mul);
	vec4 f_x = renormalize(texture1D(lut_int2, lutIntCoord.x), lut_int2_mul);
	vec4 f_y = renormalize(texture1D(lut_int2, lutIntCoord.y), lut_int2_mul);

	vec4 tex00 = texture2D(tex, coord + vec2(-h_x.b, -h_y.b)*dxy);
	vec4 tex01 = texture2D(tex, coord + vec2(-h_x.b, -h_y.g)*dxy);
	vec4 tex02 = texture2D(tex, coord + vec2(-h_x.b,  h_y.r)*dxy);
	tex00 = mix3(tex00, tex01, tex02, f_y.b, f_y.g);

	vec4 tex10 = texture2D(tex, coord + vec2(-h_x.g, -h_y.b)*dxy);
	vec4 tex11 = texture2D(tex, coord + vec2(-h_x.g, -h_y.g)*dxy);
	vec4 tex12 = texture2D(tex, coord + vec2(-h_x.g,  h_y.r)*dxy);
	tex10 = mix3(tex10, tex11, tex12, f_y.b, f_y.g);

	vec4 tex20 = texture2D(tex, coord + vec2( h_x.r, -h_y.b)*dxy);
	vec4 tex21 = texture2D(tex, coord + vec2( h_x.r, -h_y.g)*dxy);
	vec4 tex22 = texture2D(tex, coord + vec2( h_x.r,  h_y.r)*dxy);
	tex20 = mix3(tex20, tex21, tex22, f_y.b, f_y.g);
	return mix3(tex00, tex10, tex20, f_x.b, f_x.g);
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
