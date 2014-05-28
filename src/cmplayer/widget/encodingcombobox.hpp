#ifndef ENCODINGCOMBOBOX_HPP
#define ENCODINGCOMBOBOX_HPP

class EncodingComboBox : public QComboBox {
    Q_OBJECT
public:
    EncodingComboBox(QWidget *parent = 0);
    auto encoding() const -> QString;
    auto setEncoding(const QString &encoding) -> void;
private:
    QStringList enc;
};

#endif // ENCODINGCOMBOBOX_HPP
