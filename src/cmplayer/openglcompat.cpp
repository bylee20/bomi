#include "openglcompat.hpp"
#include "colorproperty.hpp"

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

	m_bicubicParams[(int)InterpolatorType::Bilinear] = {0.0, 0.0};
	m_bicubicParams[(int)InterpolatorType::BicubicBS] = {1.0, 0.0};
	m_bicubicParams[(int)InterpolatorType::BicubicCR] = {0.0, 0.5};
	m_bicubicParams[(int)InterpolatorType::BicubicMN] = {1./3., 1./3.};
}

QVector<GLushort> OpenGLCompat::makeBicubicLut(double b, double c) {
	static auto weight = [b, c] (double x) {
		x = qAbs(x);
		Q_ASSERT(x <= 2.0);
		if (x < 1.0)
			return ((12.0 - 9.0*b - 6.0*c)*x*x*x + (-18.0 + 12.0*b + 6.0*c)*x*x + (6.0 - 2.0*b))/6.0;
		if (x < 2.0)
			return ((-b - 6.0*c)*x*x*x + (6.0*b + 30.0*c)*x*x + (-12.0*b - 48.0*c)*x + (8.0*b + 24.0*c))/6.0;
		return 0.0;
	};
	auto conv = [] (double d) { return static_cast<GLushort>(d * std::numeric_limits<GLushort>::max()); };
	QVector<GLushort> lut(CubicLutSize);
	auto p = lut.data();
	for (int i=0; i<CubicLutSamples; ++i) {
		const auto a = (double)i/(CubicLutSamples-1);
		const auto w0 = weight(a+1.0);	const auto w1 = weight(a);
		const auto w2 = weight(a-1.0);	const auto w3 = weight(a-2.0);
		const auto g0 = w0 + w1;		const auto g1 = w2 + w3;
		const auto h0 = 1.0 + a - w1/g0;const auto h1 = 1.0 - a + w3/g1;
		const auto f0 = g0 + g1;		const auto f1 = g1/f0;
		*p++ = conv(h0);		*p++ = conv(h1);
		*p++ = conv(f0);		*p++ = conv(f1);
	}
	return lut;
}

OpenGLTexture OpenGLCompat::allocateBicubicLutTexture(GLuint id, InterpolatorType interpolator) {
	const int type = (int)interpolator;
	OpenGLTexture texture;
	if (type == InterpolatorType::Bilinear)
		return texture;
	if (c.m_bicubicLuts[type].isEmpty())
		c.m_bicubicLuts[type] = makeBicubicLut(c.m_bicubicParams[type].first, c.m_bicubicParams[type].second);
	texture.target = GL_TEXTURE_1D;
	texture.width = CubicLutSamples;
	texture.height = 0;
	texture.format.internal = GL_RGBA16;
	texture.format.pixel = GL_BGRA;
	texture.format.type = GL_UNSIGNED_SHORT;
	texture.id = id;
	texture.bind();
	texture.allocate(GL_LINEAR, GL_REPEAT, c.m_bicubicLuts[type].constData());
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
