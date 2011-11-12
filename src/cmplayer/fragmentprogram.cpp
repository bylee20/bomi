#include "fragmentprogram.hpp"
#include <QtCore/QDebug>

struct FragmentProgram::Data {
	QByteArray code;
	GLuint id;
	bool available;
	bool hasError;
};

FragmentProgram::FragmentProgram(const QByteArray &code)
: d(new Data) {
	d->code = code;
	d->hasError = true;
	d->id = 0;
	const QGLContext *ctx = QGLContext::currentContext();
	Q_ASSERT(ctx != 0);
#define GET_PROC_ADDRESS(func) func = (_##func)(ctx->getProcAddress(QLatin1String(#func)))
	GET_PROC_ADDRESS(glProgramStringARB);
	GET_PROC_ADDRESS(glBindProgramARB);
	GET_PROC_ADDRESS(glDeleteProgramsARB);
	GET_PROC_ADDRESS(glGenProgramsARB);
	GET_PROC_ADDRESS(glProgramLocalParameter4fARB);
#undef GET_PROC_ADDRESS
	d->available = glProgramStringARB && glBindProgramARB
		&& glDeleteProgramsARB && glGenProgramsARB && glProgramLocalParameter4fARB;
	if (!d->available) {
		qDebug() << "fragment programs aren't available!";
		return;
	}
	glGenProgramsARB(1, &d->id);
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, d->id);
	const char *data = d->code.constData();
	glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB, qstrlen(data), data);
	d->hasError = (glGetError() != GL_NO_ERROR);
	if (d->hasError) {
		glDeleteProgramsARB(1, &d->id);
		qDebug() << "fragmane program can't be compiled!";
	}
}

FragmentProgram::~FragmentProgram() {
	if (!d->hasError)
		glDeleteProgramsARB(1, &d->id);
	delete d;
}

bool FragmentProgram::isAvailable() const {
	return d->available;
}

bool FragmentProgram::hasError() const {
	return d->hasError;
}

void FragmentProgram::bind() {
	glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, d->id);
	glEnable(GL_FRAGMENT_PROGRAM_ARB);
}

void FragmentProgram::release() {
	glDisable(GL_FRAGMENT_PROGRAM_ARB);
}

void FragmentProgram::setLocalParam(int row, float v1, float v2, float v3, float v4) {
	glProgramLocalParameter4fARB(GL_FRAGMENT_PROGRAM_ARB, row, v1, v2, v3, v4);
}

QByteArray FragmentProgram::code() const {
	return d->code;
}
