#ifndef DATACOMBOBOX_HPP
#define DATACOMBOBOX_HPP

class DataComboBox : public QComboBox {
    Q_OBJECT
    Q_PROPERTY(QVariant value READ currentData WRITE setCurrentData NOTIFY currentDataChanged)
protected:
public:
    DataComboBox(QWidget *parent = nullptr);
    auto addItemTextData(const QStringList &list) -> void
        { for (int i=0; i<list.size(); ++i) addItem(list[i], list[i]); }
    template<class T>
    auto addItemData(const QList<T> &list) -> void
        { for (int i=0; i<list.size(); ++i) addItem(QString(), list[i]); }
    auto setCurrentData(const QVariant &data, int role = Qt::UserRole) -> void;
    auto setCurrentText(const QString &text,
                        Qt::MatchFlags flags = Qt::MatchExactly
                                              | Qt::MatchCaseSensitive) -> void;
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
