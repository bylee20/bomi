#ifndef DATACOMBOBOX_HPP
#define DATACOMBOBOX_HPP

class DataComboBox : public QComboBox {
    Q_OBJECT
    Q_PROPERTY(QVariant value READ currentData WRITE setCurrentData NOTIFY currentDataChanged)
public:
    DataComboBox(QWidget *parent = nullptr);
    auto addItemTextData(const QStringList &list) -> void
        { for (int i=0; i<list.size(); ++i) addItem(list[i], list[i]); }
    template<class T>
    auto addItem(const QString &text, const T &t, int role = Qt::UserRole) -> void
        { QComboBox::addItem(text); setItemData(count()-1, QVariant::fromValue<T>(t), role); }
    auto setCurrentData(const QVariant &data, int role = Qt::UserRole) -> void;
    auto setCurrentText(const QString &text,
                        Qt::MatchFlags flags = Qt::MatchExactly
                                              | Qt::MatchCaseSensitive) -> void;
    auto data(int i, int role = Qt::UserRole) const -> QVariant
        { return itemData(i, role); }
    template<class T>
    auto value(int i, int role = Qt::UserRole) const -> T
        { return itemData(i, role).value<T>(); }
    template<class T>
    auto currentValue() const -> T { return currentData().value<T>(); }
    template<class T>
    auto setCurrentValue(const T &t, int role = Qt::UserRole) -> void
        { setCurrentData(QVariant::fromValue<T>(t), role); }
signals:
    void currentDataChanged(const QVariant &data);
};

inline auto DataComboBox::setCurrentData(const QVariant &data, int role) -> void
{
    const int idx = findData(data, role);
    if (idx >= 0)
        setCurrentIndex(idx);
}

inline auto DataComboBox::setCurrentText(const QString &text,
                                         Qt::MatchFlags flags) -> void
{
    const int idx = findText(text, flags);
    if (idx >= 0)
        setCurrentIndex(idx);
}

#endif // DATACOMBOBOX_HPP
