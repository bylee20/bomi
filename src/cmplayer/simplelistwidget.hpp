#ifndef SIMPLELISTWIDGET_HPP
#define SIMPLELISTWIDGET_HPP

#include <QtGui/QWidget>

class QListWidgetItem;

class SimpleListWidget : public QWidget {
	Q_OBJECT
public:
	SimpleListWidget(QWidget *parent = 0);
	~SimpleListWidget();
	void setValues(const QStringList &values);
	QStringList values() const;
	void setAddingAndErasingEnabled(bool enabled);
private slots:
	void slotCurrentItemChanged(QListWidgetItem *item);
	void slotErase();
	void slotUp();
	void slotDown();
	void slotAdd();
private:
	struct Data;
	Data *d;
};

#endif // SIMPLELISTWIDGET_HPP
