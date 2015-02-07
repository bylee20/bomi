#ifndef DEINTWIDGET_HPP
#define DEINTWIDGET_HPP

#include "video/deintoption.hpp"

class DeintWidget : public QGroupBox {
    Q_OBJECT
    Q_PROPERTY(DeintOptionSet value READ get WRITE set NOTIFY optionsChanged)
public:
    DeintWidget(QWidget *parent = nullptr);
    ~DeintWidget();
    auto set(const DeintOptionSet &options) -> void;
    auto get() const -> DeintOptionSet;
signals:
    void optionsChanged();
private:
    struct Data;
    Data *d;
};

#endif // DEINTWIDGET_HPP
