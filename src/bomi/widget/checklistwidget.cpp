#include "checklistwidget.hpp"

struct CheckListWidget::_Data {
    CheckListWidget *p = nullptr;
    QCheckBox *header = nullptr;
    QSet<QListWidgetItem*> checkedItems;
    auto newItem(const QString &text, const QVariant &data,
                 int role, QListWidget *w) -> QListWidgetItem*
    {
        auto item = new QListWidgetItem;
        item->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        item->setText(text);
        item->setData(role, data);
        w->addItem(item);
        return item;
    }
    auto check(QListWidgetItem *item, bool c) -> void
    {
        item->setCheckState(c ? Qt::Checked : Qt::Unchecked);
        if (c)
            checkedItems.insert(item);
        else
            checkedItems.remove(item);
    }
    auto syncHeader() -> void
    {
        if (!header)
            return;
        auto was = header->blockSignals(true);
        int hidden = 0, checked = 0;
        for (int i = 0; i < p->count(); ++i) {
            auto item = p->item(i);
            if (item->isHidden())
                ++hidden;
            else if (item->checkState())
                ++checked;
        }
        if (!checked)
            header->setCheckState(Qt::Unchecked);
        else if (checked + hidden < p->count())
            header->setCheckState(Qt::PartiallyChecked);
        else
            header->setCheckState(Qt::Checked);
        header->blockSignals(was);
    }
};

struct CheckListWidget::Reset {
    Reset(_Data *d): d(d) {
        d->checkedItems.clear();
        was = d->p->blockSignals(true);
    }
    ~Reset() {
        d->p->blockSignals(was);
        d->syncHeader();
        emit d->p->checkedItemsChanged();
    }
    bool was = false;
    _Data *d = nullptr;
};

CheckListWidget::CheckListWidget(QWidget *parent)
    : QListWidget(parent), d(new _Data)
{
    d->p = this;
}

CheckListWidget::~CheckListWidget()
{
    delete d;
}

auto CheckListWidget::setHeaderCheckBox(QCheckBox *cb) -> void
{
    d->header = cb;

    connect(cb, &QCheckBox::stateChanged, this, [=] (int checked) {
        auto was = blockSignals(true);
        for (auto i = 0; i < count(); ++i)
            d->check(item(i), checked);
        blockSignals(was);
        emit checkedItemsChanged();
    });

    connect(this, &QListWidget::itemChanged, cb, [=] (QListWidgetItem *item) {
        if (d->checkedItems.contains(item) != !!item->checkState()) {
            d->check(item, item->checkState());
            d->syncHeader();
            emit checkedItemsChanged();
        }
    });

    d->syncHeader();
}

auto CheckListWidget::addItems(const QStringList &labels,
                               const QVariantList &data, int role) -> void
{
    for (int i = 0; i < labels.size(); ++i)
        addItem(labels[i], data.value(i), role);
}

auto CheckListWidget::addItem(const QString &label,
                              const QVariant &data, int role) -> void
{
    d->newItem(label, data, role, this);
}

auto CheckListWidget::checkedIndexes() const -> QList<int>
{
    QList<int> indexes;
    for (int i = 0; i < count(); ++i) {
        if (item(i)->checkState())
            indexes.push_back(i);
    }
    return indexes;
}

auto CheckListWidget::checkedTexts() const -> QStringList
{
    QStringList texts;
    for (int i = 0; i < count(); ++i) {
        if (item(i)->checkState())
            texts.push_back(item(i)->text());
    }
    return texts;
}

auto CheckListWidget::checkedItems() const -> QList<QListWidgetItem*>
{
    QList<QListWidgetItem*> items;
    for (int i = 0; i < count(); ++i) {
        if (item(i)->checkState())
            items.push_back(item(i));
    }
    return items;
}

auto CheckListWidget::checkedData(int role) const -> QVariantList
{
    QVariantList data;
    for (int i = 0; i < count(); ++i) {
        if (item(i)->checkState())
            data.push_back(item(i)->data(role));
    }
    return data;
}

auto CheckListWidget::setCheckedIndexes(const QList<int> &indexes) -> void
{
    Reset reset(d);
    for (int i = 0; i < count(); ++i)
        d->check(item(i), indexes.contains(i));
}

auto CheckListWidget::setCheckedTexts(const QStringList &texts, Qt::CaseSensitivity cs) -> void
{
    Reset reset(d);
    for (int i = 0; i < count(); ++i)
        d->check(item(i), texts.contains(item(i)->text(), cs));
}

auto CheckListWidget::setCheckedData(const QVariantList &data, int role) -> void
{
    Reset reset(d);
    for (int i = 0; i < count(); ++i)
        d->check(item(i), data.contains(item(i)->data(role)));
}

auto CheckListWidget::checkedStates() const -> QList<bool>
{
    QList<bool> checked; checked.reserve(count());
    for (int i = 0; i < count(); ++i)
        checked.push_back(item(i)->checkState());
    return checked;
}

auto CheckListWidget::toJson(int contents, int role) const -> QJsonArray
{
    QJsonArray json;
    for (int i = 0; i < count(); ++i) {
        QJsonObject o;
        auto item = this->item(i);
        o.insert(u"checked"_q, !!item->checkState());
        if (contents & Text)
            o.insert(u"text"_q, item->text());
        if (contents & Data)
            o.insert(u"data"_q, QJsonValue::fromVariant(item->data(role)));
        json.push_back(o);
    }
    return json;
}

auto CheckListWidget::setFromJson(const QJsonArray &json, int contents, bool resize, int role) -> bool
{
    Reset reset(d);
    if (resize)
        setCount(json.size());
    for (int i = 0; i < json.size(); ++i) {
        if (i >= count())
            break;
        auto o = json[i].toObject();
        auto item = this->item(i);
        if (contents & Text)
            item->setText(o[u"text"_q].toString());
        if (contents & Data)
            item->setData(role, o.value(u"data"_q).toVariant());
        d->check(item, o[u"checked"_q].toBool());
    }
    return true;
}

auto CheckListWidget::setCount(int n) -> void
{
    while (count() < n)
        d->newItem(QString(), QVariant(), Qt::UserRole, this);
    while (count() > n)
        delete takeItem(count() - 1);
    Q_ASSERT(count() == n);
}

auto CheckListWidget::toVariant(int contents, int role) const -> QVariant
{
    return QJsonValue(toJson(contents, role)).toVariant();
}

auto CheckListWidget::setFromVariant(const QVariant &var, int contents, bool resize, int role) -> bool
{
    return setFromJson(QJsonValue::fromVariant(var).toArray(), contents, resize, role);
}

auto CheckListWidget::setChecked(int i, bool checked) -> void
{
    if (auto item = this->item(i))
        d->check(item, checked);
}

auto CheckListWidget::clear() -> void
{
    Reset reset(d);
    QListWidget::clear();
}

auto CheckListWidget::isChecked(int row) const -> bool
{
    if (auto item = this->item(row))
        return item->checkState();
    return false;
}

auto CheckListWidget::data(int row, int role) const -> QVariant
{
    if (auto item = this->item(row))
        return item->data(role);
    return QVariant();
}

auto CheckListWidget::text(int row) const -> QString
{
    if (auto item = this->item(row))
        return item->text();
    return QString();
}
