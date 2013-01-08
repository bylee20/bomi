#ifndef SUBTITLERENDERERITEM_HPP
#define SUBTITLERENDERERITEM_HPP

#include "stdafx.hpp"
#include "texturerendereritem.hpp"

class SubtitleShadowObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
	Q_PROPERTY(QColor color READ color WRITE setColor)
	Q_PROPERTY(bool blur READ blur WRITE setBlur)
	Q_PROPERTY(QPointF offset READ offset WRITE setOffset)
public:
	SubtitleShadowObject(QObject *parent): QObject(parent) {}
	QColor color() const {return m_color;}
	bool isEnabled() const {return m_enabled;}
	bool blur() const {return m_blur;}
	QPointF offset() const {return m_offset;}
	void setOffset(const QPointF &offset) {m_offset = offset;}
	void setBlur(bool blur) {m_blur = blur;}
	void setColor(const QColor &color) {m_color = color;}
	void setEnabled(bool enabled) {m_enabled = enabled;}
private:
	bool m_enabled = true, m_blur = false;
	QColor m_color = {Qt::black};
	QPointF m_offset = {0.0, 0.0};
};

class SubtitleOutlineObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(QColor color READ color WRITE setColor)
	Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled)
	Q_PROPERTY(double width READ width WRITE setWidth)
public:
	SubtitleOutlineObject(QObject *parent): QObject(parent) {}
	QColor color() const {return m_color;}
	bool isEnabled() const {return m_enabled;}
	double width() const {return m_width;}
	void setColor(const QColor &color) {m_color = color;}
	void setEnabled(bool enabled) {m_enabled = enabled;}
	void setWidth(double width) {m_width = width;}
private:
	QColor m_color = {Qt::black};
	double m_width = 0.001;
	bool m_enabled = true;
};

class SubtitleStyleObject : public QObject {
	Q_OBJECT
	Q_ENUMS(Scale)
	Q_PROPERTY(SubtitleShadowObject *shadow READ shadow)
	Q_PROPERTY(SubtitleOutlineObject *outline READ outline)
	Q_PROPERTY(QColor color READ color WRITE setColor)
	Q_PROPERTY(double size READ size WRITE setSize)
public:
	enum Scale {Width, Height, Diagonal};
	SubtitleStyleObject(QObject *parent): QObject(parent) {}
	SubtitleShadowObject *shadow() const {return m_shadow;}
	SubtitleOutlineObject *outline() const {return m_outline;}
	QColor color() const {return m_color;}
	Scale scale() const {return m_scale;}
	double size() const {return m_size;}
	void setColor(const QColor &color) {m_color = color;}
	void setScale(Scale scale) {m_scale = scale;}
	void setSize(double size) {m_size = size;}

	QFont font = {};
	QTextOption::WrapMode wrap_mode = QTextOption::WrapAtWordBoundaryOrAnywhere;
	double line_spacing = 0.0;
	double paragraph_spacing = 0.0;
private:
	SubtitleShadowObject *m_shadow = new SubtitleShadowObject(this);
	SubtitleOutlineObject *m_outline = new SubtitleOutlineObject(this);
//	void save(Record &r, const QString &group) const;
//	void load(Record &r, const QString &group);
	QColor m_color = {Qt::white};
	double m_size = 0.03;
	Scale m_scale = Width;
	// text

};

class RichTextDocument;

class SubtitleRendererItem : public TextureRendererItem  {
	Q_OBJECT
	Q_PROPERTY(SubtitleStyleObject *style READ style)
public:
	SubtitleRendererItem(QQuickItem *parent = nullptr);
	~SubtitleRendererItem();
	SubtitleStyleObject *style() const {return m_style;}
	QSizeF contentSize() const {return m_size;}
	void setText(const RichTextDocument &doc);
	bool blending() const override {return true;}
//	TextOsdRenderer(Qt::Alignment align = Qt::AlignTop | Qt::AlignHCenter);
//	~TextOsdRenderer();
//	void show(const QString &text, int last = 2500);
//	void show(const RichTextDocument &doc, int last = 2500);
//	void prepareToRender(const QPointF &);
//	void render(QPainter *painter, const QPointF &pos, int layer);
//	QPointF posHint() const;
//	QSizeF size() const;
	void setAlignment(Qt::Alignment alignment);
//	RichTextDocument doc() const;
	Qt::Alignment alignment() const {return m_alignment;}

//	void setMargin(double top, double bottom, double right, double left);
//	int layers() const {return 2;}
//public slots:
//	void clear();
//private:
	void prepare();
//	bool updateRenderableSize(const QSizeF &renderable);
//	void updateStyle(const OsdStyle &);
//	void updateFont();
//	struct Data;
//	Data *d;


	void link(QOpenGLShaderProgram *program);
	void bind(const RenderState &state, QOpenGLShaderProgram *program);
	void updateTexturedPoint2D(TexturedPoint2D *tp);
	bool beforeUpdate() override;


private:
	const char *fragmentShader() const;
	struct Data;
	Data *d;
	SubtitleStyleObject *m_style = new SubtitleStyleObject(this);
	QSizeF m_size;
	Qt::Alignment m_alignment = Qt::AlignBottom | Qt::AlignHCenter;
};

#endif // SUBTITLERENDERERITEM_HPP
