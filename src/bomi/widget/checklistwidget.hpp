#ifndef CHECKLISTWIDGET_HPP
#define CHECKLISTWIDGET_HPP

#include <QListWidget>

enum CheckListContent {
    CheckListNoContent = 0,
    CheckListText = 1,
    CheckListData = 2
};

class CheckListWidget : public QListWidget {
    Q_OBJECT
    Q_PROPERTY(QList<int> checkedIndexes READ checkedIndexes WRITE setCheckedIndexes NOTIFY checkedItemsChanged)
    Q_PROPERTY(QStringList checkedTexts READ checkedTexts WRITE setCheckedTexts NOTIFY checkedItemsChanged)
    Q_PROPERTY(QVariantList checkedData READ checkedData WRITE setCheckedData NOTIFY checkedItemsChanged)
public:
    enum Content { NoConent = 0, Text = CheckListText, Data = CheckListData };
    CheckListWidget(QWidget *parent);
    ~CheckListWidget();
    auto addItems(const QStringList &labels, const QVariantList &data = QVariantList(), int role = Qt::UserRole) -> void;
    auto addItem(const QString &label, const QVariant &data = QVariant(), int role = Qt::UserRole) -> void;
    auto setChecked(int i, bool checked) -> void;
    auto setHeaderCheckBox(QCheckBox *cb) -> void;
    auto setCheckedIndexes(const QList<int> &indexes) -> void;
    auto setCheckedTexts(const QStringList &texts, Qt::CaseSensitivity = Qt::CaseSensitive) -> void;
    auto setCheckedData(const QVariantList &data, int role = Qt::UserRole) -> void;
    auto checkedIndexes() const -> QList<int>;
    auto checkedTexts() const -> QStringList;
    auto checkedItems() const -> QList<QListWidgetItem*>;
    auto checkedData(int role = Qt::UserRole) const -> QVariantList;
    auto checkedStates() const -> QList<bool>;
    auto toVariant(int contents = Text | Data, int role = Qt::UserRole) const -> QVariant;
    auto setFromVariant(const QVariant &var, int contents = Text | Data, bool resize = true, int role = Qt::UserRole) -> bool;
    auto toJson(int contents = Text | Data, int role = Qt::UserRole) const -> QJsonArray;
    auto setFromJson(const QJsonArray &json, int contents = Text | Data, bool resize = true, int role = Qt::UserRole) -> bool;
    auto clear() -> void;
    auto setCount(int count) -> void;
    auto data(int row, int role = Qt::UserRole) const -> QVariant;
    auto text(int row) const -> QString;
    auto isChecked(int row) const -> bool;
signals:
    void checkedItemsChanged();
private:
    struct _Data;
    struct Reset;
    _Data *d;
};

#endif // CHECKLISTWIDGET_HPP
