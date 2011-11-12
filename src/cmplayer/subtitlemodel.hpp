#ifndef SUBTITLECOMPONENTMODEL_HPP
#define SUBTITLECOMPONENTMODEL_HPP

#include "subtitle.hpp"
#include "listmodel.hpp"
#include <QtGui/QTreeView>

class SubtitleComponentModel : public ListModel {
	Q_OBJECT
public:
	enum Column {Start = 0, End, Text, ColumnCount};
	SubtitleComponentModel(const Subtitle::Component *comp, QObject *parent = 0);
	void setCurrentNode(const Subtitle::Node *node);
	int currentRow() const;
	void setVisible(bool visible);
	QString name() const;
signals:
	void currentRowChanged(int row);
private:
	typedef Subtitle::Component::const_iterator c_iterator;
	typedef Subtitle::Component::iterator iterator;
	class Item;
	struct Data;
	Data *d;
};

class SubtitleComponentView : public QTreeView {
	Q_OBJECT
public:
	SubtitleComponentView(QWidget *parent = 0);
	void setModel(QAbstractItemModel *model);
	void setAutoScrollEnabled(bool enabled);
	void setTimeVisible(bool visible);
private slots:
	void updateCurrentRow(int row);
	void setModelToNull();
private:

	void showEvent(QShowEvent *event);
	void hideEvent(QHideEvent *event);
	struct Data;
	Data *d;
};

#endif // SUBTITLEMODEL_HPP
