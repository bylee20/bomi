#ifndef WIDGETS_HPP
#define WIDGETS_HPP

#include "stdafx.hpp"
#include "enums.hpp"

class EncodingComboBox : public QComboBox {
    Q_OBJECT
public:
    EncodingComboBox(QWidget *parent = 0);
    auto encoding() const -> QString;
    auto setEncoding(const QString &encoding) -> void;
private:
    QStringList enc;
};

class LocaleComboBox : public QComboBox {
    Q_OBJECT
public:
    LocaleComboBox(QWidget *parent = nullptr);
    ~LocaleComboBox();
    auto currentLocale() const -> QLocale { return itemData(currentIndex()).toLocale(); }
    auto setCurrentLocale(const QLocale &locale) -> void { setCurrentIndex(findData(locale)); }
private:
    auto reset() -> void;
    auto changeEvent(QEvent *event) -> void {
        QComboBox::changeEvent(event);
        if (event->type() == QEvent::LanguageChange)
            reset();
    }
    struct Item;
    struct Data;
    Data *d;
};

class FontOptionWidget : public QWidget {
    Q_OBJECT
public:
    FontOptionWidget(QWidget *parent = 0);
    ~FontOptionWidget();
    auto set(const QFont &f) -> void {set(f.bold(), f.italic(), f.underline(), f.strikeOut());}
    auto set(bool bold, bool italic, bool underline, bool strikeout) -> void;
    auto bold() const -> bool;
    auto italic() const -> bool;
    auto underline() const -> bool;
    auto strikeOut() const -> bool;
    auto apply(QFont &font) -> void;
private:
    struct Data;
    Data *d;
};

class ColorSelectWidget : public QWidget {
    Q_OBJECT
public:
    ColorSelectWidget(QWidget *parent = 0);
    ~ColorSelectWidget();
    auto setColor(const QColor &color, bool hasAlpha) -> void;
    auto color() const -> QColor;
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
    auto addItemTextData(const QStringList &list) -> void {
        for (int i=0; i<list.size(); ++i)
            addItem(list[i], list[i]);
    }
    template<class T>
    auto addItemData(const QList<T> &list) -> void {
        for (int i=0; i<list.size(); ++i)
            addItem(QString(), list[i]);
    }
    auto setCurrentData(const QVariant &data, int role = Qt::UserRole) -> void {
        const int idx = findData(data, role);
        if (idx >= 0)
            setCurrentIndex(idx);
    }
    void setCurrentText(const QString &text, Qt::MatchFlags flags = static_cast<Qt::MatchFlags>(Qt::MatchExactly|Qt::MatchCaseSensitive)) {
        const int idx = findText(text, flags);
        if (idx >= 0)
            setCurrentIndex(idx);
    }
//    template<class T>
//    auto currentValue(int role = Qt::UserRole) const -> T {
//        const int idx = currentIndex();
//        return idx < 0 ? T() : itemData(idx, role).value<T>();
//    }
    auto currentData(int role = Qt::UserRole) const -> QVariant {
        const int idx = currentIndex();
        return idx < 0 ? QVariant() : itemData(idx, role);
    }
signals:
    void currentDataChanged(const QVariant &data);
};

template<class Enum>
class EnumComboBox : public DataComboBox {
public:
    EnumComboBox(QWidget *parent = 0): DataComboBox(parent) {
        setup(this);
        retranslate(this);
    }
    static auto setup(QComboBox *combo) -> void {
        combo->clear();
        for (auto &item : EnumInfo<Enum>::items())
            combo->addItem(QString(), (int)item.value);
    }
    static auto retranslate(QComboBox *combo) -> void {
        const auto items = EnumInfo<Enum>::items();
        Q_ASSERT((int)items.size() == combo->count());
        for (int i=0; i<(int)items.size(); ++i)
            combo->setItemText(i, EnumInfo<Enum>::description(items[i].value));
    }
    auto currentValue() const -> Enum { return EnumInfo<Enum>::from(currentData().toInt()); }
    auto setCurrentValue(Enum e) -> void { setCurrentData((int)e); }
    auto setRetranslatable(bool retrans) -> void { m_retrans = retrans; }
private:
    auto changeEvent(QEvent *event) -> void {
        QComboBox::changeEvent(event);
        if (event->type() == QEvent::LanguageChange && m_retrans)
            retranslate(this);
    }
    bool m_retrans = true;
};

typedef EnumComboBox<GeneratePlaylist> GeneratePlaylistComboBox;
typedef EnumComboBox<SubtitleAutoload> SubtitleAutoloadComboBox;
typedef EnumComboBox<SubtitleAutoselect> SubtitleAutoselectComboBox;
typedef EnumComboBox<OsdScalePolicy> OsdScalePolicyComboBox;
typedef EnumComboBox<ClippingMethod> ClippingMethodComboBox;
typedef EnumComboBox<InterpolatorType> InterpolatorTypeComboBox;

#endif // WIDGETS_HPP
