#include "openglcompat.hpp"
#include "colorproperty.hpp"
#include <cmath>

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
	m_formats[GL_RED] = {GL_R8, GL_RED, GL_UNSIGNED_BYTE};
	m_formats[GL_RG] = {GL_RG8, GL_RG, GL_UNSIGNED_BYTE};
	m_formats[GL_LUMINANCE] = {GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE};
	m_formats[GL_LUMINANCE_ALPHA] = {GL_LUMINANCE8_ALPHA8, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE};
	m_formats[GL_RGB] = {GL_RGB8, GL_RGB, GL_UNSIGNED_BYTE};
	m_formats[GL_BGR] = {GL_RGB8, GL_BGR, GL_UNSIGNED_BYTE};
	m_formats[GL_BGRA] = {GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV};
	m_formats[GL_RGBA] = {GL_RGBA8, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV};
	if (m_hasRG) {
		m_formats[1] = m_formats[GL_RED];
		m_formats[2] = m_formats[GL_RG];
	} else {
		qDebug() << "no GL_RG type support";
		m_formats[1] = m_formats[GL_LUMINANCE];
		m_formats[2] = m_formats[GL_LUMINANCE_ALPHA];
	}
	m_formats[3] = m_formats[GL_BGR];
	m_formats[4] = m_formats[GL_BGRA];

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &m_maxTextureSize);

	m_bicubicParams[(int)InterpolatorType::BicubicBS] = {1.0, 0.0};
	m_bicubicParams[(int)InterpolatorType::BicubicCR] = {0.0, 0.5};
	m_bicubicParams[(int)InterpolatorType::BicubicMN] = {1./3., 1./3.};
	m_bicubicParams[(int)InterpolatorType::Lanczos2] = {2.0, 2.0};
	m_bicubicParams[(int)InterpolatorType::Lanczos3Fast] = {3.0, 3.0};
}

static auto conv = [] (double d) { return static_cast<GLushort>(d * std::numeric_limits<GLushort>::max()); };

template<typename Func>
QVector<GLushort> makeInterpolatorLut4(Func func) {
	QVector<GLushort> lut(OpenGLCompat::CubicLutSize);
	auto p = lut.data();
	for (int i=0; i<OpenGLCompat::CubicLutSamples; ++i) {
		const auto a = (double)i/(OpenGLCompat::CubicLutSamples-1);
		const auto w0 = func(a + 1.0) + func(a + 2.0);
		const auto w1 = func(a + 0.0);
		const auto w2 = func(a - 1.0);
		const auto w3 = func(a - 2.0) + func(a - 3.0);
		const auto g0 = w0 + w1;		const auto g1 = w2 + w3;
		const auto h0 = 1.0 + a - w1/g0;const auto h1 = 1.0 - a + w3/g1;
		const auto f0 = g0 + g1;		const auto f1 = g1/f0;
		*p++ = conv(h0);		*p++ = conv(h1);
		*p++ = conv(f0);		*p++ = conv(f1);
	}
	return lut;
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

void OpenGLCompat::fillInterpolatorLut(InterpolatorType interpolator) {
	const int type = (int)interpolator;
	const double b = m_bicubicParams[type].first;
	const double c = m_bicubicParams[type].second;
	switch (interpolator) {
	case InterpolatorType::BicubicBS:
	case InterpolatorType::BicubicCR:
	case InterpolatorType::BicubicMN:
		m_intLuts[type] = makeInterpolatorLut4([b, c] (double x) { return bicubic(x, b, c); });
		break;
	case InterpolatorType::Lanczos2:
	case InterpolatorType::Lanczos3Fast:
		m_intLuts[type] = makeInterpolatorLut4([b] (double x) { return lanczos(x, b); });
		break;
	default:
		break;
	}
}

OpenGLTexture OpenGLCompat::allocateInterpolatorLutTexture(GLuint id, InterpolatorType interpolator) {
	OpenGLTexture texture;
	texture.id = id;
	if (interpolator == InterpolatorType::Bilinear)
		return texture;
	auto &lut = c.m_intLuts[(int)interpolator];
	if (lut.isEmpty())
		c.fillInterpolatorLut(interpolator);
	Q_ASSERT(!lut.isEmpty());
	texture.target = GL_TEXTURE_1D;
	texture.width = CubicLutSamples;
	texture.height = 0;
	texture.format.internal = GL_RGBA16;
	texture.format.pixel = GL_BGRA;
	texture.format.type = GL_UNSIGNED_SHORT;
	texture.bind();
	texture.allocate(GL_LINEAR, GL_REPEAT, lut.constData());
	return texture;
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

void OpenGLCompat::upload3dLutTexture(const OpenGLTexture &texture, const QVector3D &sub, const QMatrix3x3 &mul, const QVector3D &add) {
	constexpr static const int max = std::numeric_limits<GLushort>::max();
	const int length = texture.width*texture.height*texture.depth*4;
	if (length != c.m_3dLut.size() || c.m_subLut != sub || c.m_addLut != add || c.m_mulLut != mul) {
		c.m_subLut = sub;
		c.m_addLut = add;
		c.m_mulLut = mul;
		c.m_3dLut.resize(length);
		auto p = c.m_3dLut.data();
		auto conv = [] (float v) { v = qBound(0.f, v, 1.f); return (GLushort)(v*max); };
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
					*p++ = max;
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
