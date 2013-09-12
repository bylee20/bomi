#include "openglcompat.hpp"

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
}
