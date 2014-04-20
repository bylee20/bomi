#ifndef TOPLEVELITEM_HPP
#define TOPLEVELITEM_HPP

#include "stdafx.hpp"

class TopLevelItem : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(QColor shade READ shade WRITE setShade NOTIFY shadeChanged)
public:
    TopLevelItem(QQuickItem *parent = nullptr);
    ~TopLevelItem();
    QColor shade() const;
    void setShade(const QColor &color);
    bool filteredMousePressEvent() const;
    void resetMousePressEventFilterState();
    Q_INVOKABLE void check();
signals:
    void shadeChanged();
private:
    void itemChange(ItemChange change, const ItemChangeData &data);
    void geometryChanged(const QRectF &new_, const QRectF &old) override;
    void mousePressEvent(QMouseEvent *event) override;
    QSGNode *updatePaintNode(QSGNode *old, UpdatePaintNodeData *data) override;
    struct Data;
    Data *d;
};

#endif // TOPLEVELITEM_HPP
