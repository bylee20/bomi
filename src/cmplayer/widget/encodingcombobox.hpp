#ifndef ENCODINGCOMBOBOX_HPP
#define ENCODINGCOMBOBOX_HPP

class EncodingComboBox : public QComboBox {
    Q_OBJECT
    Q_PROPERTY(QString encoding READ encoding WRITE setEncoding)
public:
    EncodingComboBox(QWidget *parent = 0);
    auto encoding() const -> QString;
    auto setEncoding(const QString &encoding) -> void;
};

#endif // ENCODINGCOMBOBOX_HPP
