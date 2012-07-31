#ifndef PREFDIALOG_HPP
#define PREFDIALOG_HPP

#include <QtGui/QDialog>

class QTreeWidgetItem;

class PrefDialog : public QDialog {
	Q_OBJECT
public:
	PrefDialog(QWidget *parent = 0);
	~PrefDialog();
private slots:
	void checkSubAutoselect(const QVariant &data);
	void onBlurKernelChanged();
	void onSharpenKernelChanged();
	void onCategoryChanged();
	void apply();
	void accept();
	void getShortcut(int id);
	void onCurrentMenuChanged(QTreeWidgetItem *it);
private:
	void changeEvent(QEvent *event);
	QString toString(const QLocale &locale);
	void fill();
	void retranslate();
	class MenuTreeItem;
	class Delegate;
	struct Data;
	Data *d;
};

#endif // PREFDIALOG_HPP
