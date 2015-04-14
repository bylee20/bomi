#ifndef FONTCOMBOBOX_HPP
#define FONTCOMBOBOX_HPP

class FontComboBox : public QComboBox {
    Q_OBJECT
    Q_PROPERTY(QFont currentFont READ currentFont WRITE setCurrentFont NOTIFY currentFontChanged)
public:
    FontComboBox(QWidget *parent = nullptr);
    ~FontComboBox();
    auto setCurrentFont(const QFont &font) -> void;
    auto currentFont() const -> QFont;
    auto setFixedFontOnly(bool fixed) -> void;
signals:
    void currentFontChanged();
private:
    struct Data;
    Data *d;
};

#endif // FONTCOMBOBOX_HPP
