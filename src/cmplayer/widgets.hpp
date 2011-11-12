#ifndef WIDGETS_HPP
#define WIDGETS_HPP

#include <QtGui/QWidget>
#include <QtGui/QComboBox>
#include <QtCore/QEvent>
#include "enums.hpp"

#include <QtGui/QSlider>
#include <QtGui/QToolButton>
#include <QtGui/QComboBox>

class PlayEngine;		class AudioController;

class JumpSlider : public QSlider {
Q_OBJECT
public:
	JumpSlider(QWidget *parent = 0);
protected:
	virtual void mousePressEvent(QMouseEvent *event);
};

class SeekSlider : public JumpSlider {
Q_OBJECT
	public:
	SeekSlider(QWidget *parent = 0);
private slots:
	void setDuration(int duration);
	void slotTick(int time);
	void seek(int msec);
private:
	PlayEngine *engine;
	bool tick;
};

class VolumeSlider : public JumpSlider {
	Q_OBJECT
public:
	VolumeSlider(QWidget *parent = 0);
};

class Button : public QToolButton {
	Q_OBJECT
public:
	Button(QWidget *parent);
	Button(const QIcon &icon, QWidget *parent);
	Button(const QString &text, QWidget *parent);
	~Button();
	void setIconSize(int extent);
	void setAction(QAction *action, bool update);
	void setBlock(bool block);
private slots:
	void toggleAction(bool checked);
private:
	void init();
	struct Data;
	Data *d;
};

class EncodingComboBox : public QComboBox {
	Q_OBJECT
public:
	EncodingComboBox(QWidget *parent = 0);
	QString encoding() const;
	void setEncoding(const QString &encoding);
private:
	QStringList enc;
};

class FontOptionWidget : public QWidget {
public:
	FontOptionWidget(QWidget *parent = 0);
	~FontOptionWidget();
	void set(const QFont &f) {set(f.bold(), f.italic(), f.underline(), f.strikeOut());}
	void set(bool bold, bool italic, bool underline, bool strikeout);
	bool bold() const;
	bool italic() const;
	bool underline() const;
	bool strikeOut() const;
private:
	struct Data;
	Data *d;
};

class ColorSelectWidget : public QWidget {
public:
	ColorSelectWidget(QWidget *parent = 0);
	~ColorSelectWidget();
	void setColor(const QColor &color, bool hasAlpha);
	QColor color() const;
private:
	struct Data;
	Data *d;
};

class DataComboBox : public QComboBox {
	Q_OBJECT
public:
	DataComboBox(QWidget *parent = 0): QComboBox(parent) {
		connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(emitCurrentDataChanged(int)));
	}
	void addItemTextData(const QStringList &list) {
		for (int i=0; i<list.size(); ++i)
			addItem(list[i], list[i]);
	}
	template<typename T>
	void addItemData(const QList<T> &list) {
		for (int i=0; i<list.size(); ++i)
			addItem(QString(), list[i]);
	}
	void setCurrentData(const QVariant &data, int role = Qt::UserRole) {
		const int idx = findData(data, role);
		if (idx >= 0)
			setCurrentIndex(idx);
	}
	QVariant currentData(int role = Qt::UserRole) const {
		const int idx = currentIndex();
		return idx < 0 ? QVariant() : itemData(idx, role);
	}
signals:
	void currentDataChanged(const QVariant &data);
private slots:
	void emitCurrentDataChanged(int idx) {
		if (idx < 0)
			emit currentDataChanged(QVariant());
		emit currentDataChanged(itemData(idx));
	}
};

template<typename Enum>
class EnumComboBox : public DataComboBox {
public:
	typedef Enum EnumType;
	EnumComboBox(QWidget *parent = 0): DataComboBox(parent) {
		setup(this);
		retranslate(this);
	}
	static void setup(QComboBox *combo) {
		const typename Enum::List list = Enum::list();
		combo->clear();
		for (int i=0; i<list.size(); ++i)
			combo->addItem(QString(), list[i].id());
	}
	static void retranslate(QComboBox *combo) {
		const typename Enum::List list = Enum::list();
		Q_ASSERT(list.size() == combo->count());
		for (int i=0; i<list.size(); ++i)
			combo->setItemText(i, list[i].description());
	}
private:
	void changeEvent(QEvent *event) {
		QComboBox::changeEvent(event);
		if (event->type() == QEvent::LanguageChange)
			retranslate(this);
	}
};

typedef EnumComboBox<Enum::GeneratePlaylist> GeneratePlaylistComboBox;
typedef EnumComboBox<Enum::SubtitleAutoload> SubtitleAutoloadComboBox;
typedef EnumComboBox<Enum::SubtitleAutoselect> SubtitleAutoselectComboBox;
typedef EnumComboBox<Enum::OsdAutoSize> OsdAutoSizeComboBox;


#endif // WIDGETS_HPP
