#ifndef PREF_WIDGET_HPP
#define PREF_WIDGET_HPP

#include <QtGui/QWidget>
#include "pref.hpp"

class QTreeWidgetItem;		class QComboBox;


class Pref::Widget : public QWidget {
	Q_OBJECT
public:
	Widget(QWidget *parent = 0);
	~Widget();
	int pageCount() const;
	int currentPage() const;
	QString pageName(int idx) const;
	QIcon pageIcon(int idx) const;
	QSize sizeHint() const;
signals:

public slots:
	void fill();
	void apply();
	void setCurrentPage(int idx) const;
private slots:
	void getShortcut(int id);
	void checkCurrentMenu(QTreeWidgetItem *current);
	void checkSubAutoselect(const QVariant &data);
private:
	void retranslate();
	void changeEvent(QEvent *event);
	static QString toString(const QLocale &locale);
//	static QVariant currentComboData(QComboBox *combo);
//	static void setComboIndex(QComboBox *combo, const QVariant &value);
	struct Data;
	class MenuTreeItem;
	Data *d;
};

#endif // PREF_WIDGET_HPP
