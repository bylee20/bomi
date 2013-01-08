//#ifndef OVERLAY_HPP
//#define OVERLAY_HPP

//#include "stdafx.hpp"

//class QGLWidget;

//class OsdRenderer;	class QGLShaderProgram;

//class OsdWrapper {
//public:
//	virtual ~OsdWrapper() {free();}
//	virtual void free() {
//		if (!m_texture.isEmpty()) {
//			glDeleteTextures(m_texture.size(), m_texture.data());
//			m_texture.clear();
//		}
//		m_sub_x.clear();		m_sub_y.clear();
//		m_size.clear();			m_texSize.clear();
//		m_empty = true;
//	}
//	virtual void alloc() {
//		const int count = _count();
//		free();
//		m_texture.resize(count);	glGenTextures(count, m_texture.data());
//		m_sub_x.resize(count);		m_sub_x.fill(1.f);
//		m_sub_y = m_sub_x;
//		m_size.resize(count);		m_size.fill(QSize(0, 0));
//		m_texSize = m_size;
//	}
//	GLuint texture(int i) const {return m_texture[i];}
//	float dx(int i) const {return 1.0f/m_texSize[i].width();}
//	float dy(int i) const {return 1.0f/m_texSize[i].height();}
//	float sub_x(int i) const {return m_sub_x[i];}
//	float sub_y(int i) const {return m_sub_y[i];}
//	QSize size(int i) const {return m_size[i];}
//	int width(int i) const {return m_size[i].width();}
//	int height(int i) const {return m_size[i].height();}
//	void upload(const QSize &size, const GLvoid *data, int i, bool alloc = false) {
//		glBindTexture(GL_TEXTURE_2D, m_texture[i]);
//		if (alloc || size.width() > m_texSize[i].width() || size.height() > m_texSize[i].height()) {
//			glTexImage2D(GL_TEXTURE_2D, 0, _internalFormat(i), size.width(), size.height(), 0, _format(i), _type(i), data);
//			m_texSize[i] = size;
//			m_sub_x[i] = m_sub_y[i] = 1.0f;
//		} else {
//			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.width(), size.height(), _format(i), _type(i), data);
//			m_sub_x[i] = (double)qMax(0, size.width()-1)/m_texSize[i].width();
//			m_sub_y[i] = (double)size.height()/m_texSize[i].height();
//		}
//		m_size[i] = size;
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
//		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
//	}
//	bool isEmpty() const {return m_empty;}
//	int count() const {return m_texture.size();}
//protected:
//	virtual GLint _internalFormat(int idx) const = 0;
//	virtual GLenum _format(int idx) const = 0;
//	virtual GLenum _type(int idx) const = 0;
//	virtual int _count() const = 0;
//	bool m_empty = true;
//private:
//	QVector<GLuint> m_texture;
//	QVector<QSize> m_size, m_texSize;
//	QVector<float> m_sub_x, m_sub_y;
//};

//class VideoScreen;

//class Overlay: public QObject, public QGLFunctions {
//	Q_OBJECT
//public:
//	Overlay(VideoScreen *screen);
//	~Overlay();
//	QGLWidget *screen() const;
//	void setArea(const QRect &renderable, const QRect &frame);
//	void add(OsdRenderer *osd);
//	void render(QPainter *painter);
//	// call from screen rendering only
//	void renderToScreen();
//	OsdRenderer *take(OsdRenderer *osd);
//private slots:
//	void update();
//	void onOsdDeleted();
//private:
//	struct Data;
//	Data *d;
//};



//#endif // OVERLAY_HPP
