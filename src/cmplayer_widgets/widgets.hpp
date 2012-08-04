#ifndef WIDGETS_HPP
#define WIDGETS_HPP

#include <QtDesigner/QDesignerCustomWidgetCollectionInterface>
#include <QtGui/QFrame>

class WidgetPlugin : public QDesignerCustomWidgetInterface {
	Q_INTERFACES(QDesignerCustomWidgetInterface)
public:
	WidgetPlugin(const QString &name, const QString &dom): m_name(name), m_dom(dom) {}
	QString name() const {return m_name;}
	bool isContainer() const {return false;}
	bool isInitialized() const {return m_init;}
	QIcon icon() const {return QIcon();}
	QString codeTemplate() const {return QString();}
	QString whatsThis() const {return QString();}
	QString toolTip() const {return QString();}
	QString group() const {return QLatin1String("CMPlayer Widgets");}
	void initialize(QDesignerFormEditorInterface *) {if (m_init) return; m_init = true;}
	QString domXml() const { return m_dom;}
private:
	const QString m_name;
	const QString m_dom;
	bool m_init;
};

class QTextDocument;

// from KDE's KSqueezedTextLabel
class Label : public QFrame {
	Q_OBJECT
	Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged)
	Q_PROPERTY(Qt::Alignment alignment READ alignment WRITE setAlignment)
	Q_PROPERTY(Qt::TextElideMode textElideMode READ textElideMode WRITE setTextElideMode)
public:
	Label(QWidget *parent = 0);
	QSize minimumSizeHint() const;
	QSize sizeHint() const;
	void setAlignment(Qt::Alignment alignment) {
		if (m_alignment != alignment) {
			m_alignment = alignment;
			updateAll();
		}
	}
	Qt::Alignment alignment() const {return m_alignment;}
	Qt::TextElideMode textElideMode() const {return m_elide;}
	void setTextElideMode(Qt::TextElideMode mode) {
		if (m_elide != mode) {
			m_elide = mode;
			updateAll();
		}
	}
	QString text() const {return m_text;}
public slots:
	void setText(const QString &text) {
		if (m_text != text) {
			m_text = text;
			updateAll();
			emit textChanged(text);
		}
	}
signals:
	void textChanged(const QString &text);
protected:
	void paintEvent(QPaintEvent *event);
	void changeEvent(QEvent *event);
private:
	void updateAll() {updateGeometry(); update();}
	Qt::TextElideMode m_elide;
	Qt::Alignment m_alignment;
	QString m_text;
};

class LabelPlugin : public QObject, public WidgetPlugin {
	Q_OBJECT
public:
	LabelPlugin(QObject *parent = 0);
	QWidget *createWidget(QWidget *parent);
	QString includeFile() const {return "widgets.h";}
private:
};

class Widgets : public QObject, public QDesignerCustomWidgetCollectionInterface {
	Q_OBJECT
	Q_INTERFACES(QDesignerCustomWidgetCollectionInterface)
public:
	Widgets(QObject *parent = 0);
	QList<QDesignerCustomWidgetInterface*> customWidgets() const {return m_widgets;}
private:
	QList<QDesignerCustomWidgetInterface*> m_widgets;
};

#endif // WIDGETS_HPP
