#ifndef COLORSELECTWIDGET_HPP
#define COLORSELECTWIDGET_HPP

class ColorSelectWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor NOTIFY colorChanged)
public:
    ColorSelectWidget(QWidget *parent = 0);
    ~ColorSelectWidget();
    auto setAlphaChannel(bool on) -> void;
    auto hasAlphaChannel() const -> bool;
    auto setColor(const QColor &color) -> void;
    auto color() const -> QColor;
signals:
    void colorChanged();
private:
    struct Data;
    Data *d;
};

DECL_PLUG_CHANGED(ColorSelectWidget, colorChanged)

#endif // COLORSELECTWIDGET_HPP
