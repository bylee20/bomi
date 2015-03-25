#ifndef WINDOWSIZE_HPP
#define WINDOWSIZE_HPP

struct WindowSize {
    DECL_EQ(WindowSize, &T::display_based, &T::rate)
    WindowSize() = default;
    WindowSize(bool dis, double rate): display_based(dis), rate(rate) { }
    bool display_based = false;
    double rate = 1.0;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    auto fillAction(QAction *action) const -> void;
    static auto defaults() -> QList<WindowSize>;
    static auto baseText(bool dis) -> QString;
};

Q_DECLARE_METATYPE(WindowSize)

class WindowSizeWidget : public QGroupBox {
    Q_OBJECT
    Q_PROPERTY(QList<WindowSize> value READ values WRITE setValues NOTIFY valuesChanged)
public:
    WindowSizeWidget(QWidget *parent = nullptr);
    ~WindowSizeWidget();
    auto values() const -> QList<WindowSize>;
    auto setValues(const QList<WindowSize> &values) -> void;
signals:
    void valuesChanged();
private:
    struct Data;
    Data *d;
};

#endif // WINDOWSIZE_HPP
