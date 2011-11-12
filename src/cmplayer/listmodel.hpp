#ifndef LISTMODEL_HPP
#define LISTMODEL_HPP

#include <QtCore/QAbstractItemModel>
#include <QtCore/QStringList>

class ListModel : public QAbstractItemModel {
	Q_OBJECT
public:
	class Item {
	public:
		Item(): m_model(0), m_app(-1) {}
		virtual ~Item() {}
		virtual QVariant data(int column, int role) const = 0;
		const ListModel *model() const {return m_model;}
		ListModel *model() {return m_model;}
		int row() const {return m_model ? _row() : -1;}
	protected:
		void emitDataChanged();
		void emitDataChanged(int column);
	private:
		inline int _row() const {return m_app - m_model->first()->m_app;}
		friend class ListModel;
		ListModel *m_model;
		int m_app;
	};
	ListModel(int columnCount, QObject *parent = 0);
	~ListModel();
	void setColumnTitle(const QStringList &title);
	void setColumnTitle(int column, const QString &title);
	int rowCount(const QModelIndex &parent = QModelIndex()) const;
	int columnCount(const QModelIndex &parent =  QModelIndex()) const;
	int size() const {return m_item.size();}
	Item *first() {return m_item.first();}
	Item *last() {return m_item.last();}
	bool isEmpty() const {return m_item.isEmpty();}
	QVariant data(const QModelIndex &index, int role) const;
	QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
	QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	QModelIndex parent(const QModelIndex &/*child*/) const {return QModelIndex();}
	bool isValidRow(int row) const {return 0 <= row && row < m_item.size();}
	bool isValidColumn(int c) const {return 0 <= c && c < m_c_count;}

	// emit signals
	void clear();
	void setItems(const QList<Item*> &item);
	void moveItem(int from, int to);
	void appendItem(Item *item) {insertItem(m_item.size(), item);}
	void prependItem(Item *item) {insertItem(0, item);}
	void insertItem(int row, Item *item);
	Item *takeItem(int row);
	void removeItem(int row) {delete takeItem(row);}
	void removeItem(const QModelIndexList &indexes);
	// no signals
	bool move(int from, int to);
	void append(Item *item);
	void prepend(Item *item);
	void insert(int row, Item *item);
	void remove(int row) {delete take(row);}
	Item *take(int row);
	Item *at(int row) {return m_item[row];}
	const Item *at(int row) const {return m_item[row];}
	Item *item(int row) {return m_item.value(row, 0);}
	const Item *item(int row) const {return m_item.value(row, 0);}
protected:
private:
	void sync(int from, int to);
	void sync(int m) {sync(m, (m < m_item.size() - m) ? 0 : m_item.size()-1);}
	void emitRowDataChanged(int top, int bottom);
	void emitRowDataChanged(int row);
	void emitDataChanged(int row, int column);
	QList<Item*> m_item;
	QStringList m_c_title;
	int m_c_count;
};


#endif // ITEMLISTMODEL_HPP
