//#ifndef VIDEORENDERER_HPP
//#define VIDEORENDERER_HPP

//#include "stdafx.hpp"
//#include "mpmessage.hpp"


//class OsdRenderer;			class VideoFormat;
//class ColorProperty;		class Overlay;
//class VideoFrame;			class PlayEngine;
//class VideoScreen;
//namespace Core {class PlayEngine;}

//class VideoRenderer : public QObject, public MpMessage {
//	Q_OBJECT
//public:
//	enum Effect {
//		NoEffect			= 0,
//		FlipVertically		= 1 << 0,
//		FlipHorizontally	= 1 << 1,
//		Grayscale			= 1 << 2,
//		InvertColor			= 1 << 3,
//		Blur				= 1 << 4,
//		Sharpen				= 1 << 5,
//		RemapLuma			= 1 << 6,
//		IgnoreEffect		= 1 << 8
//	};
//	Q_DECLARE_FLAGS(Effects, Effect)
//	static const int FilterEffects = InvertColor | RemapLuma;
//	static const int KernelEffects = Blur | Sharpen;
//public:
//	VideoRenderer(PlayEngine *engine);
//	~VideoRenderer();
//	double frameRate() const;
//	QSize sizeHint() const;
//	bool hasFrame() const;
//	QImage frameImage() const;
//	const VideoFormat &format() const;
//	double aspectRatio() const;
//	double cropRatio() const;
//	void setLogoMode(bool on);
//	void setColorProperty(const ColorProperty &prop);
//	const ColorProperty &colorProperty() const;
//	void setFixedRenderSize(const QSize &size);
//	void clearEffects();
//	void setEffect(Effect effect, bool on);
//	void setEffects(Effects effect);
//	void setInfoVisible(bool visible);
//	QPoint offset() const;
//	int alignment() const;
//	Effects effects() const;
//	void update();
//	StreamList streams() const;
//	int currentStreamId() const;
//	void setCurrentStream(int id);
//	bool needsToBeRendered() const;
//	int outputWidth() const;
//	void takeSnapshot() const;
//	quint64 drawnFrames() const;
//	GLuint *textures() const;
//	GLuint texture(int idx) const;
//	void makeCurrent();
//	void doneCurrent();
//	bool beginUploadingTextures();
//	void endUploadingTextures();
//	VideoScreen &screen() const;
//public slots:
//	void setOffset(const QPoint &offset);
//	void setAlignment(int alignment);
//	void setAspectRatio(double ratio);
//	void setCropRatio(double ratio);
//signals:
//	void prepared();
//	void formatChanged(const VideoFormat &format);
//	void tookSnapshot(QImage image);
//protected:
//	void setScreen(VideoScreen *gl);
//private slots:
//	void onAboutToOpen();
//	void onAboutToPlay();
//private:
//	VideoRenderer(const VideoRenderer&) = delete;
//	VideoRenderer &operator = (const VideoRenderer &) = delete;
//	struct Data;
////	class VideoShader;

//	bool parse(const Id &id);
//	bool parse(const QString &line);
//	void prepare(const VideoFormat &format);
//	VideoFrame &bufferFrame();
//	void uploadTexture(GLuint texture);
//	void render();
//	void updateSize();
//	void onMouseClicked(const QPointF &pos);
//	void onMouseMoved(const QPointF &pos);
//	QSize renderableSize() const;
//	static bool isSameRatio(double r1, double r2) {return (r1 < 0.0 && r2 < 0.0) || qFuzzyCompare(r1, r2);}
//	static void drawAlpha(void *p, int x, int y, int w, int h, uchar *src, uchar *srca, int stride);

//	Data *d;

//	friend class VideoOutput;
//	friend class VideoScreen;
//	friend class PlayEngine;
//	friend class Core::PlayEngine;
//	QGLWidget *gl = nullptr;
//};

//class VideoScreen : public QGLWidget, public QGLFunctions {
//	Q_OBJECT
//public:
//	~VideoScreen();
//	QSize sizeHint() const {return r ? r->sizeHint() : QGLWidget::sizeHint();}
//	double aspectRatio() const {return (double)width()/(double)height();}
//	void redraw() {if (r) r->update();}
//	Overlay *overlay() const {return m_overlay;}
//signals:
//	void sizeChanged(const QSize &size);
//private:
//	VideoScreen(const VideoScreen&) = delete;
//	VideoScreen &operator = (const VideoScreen&) = delete;
//	VideoScreen();
//	bool eventFilter(QObject *o, QEvent *e);
//	void changeEvent(QEvent *event);
//	void paintEvent(QPaintEvent *) {if (r) r->update();}
//	void resizeEvent(QResizeEvent *) {if (r) {r->updateSize(); r->update();} emit sizeChanged(size());}
//	void mouseMoveEvent(QMouseEvent *e) {QGLWidget::mouseMoveEvent(e); if (r) r->onMouseMoved(e->localPos());}
//	void mousePressEvent(QMouseEvent *e) {QGLWidget::mousePressEvent(e); if (r && (e->buttons() & Qt::LeftButton)) r->onMouseClicked(e->localPos());}

//	VideoRenderer *r = nullptr;
//	Overlay *m_overlay;
//	QWidget *m_parent = nullptr;

//	friend class VideoRenderer;
//};

//Q_DECLARE_OPERATORS_FOR_FLAGS(VideoRenderer::Effects)


//#endif // VIDEORENDERER_HPP
