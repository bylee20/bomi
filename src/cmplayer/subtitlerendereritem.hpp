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
	QColor m_color = {0, 0, 0, 255/2};
	QPointF m_offset = {0.2, 0.2};
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
	double m_width = 0.1;
	bool m_enabled = true;
};

class SubtitleSpacingObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(double line READ line WRITE setLine)
	Q_PROPERTY(double paragraph READ paragraph WRITE setParagraph)
public:
	SubtitleSpacingObject(QObject *parent): QObject(parent) {}
	double line() const {return m_line;}
	double paragraph() const {return m_para;}
	void setLine(double line) {m_line = line;}
	void setParagraph(double para) {m_para = para;}
private:
	double m_line = 0, m_para = 0;
};

class SubtitleFontObject : public QObject {
	Q_OBJECT
public:
	Q_ENUMS(Scale)
	enum Scale {Width, Height, Diagonal};
private:
	Q_PROPERTY(QString family READ family WRITE setFamily)
	Q_PROPERTY(bool bold READ bold WRITE setBold)
	Q_PROPERTY(bool italic READ italic WRITE setItalic)
	Q_PROPERTY(bool underline READ underline WRITE setUnderline)
	Q_PROPERTY(bool strikeOut READ strikeOut WRITE setStrikeOut)
	Q_PROPERTY(QColor color READ color WRITE setColor)
	Q_PROPERTY(double size READ size WRITE setSize)
	Q_PROPERTY(Scale scale READ scale WRITE setScale)
public:
	SubtitleFontObject(QObject *parent): QObject(parent) {
		m_font.setPixelSize(height());
	}
	QString family() const {return m_font.family();}
	bool bold() const {return m_font.bold();}
	bool italic() const {return m_font.italic();}
	bool underline() const {return m_font.underline();}
	bool strikeOut() const {return m_font.strikeOut();}
	void setFamily(const QString &family) {m_font.setFamily(family);}
	void setBold(bool bold) {m_font.setBold(bold);}
	void setItalic(bool italic) {m_font.setItalic(italic);}
	void setUnderline(bool underline) {m_font.setUnderline(underline);}
	void setStrikeOut(bool strikeOut) {m_font.setStrikeOut(strikeOut);}
	const QFont &font() const {return m_font;}
	static constexpr int height() {return 20;}
	QColor color() const {return m_color;}
	Scale scale() const {return m_scale;}
	double size() const {return m_size;}
	void setColor(const QColor &color) {m_color = color;}
	void setScale(Scale scale) {m_scale = scale;}
	void setSize(double size) {m_size = size;}
	int weight() const {return m_font.weight();}
private:
	QFont m_font;
	QColor m_color = {Qt::white};
	double m_size = 0.03;
	Scale m_scale = Width;
};

class SubtitleStyleObject : public QObject {
	Q_OBJECT
	Q_PROPERTY(SubtitleShadowObject *shadow READ shadow)
	Q_PROPERTY(SubtitleOutlineObject *outline READ outline)
	Q_PROPERTY(SubtitleFontObject *font READ font)
	Q_PROPERTY(SubtitleSpacingObject *spacing READ spacing)
public:
	SubtitleStyleObject(QObject *parent): QObject(parent) {}
	SubtitleShadowObject *shadow() const {return m_shadow;}
	SubtitleOutlineObject *outline() const {return m_outline;}
	SubtitleFontObject *font() const {return m_font;}
	SubtitleSpacingObject *spacing() const {return m_spacing;}
	QTextOption::WrapMode wrap_mode = QTextOption::WrapAtWordBoundaryOrAnywhere;
private:
	SubtitleShadowObject *m_shadow = new SubtitleShadowObject(this);
	SubtitleOutlineObject *m_outline = new SubtitleOutlineObject(this);
	SubtitleFontObject *m_font = new SubtitleFontObject(this);
	SubtitleSpacingObject *m_spacing = new SubtitleSpacingObject(this);
//	void save(Record &r, const QString &group) const;
//	void load(Record &r, const QString &group);
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
	void setScreenRect(const QRectF &screen);
	void setAlignment(Qt::Alignment alignment);
	double fps() const;
	void setLetterboxHint(bool hint);
	bool letterboxHint() const {return m_letterbox;}
	Qt::Alignment alignment() const {return m_alignment;}
	void setMargin(double top, double bottom, double right, double left);
signals:
	void testChanged();
private:
	void updateStyle();
	void updateAlignment();
	bool blending() const override {return true;}
	void prepare();
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	QRectF drawArea() const;
	double scale() const;
	void link(QOpenGLShaderProgram *program);
	void bind(const RenderState &state, QOpenGLShaderProgram *program);
	void updateTexturedPoint2D(TexturedPoint2D *tp);
	void beforeUpdate();
	void initializeTextures();
	const char *fragmentShader() const;
	struct Data;
	Data *d;
	SubtitleStyleObject *m_style = new SubtitleStyleObject(this);
	QSizeF m_size;
	bool m_letterbox = true;
	Qt::Alignment m_alignment = Qt::AlignBottom | Qt::AlignHCenter;
};

#endif // SUBTITLERENDERERITEM_HPP
