#ifndef PREFWIDGETS_HPP
#define PREFWIDGETS_HPP

#include "misc/simplelistmodel.hpp"
#include "player/mrlstate.hpp"

enum class CodecId;

class HwAccCodecBox : public QGroupBox {
    Q_OBJECT
    Q_PROPERTY(QList<CodecId> value READ value WRITE setValue NOTIFY valueChanged)
public:
    HwAccCodecBox(QWidget *parent = nullptr);
    auto value() const -> QList<CodecId>;
    auto setValue(const QList<CodecId> &list) -> void;
signals:
    void valueChanged();
private:
    QMap<CodecId, QCheckBox*> m_checks;
};

class DataButtonGroup : public QButtonGroup {
    Q_OBJECT
    Q_PROPERTY(QVariant value READ currentData WRITE setCurrentData NOTIFY currentDataChanged)
public:
    DataButtonGroup(QObject *parent = nullptr);
    auto addButton(QAbstractButton *button, const QVariant &data) -> void;
    auto setCurrentData(const QVariant &data) -> void;
    auto currentData() const -> QVariant { return m_data.value(m_button); }
    auto button(const QVariant &data) const -> QAbstractButton*;
signals:
    void currentDataChanged(const QVariant &data);
private:
    QMap<QAbstractButton*, QVariant> m_data;
    QAbstractButton *m_button = nullptr;
};

class MrlStatePropertyListModel
        : public SimpleListModel<MrlState::PropertyInfo,
                                 QVector<MrlState::PropertyInfo>> {
    Q_OBJECT
    Q_PROPERTY(QStringList value READ value WRITE setValue NOTIFY valueChanged)
public:
    MrlStatePropertyListModel(QObject *parent)
        : SimpleListModel<MrlState::PropertyInfo,
                          QVector<MrlState::PropertyInfo> >(parent)
    {
        setCheckable(0, true);
        setList(MrlState::restorableProperties());
        connect(this, &MrlStatePropertyListModel::contentsChanged,
                this, &MrlStatePropertyListModel::valueChanged);
    }
    auto displayData(int row, int /*column*/) const -> QVariant
        { return at(row).description; }
    auto value() const -> QStringList
    {
        auto restores = checkedList(0);
        QStringList list;
        for (int i=0; i<restores.size(); ++i) {
            if (restores[i])
                list.append(at(i).property);
        }
        return list;
    }
    auto setValue(const QStringList &list) -> void
    {
        QVector<bool> restores(size(), false);
        for (int i=0; i<size(); ++i)
            restores[i] = list.contains(at(i).property);
        setChecked(0, restores);
    }
    auto compare(const QVariant &var) const -> bool
    {
        auto list = var.toStringList();
        auto restores = checkedList(0);
        for (int i = 0; i < restores.size(); ++i) {
            if (list.contains(at(i).property) != restores[i])
                return false;
        }
        return true;
    }
signals:
    void valueChanged();
};

#endif // PREFWIDGETS_HPP
