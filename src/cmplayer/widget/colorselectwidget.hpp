#ifndef COLORSELECTWIDGET_HPP
#define COLORSELECTWIDGET_HPP

class ColorSelectWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor)
public:
    ColorSelectWidget(QWidget *parent = 0);
    ~ColorSelectWidget();
    auto setAlphaChannel(bool on) -> void;
    auto hasAlphaChannel() const -> bool;
    auto setColor(const QColor &color) -> void;
    auto color() const -> QColor;
private:
    struct Data;
    Data *d;
};

#endif // COLORSELECTWIDGET_HPP
