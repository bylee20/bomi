#ifndef WIDGETS_HPP
#define WIDGETS_HPP

#include "stdafx.hpp"
#include "enums.hpp"

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
	Q_OBJECT
public:
	FontOptionWidget(QWidget *parent = 0);
	~FontOptionWidget();
	void set(const QFont &f) {set(f.bold(), f.italic(), f.underline(), f.strikeOut());}
	void set(bool bold, bool italic, bool underline, bool strikeout);
	bool bold() const;
	bool italic() const;
	bool underline() const;
	bool strikeOut() const;
	void apply(QFont &font);
private:
	struct Data;
	Data *d;
};

class ColorSelectWidget : public QWidget {
	Q_OBJECT
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
protected:
	using IntFunc = void(QComboBox::*)(int);
public:
	DataComboBox(QWidget *parent = 0): QComboBox(parent) {
		connect(this, static_cast<IntFunc>(&QComboBox::currentIndexChanged), [this] (int idx) {
			emit currentDataChanged(idx < 0 ? QVariant() : itemData(idx));
		});
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
	void setCurrentText(const QString &text, Qt::MatchFlags flags = static_cast<Qt::MatchFlags>(Qt::MatchExactly|Qt::MatchCaseSensitive)) {
		const int idx = findText(text, flags);
		if (idx >= 0)
			setCurrentIndex(idx);
	}
//	template<typename T>
//	T currentValue(int role = Qt::UserRole) const {
//		const int idx = currentIndex();
//		return idx < 0 ? T() : itemData(idx, role).value<T>();
//	}
	QVariant currentData(int role = Qt::UserRole) const {
		const int idx = currentIndex();
		return idx < 0 ? QVariant() : itemData(idx, role);
	}
signals:
	void currentDataChanged(const QVariant &data);
};

template<typename Enum>
class EnumComboBox : public DataComboBox {
public:
	EnumComboBox(QWidget *parent = 0): DataComboBox(parent) {
		setup(this);
		retranslate(this);
	}
	static void setup(QComboBox *combo) {
		combo->clear();
		for (auto &item : EnumInfo<Enum>::items())
			combo->addItem(QString(), (int)item.value);
	}
	static void retranslate(QComboBox *combo) {
		const auto items = EnumInfo<Enum>::items();
        Q_ASSERT((int)items.size() == combo->count());
		for (int i=0; i<(int)items.size(); ++i)
			combo->setItemText(i, EnumInfo<Enum>::description(items[i].value));
	}
	Enum currentValue() const { return EnumInfo<Enum>::from(currentData().toInt()); }
	void setCurrentValue(Enum e) { setCurrentData((int)e); }
private:
	void changeEvent(QEvent *event) {
		QComboBox::changeEvent(event);
		if (event->type() == QEvent::LanguageChange)
			retranslate(this);
	}
};

typedef EnumComboBox<GeneratePlaylist> GeneratePlaylistComboBox;
typedef EnumComboBox<SubtitleAutoload> SubtitleAutoloadComboBox;
typedef EnumComboBox<SubtitleAutoselect> SubtitleAutoselectComboBox;
typedef EnumComboBox<OsdScalePolicy> OsdScalePolicyComboBox;
typedef EnumComboBox<ClippingMethod> ClippingMethodComboBox;
typedef EnumComboBox<InterpolatorType> InterpolatorTypeComboBox;

#endif // WIDGETS_HPP
