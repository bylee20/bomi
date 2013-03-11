#ifndef SUBTITLERENDERERITEM_HPP
#define SUBTITLERENDERERITEM_HPP

#include "stdafx.hpp"
#include "texturerendereritem.hpp"
#include "subtitle.hpp"

class RichTextDocument;		class SubtitleComponentModel;
struct SubtitleStyle;		struct Margin;

class LoadedSubtitle {
public:
	LoadedSubtitle() {}
	LoadedSubtitle(const SubtitleComponent &comp): m_comp(comp) {}
	bool isSelected() const {return m_selected;}
	QString name() const {return m_comp.name();}
	SubtitleComponent &component() { return m_comp; }
	const SubtitleComponent &component() const { return m_comp; }
	bool &selection() { return m_selected; }
private:
	bool m_selected = false;
	SubtitleComponent m_comp;
};

class SubtitleRendererItem : public TextureRendererItem  {
	Q_OBJECT
public:
	SubtitleRendererItem(QQuickItem *parent = nullptr);
	~SubtitleRendererItem();
	int previous() const;
	int next() const;
	int current() const;
	int start(int pos) const;
	int finish(int pos) const;
	int delay() const { return m_delay; }
	double fps() const { return m_fps; }
	double pos() const { return m_pos; }
	bool letterboxHint() const {return m_letterbox;}
	bool hasSubtitles() const { return !m_compempty; }
	bool isTopAligned() const { return m_top; }
	QVector<SubtitleComponentModel*> models() const;
	const QList<LoadedSubtitle> &loaded() const { return m_loaded; }
	void setLoaded(const QList<LoadedSubtitle> &loaded);
	void setPriority(const QStringList &priority);
	void setPos(double pos) { if (_ChangeF(m_pos, qBound(0.0, pos, 1.0))) setMargin(m_top ? m_pos : 0, m_top ? 0.0 : 1.0 - m_pos, 0, 0); }
	void setLetterboxHint(bool hint);
	void setDelay(int delay) { if (_Change(m_delay, delay)) rerender(); }
	bool load(const QString &fileName, const QString &enc, bool select);
	void unload();
	void select(int idx);
	void deselect(int idx = -1);
	const SubtitleStyle &style() const;
	void setStyle(const SubtitleStyle &style);
	double scale(const QRectF &area) const;
	const Margin &margin() const;
	const RichTextDocument &text() const;
	Qt::Alignment alignment() const {return m_alignment;}
	double dpr() const {
		if (!window() || !window()->screen())
			return 1.0;
		const auto screen = window()->screen();
		return qMax(screen->logicalDotsPerInch()/screen->physicalDotsPerInch(), 1.0);
	}
public slots:
	void setScreenRect(const QRectF &screen);
	void setHidden(bool hidden);
	void render(int ms);
	void setTopAlignment(bool top);
	void setFps(double fps) { if (_Change(m_fps, fps)) rerender(); }
signals:
	void modelsChanged(const QVector<SubtitleComponentModel*> &models);
private slots:

private:
	void rerender();
	const SubtitleComponent *take(int loadedIndex);
	QSizeF contentSize() const {return m_size;}
	void setMargin(double top, double bottom, double right, double left);
	QRectF drawArea() const { return m_letterbox ? boundingRect() : m_screen; }
	void applySelection();
	void updateStyle();
	void updateAlignment();
	bool blending() const override {return true;}
	void clear();
	void customEvent(QEvent *event);
	void geometryChanged(const QRectF &newGeometry, const QRectF &oldGeometry);
	void link(QOpenGLShaderProgram *program);
	void bind(const RenderState &state, QOpenGLShaderProgram *program);
	void updateTexturedPoint2D(TexturedPoint2D *tp);
	void beforeUpdate();
	void initializeTextures();
	const char *fragmentShader() const;
	struct Data;
	Data *d;
	QSizeF m_size;	QRectF m_screen = {0, 0, 0, 0};
	bool m_letterbox = true, m_compempty = true, m_top = false, m_hidden = false;
	Qt::Alignment m_alignment = Qt::AlignBottom | Qt::AlignHCenter;
	double m_fps = 25.0, m_pos = 1.0;
	int m_delay = 0, m_ms = 0;
	QList<LoadedSubtitle> m_loaded;
};

#endif // SUBTITLERENDERERITEM_HPP
