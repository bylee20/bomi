#ifndef SUBTITLECOMPONENTMODEL_HPP
#define SUBTITLECOMPONENTMODEL_HPP

#include "stdafx.hpp"
#include "subtitle.hpp"
#include "listmodel.hpp"

class SubtitleComponentModel : public ListModel {
	Q_OBJECT
public:
	enum Column {Start = 0, End, Text, ColumnCount};
	SubtitleComponentModel(const SubComp *comp, QObject *parent = 0);
	void setCurrentCaption(const SubCapt *caption);
	int currentRow() const;
	void setVisible(bool visible);
	QString name() const;
signals:
	void currentRowChanged(int row);
private:
	typedef SubComp::const_iterator c_iterator;
	typedef SubComp::iterator iterator;
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
