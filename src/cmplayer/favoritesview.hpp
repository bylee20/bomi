#ifndef FAVORITESVIEW_HPP
#define FAVORITESVIEW_HPP

#include <QtGui/QWidget>

class PlayEngine;			class QTreeWidgetItem;
class QSettings;

class FavoritesView : public QWidget {
	Q_OBJECT
private:
	class Item;
public:
	FavoritesView(PlayEngine *player, QWidget *parent = 0);
	~FavoritesView();
	void save() const;
	void load();
//signals:
//	void openRequested(const Core::Mrl &mrl);
private slots:
	void addNew();
	void addFolder();
	void addCurrent();
	void erase();
	void showContext(const QPoint &pos);
	void slotDblClick(QTreeWidgetItem *item);
	void modify(Item *item = 0);
private:
	void save(QTreeWidgetItem *item, QSettings *set) const;
	void load(QTreeWidgetItem *parent, QSettings *set);
	class ItemDialog;
	class Item;
	class FolderItem;
	class MrlItem;
	void addItem(Item *item, bool topLevel);
	struct Data;
	Data *d;
};

#endif // FAVORITESVIEW_HPP
