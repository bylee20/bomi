#ifndef DIALOGS_HPP
#define DIALOGS_HPP

#include "stdafx.hpp"

class BBox : public QDialogButtonBox {
	Q_OBJECT
public:
	using Role = QDialogButtonBox::ButtonRole;
	using Button = QDialogButtonBox::StandardButton;
	using Layout = QDialogButtonBox::ButtonLayout;
	BBox(QWidget *parent = nullptr): QDialogButtonBox(parent) {
		m_layout = buttonLayout(this);
	}
	void setStandardButtons(StandardButtons buttons) {
		uint flags = buttons;
		for (int i=0; i<32 && flags; ++i) {
			const Button button = Button(1 << i);
			if (flags & button) {
				addButton(button);
				flags &= ~button;
			}
		}
	}
	void addButton(StandardButton button) {
		QDialogButtonBox::addButton(button)->setText(buttonText(button, m_layout));
	}
	static QString buttonText(Button button, Layout layout);
	static Layout buttonLayout(QWidget *w) {
		return Layout(w->style()->styleHint(QStyle::SH_DialogButtonLayout, 0, w));
	}
	static BBox *make(QDialog *dlg);
private:
	Layout m_layout;
};

class MBox : public QObject {
	Q_OBJECT
public:
	using Role = BBox::Role;
	using Button = BBox::Button;
	using Icon = QMessageBox::Icon;
	MBox(QWidget *parent = nullptr): QObject(parent) {
		m_mbox = new QMessageBox(parent);
		m_layout = BBox::Layout(m_mbox->style()->styleHint(QStyle::SH_DialogButtonLayout, 0, m_mbox));
	}
	MBox(QWidget *parent, Icon icon, const QString &title
		, const QString &text = QString()
		, std::initializer_list<Button> &&buttons = {}
		, Button def = BBox::NoButton)
	: MBox(parent) {
		addButtons(std::forward<std::initializer_list<Button>>(buttons));
		setTitle(title);
		setText(text);
		setDefaultButton(def);
		setIcon(icon);
	}
	~MBox() { delete m_mbox; }
	void addButton(const QString &text, Role role) {
		m_mbox->addButton(text, (QMessageBox::ButtonRole)role);
	}
	void addButton(Button button) {
		m_mbox->addButton((QMessageBox::StandardButton)button)->setText(BBox::buttonText(button, m_layout));
	}
	void addButtons(std::initializer_list<Button> &&buttons) {
		for (auto b : buttons)
			addButton(b);
	}

	int exec() { return m_mbox->exec(); }
	QMessageBox *mbox() const { return m_mbox; }
	QCheckBox *checkBox() const {
		if (!m_mbox->checkBox())
			m_mbox->setCheckBox(new QCheckBox);
		return m_mbox->checkBox();
	}
	bool isChecked() const { return m_mbox->checkBox() && m_mbox->checkBox()->isCheckable(); }
	void setInformativeText(const QString &text) { m_mbox->setInformativeText(text); }
	void setDetailedText(const QString &text) { m_mbox->setDetailedText(text); }
	void setDefaultButton(Button button) { m_mbox->setDefaultButton((QMessageBox::StandardButton)button); }
	void setIcon(Icon icon) { m_mbox->setIcon(icon); }
	void setTitle(const QString &title) { m_mbox->setWindowTitle(title); }
	void setText(const QString &text) { m_mbox->setText(text); }
#define DEC_POPUP(func, icon) \
	static int func(QWidget *parent, const QString &title, const QString &text, std::initializer_list<Button> &&buttons, Button def = BBox::NoButton) { \
		MBox mbox(parent, icon, title, text, std::forward<std::initializer_list<Button>>(buttons), def); return mbox.exec(); }
	DEC_POPUP(warn, Icon::Warning)
	DEC_POPUP(info, Icon::Information)
#undef DEC_POPUP
	Role role(QAbstractButton *button) const { return (Role)m_mbox->buttonRole(button); }
	Role clickedRole() const { return role(m_mbox->clickedButton()); }
private:
	QMessageBox *m_mbox = nullptr;
	BBox::Layout m_layout;
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
	void setIsoEnabled(bool on);
	QString device() const;
public slots:
	void checkDevice(const QString &device);
private:
	struct Data;
	Data *d = nullptr;
};


#endif // DIALOGS_HPP
