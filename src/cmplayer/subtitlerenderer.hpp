#ifndef SUBTITLERENDERER_HPP
#define SUBTITLERENDERER_HPP

#include "subtitle.hpp"
#include "textosdrenderer.hpp"
#include <QtCore/QObject>
#include <QtCore/QtContainerFwd>

class QDialog;			class Mrl;
class SubtitleComponentModel;

class SubtitleRenderer : public QObject {
	Q_OBJECT
public:
	typedef Subtitle::Component Comp;
	typedef Comp::const_iterator CompIt;

	struct Loaded {
		Loaded(): m_selected(false) {}
		bool isSelected() const {return m_selected;}
		QString name() const {return m_comp.name();}
	private:
		friend class SubtitleRenderer;
		Loaded(const Comp &comp): m_selected(false), m_comp(comp) {}
		bool m_selected;
		Comp m_comp;
	};

	SubtitleRenderer();
	~SubtitleRenderer();
	TextOsdRenderer *osd() const;
	double frameRate() const;
	int delay() const;
	int start(int pos) const;
	int end(int pos) const;
	void setDelay(int delay);
	bool hasSubtitle() const;
	double pos() const;
	int previous() const;
	int next() const;
	int current() const;
	void setPos(double pos);
	QWidget *view(QWidget *parent = 0) const;
	void unload();
	int autoload(const Mrl &mrl, bool autoselect = true);
	void select(int idx, bool selected = true);
	void select(const QList<int> &idx, bool selected = true);
	const QList<Loaded> &loaded() const;
	bool load(const QString &fileName, const QString &enc, bool select);
	bool isTopAligned() const;
public slots:
	void clear();
	void setFrameRate(double frameRate);
	void render(int ms);
	void setVisible(bool visible);
	void setHidden(bool hidden) {setVisible(!hidden);}
	void setTopAlignment(bool top);
private:
	QList<int> autoselection(const Mrl &mrl, const QList<SubtitleRenderer::Loaded> &loaded);
	void applySelection();
	struct Render {
		Render() {comp = 0; model = 0;}
		Render(const Comp &comp);
		~Render();
		const Comp *comp;
		CompIt prev;
		SubtitleComponentModel *model;
	};
	struct Order {
		int lang;
	};

	typedef QLinkedList<Render*> RenderList;
	struct Data;
	Data *d;
};

#endif // SUBTITLERENDERER_HPP
