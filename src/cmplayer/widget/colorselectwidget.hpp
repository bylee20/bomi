#ifndef COLORSELECTWIDGET_HPP
#define COLORSELECTWIDGET_HPP

#include "stdafx.hpp"

class ColorSelectWidget : public QWidget {
    Q_OBJECT
public:
    ColorSelectWidget(QWidget *parent = 0);
    ~ColorSelectWidget();
    auto setColor(const QColor &color, bool hasAlpha) -> void;
    auto color() const -> QColor;
private:
    struct Data;
    Data *d;
};

#endif // COLORSELECTWIDGET_HPP
