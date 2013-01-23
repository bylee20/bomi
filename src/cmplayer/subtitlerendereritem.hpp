#ifndef SUBTITLERENDERERITEM_HPP
#define SUBTITLERENDERERITEM_HPP

#include "stdafx.hpp"
#include "texturerendereritem.hpp"
#include "subtitlestyle.h"
#include "subtitle.hpp"

class RichTextDocument;		class SubtitleComponentModel;

class LoadedSubtitle {
public:
	LoadedSubtitle(): m_selected(false) {}
	LoadedSubtitle(const SubtitleComponent &comp): m_selected(false), m_comp(comp) {}
	bool isSelected() const {return m_selected;}
	QString name() const {return m_comp.name();}
	SubtitleComponent &component() { return m_comp; }
	const SubtitleComponent &component() const { return m_comp; }
	bool &selection() { return m_selected; }
private:
	bool m_selected;
	SubtitleComponent m_comp;
};

class SubtitleRendererItem : public TextureRendererItem  {
	Q_OBJECT
	Q_PROPERTY(SubtitleStyleObject *style READ style)
public:
	SubtitleRendererItem(QQuickItem *parent = nullptr);
	~SubtitleRendererItem();
	void setLetterboxHint(bool hint);
	bool letterboxHint() const {return m_letterbox;}
	double fps() const { return m_fps; }
	int delay() const { return m_delay; }
	void setDelay(int delay) { if (_Change(m_delay, delay)) render(m_ms); }
	const QList<LoadedSubtitle> &loaded() const { return m_loaded; }
	void setPos(double pos) {
		if (_ChangeF(m_pos, qBound(0.0, pos, 1.0)))
			setMargin(m_alignTop ? m_pos : 0, m_alignTop ? 0.0 : 1.0 - m_pos, 0, 0);
	}
	double pos() const { return m_pos; }
	bool isTopAligned() const { return m_alignTop; }
	bool load(const QString &fileName, const QString &enc, bool select);

	void setLoaded(const QList<LoadedSubtitle> &loaded);
	int previous() const;
	int next() const;
	int current() const;
	int start(int pos) const;
	int end(int pos) const;
	void unload();
	void select(int idx, bool selected = true);

	bool hasSubtitle() const { return !m_compempty; }
public slots:
	void setScreenRect(const QRectF &screen);
	void setHidden(bool hidden) { if (_Change(m_visible, !hidden)) rerender(); }
	void render(int ms);
	void setTopAlignment(bool top);
private:
	SubtitleStyleObject *style() const {return m_style;}
	QSizeF contentSize() const {return m_size;}
	void setText(const RichTextDocument &doc);
	void setAlignment(Qt::Alignment alignment);
	Qt::Alignment alignment() const {return m_alignment;}
	void setMargin(double top, double bottom, double right, double left);
private:
	void rerender() { if (m_compempty) clear(); else render(m_ms); }
	void applySelection();

	void clear();
	void setFps(double fps);
	QWidget *view(QWidget *parent = 0) const;
	void select(const QList<int> &idx, bool selected = true);
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





	double	m_fps = 25.0,			m_pos = 1.0;
	int		m_delay = 0,			m_ms = 0;
	bool	m_visible = true,		m_compempty = true;
	bool	m_alignTop = false;
	QList<LoadedSubtitle> m_loaded;
};

#endif // SUBTITLERENDERERITEM_HPP
