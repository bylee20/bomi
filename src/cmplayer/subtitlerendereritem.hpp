#ifndef SUBTITLERENDERERITEM_HPP
#define SUBTITLERENDERERITEM_HPP

#include "stdafx.hpp"
#include "texturerendereritem.hpp"
#include "subtitle.hpp"

class RichTextDocument;		class SubtitleComponentModel;
struct SubtitleStyle;		class Margin;

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
	void setLetterboxHint(bool hint) { if (_Change(m_letterbox, hint) && m_screen != boundingRect()) { prepare(); update(); } }
	void setDelay(int delay) { if (_Change(m_delay, delay)) render(m_ms); }
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
	void setScreenRect(const QRectF &screen) { if (_Change(m_screen, screen) && !m_letterbox) { setGeometryDirty(); prepare(); update(); } }
	void setHidden(bool hidden) { if (_Change(m_hidden, hidden)) rerender(); }
	void render(int ms);
	void setTopAlignment(bool top);
	void setFps(double fps) { if (_Change(m_fps, fps)) resetIterators(); }
signals:
	void modelsChanged(const QVector<SubtitleComponentModel*> &models);
private:
	QSizeF contentSize() const {return m_size;}
	void setText(const RichTextDocument &doc);
	void setMargin(double top, double bottom, double right, double left);
	struct Render {
		Render(const SubtitleComponent &comp); ~Render();
		const SubtitleComponent *comp = nullptr;
		SubtitleComponent::const_iterator prev;
		SubtitleComponentModel *model = nullptr;
	};
	Render *take(int loadedIndex);
	QRectF drawArea() const { return m_letterbox ? boundingRect() : m_screen; }
	void resetIterators() { for (auto render : m_order) render->prev = render->comp->end(); }
	bool hasComponents() const { for (auto render : m_order) {if (!render->comp->isEmpty()) return true;} return false; }
	void rerender();
	void applySelection() { m_compempty = !hasComponents(); rerender(); emit modelsChanged(models()); }
	void updateStyle();
	void updateAlignment();
	bool blending() const override {return true;}
	void prepare();
	void clear();
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
	QList<Render*> m_order;
};

#endif // SUBTITLERENDERERITEM_HPP
