#ifndef DIALOGS_HPP
#define DIALOGS_HPP

#include "stdafx.hpp"

class CheckDialog : public QDialog {
	Q_OBJECT
public:
	CheckDialog(QWidget *parent = 0, QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok);
	~CheckDialog();
	void setButtonBox(QDialogButtonBox::StandardButtons buttons);
	void setLabelText(const QString &text);
	void setCheckBoxText(const QString &text);
	void setChecked(bool checked);
	bool isChecked() const;
public slots:
	int exec();
private slots:
	void onButtonClicked(QAbstractButton *button);
private:
	struct Data;
	Data *d = nullptr;
};

class GetShortcutDialog : public QDialog {
	Q_OBJECT
public:
	GetShortcutDialog(const QKeySequence &shortcut, QWidget *parent = 0);
	~GetShortcutDialog();
	QKeySequence shortcut() const;
	void setShortcut(const QKeySequence &shortcut);
protected:
	bool eventFilter(QObject *obj, QEvent *event);
	void keyPressEvent(QKeyEvent *event);
	void keyReleaseEvent(QKeyEvent *event);
private slots:
	void setGetting(bool on);
	void erase();
private:
	static const int MaxKeyCount = 1;
	void getShortcut(QKeyEvent *event);
	struct Data;
	Data *d = nullptr;
};

class EncodingComboBox;

class EncodingFileDialog : public QFileDialog {
	Q_OBJECT
public:
	static QString getOpenFileName(QWidget *parent = 0
			, const QString &caption = QString()
			, const QString &dir = QString()
			, const QString &filter = QString()
			, QString *enc = 0);
	static QStringList getOpenFileNames(QWidget *parent = 0
			, const QString &caption = QString()
			, const QString &dir = QString()
			, const QString &filter = QString()
			, QString *enc = 0, FileMode = ExistingFiles);
private:
	EncodingFileDialog(QWidget *parent = 0
			, const QString &caption = QString::null
			, const QString &directory = QString::null
			, const QString &filter = QString::null
			, const QString &encoding = QString::null);
	void setEncoding(const QString &encoding);
	QString encoding() const;
	EncodingComboBox *combo = nullptr;
};

class Playlist;

class GetUrlDialog : public QDialog {
	Q_OBJECT
public:
	GetUrlDialog(QWidget *parent = 0);
	~GetUrlDialog();
	void setUrl(const QUrl &url);
	QUrl url() const;
	bool isPlaylist() const;
	Playlist playlist() const;
	QString encoding() const;
private:
	void accept();
	void _accept();
	struct Data;
	Data *d;
};

class AboutDialog : public QDialog {
	Q_OBJECT
public:
	AboutDialog(QWidget *parent = 0);
	~AboutDialog();
private slots:
	void showFullLicense();
private:
	struct Data;
	Data *d = nullptr;
};

class OpenDiscDialog : public QDialog {
	Q_OBJECT
public:
	OpenDiscDialog(QWidget *parent = 0);
	~OpenDiscDialog();
	void setDeviceList(const QStringList &devices);
	void setDevice(const QString &device);
	QString device() const;
public slots:
	void checkDevice(const QString &device);
private:
	struct Data;
	Data *d = nullptr;
};


#endif // DIALOGS_HPP
