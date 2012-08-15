#ifndef PREFDIALOG_HPP
#define PREFDIALOG_HPP

#include <QtGui/QDialog>

class QTreeWidgetItem;		class QAbstractButton;
class Pref;

class PrefDialog : public QDialog {
	Q_OBJECT
public:
	PrefDialog(QWidget *parent = 0);
	~PrefDialog();
signals:
	void applicationRequested();
private slots:
	void checkSubAutoselect(const QVariant &data);
	void onBlurKernelChanged();
	void onSharpenKernelChanged();
	void onCategoryChanged();
	void apply();
	void getShortcut(int id);
	void onCurrentMenuChanged(QTreeWidgetItem *it);
	void onDialogButtonClicked(QAbstractButton *button);
	void onSkinIndexChanged(int idx);
private:
	void changeEvent(QEvent *event);
	void showEvent(QShowEvent *event);
	QString toString(const QLocale &locale);
	void fill(const Pref &p);
	void retranslate();
	class MenuTreeItem;
	class Delegate;
	struct Data;
	Data *d;
};

#endif // PREFDIALOG_HPP
