#ifndef FRAGMENTPROGRAM_HPP
#define FRAGMENTPROGRAM_HPP

#include <QtOpenGL/QGLWidget>

class FragmentProgram {
public:
	FragmentProgram(const QByteArray &code);
	~FragmentProgram();
	QByteArray code() const;
	bool isAvailable() const;
	bool hasError() const;
	void bind();
	void release();
	void setLocalParam(int row, float v1, float v2 = 0.0f, float v3 = 0.0f, float v4 = 0.0f);
private:
	typedef void (*_glProgramStringARB) (GLenum, GLenum, GLsizei, const GLvoid *);
	typedef void (*_glBindProgramARB) (GLenum, GLuint);
	typedef void (*_glDeleteProgramsARB) (GLsizei, const GLuint *);
	typedef void (*_glGenProgramsARB) (GLsizei, GLuint *);
	typedef void (*_glProgramLocalParameter4fARB) (GLenum, GLuint, GLfloat, GLfloat, GLfloat, GLfloat);
	_glProgramStringARB glProgramStringARB;
	_glBindProgramARB glBindProgramARB;
	_glDeleteProgramsARB glDeleteProgramsARB;
	_glGenProgramsARB glGenProgramsARB;
	_glProgramLocalParameter4fARB glProgramLocalParameter4fARB;
	struct Data;
	Data *d;
};

#endif // FRAGMENTPROGRAM_HPP
