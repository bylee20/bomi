#ifndef DEINTWIDGET_HPP
#define DEINTWIDGET_HPP

class DeintCaps;
enum class DecoderDevice;

class DeintWidget : public QWidget {
    Q_OBJECT
public:
    DeintWidget(DecoderDevice decoder, QWidget *parent = nullptr);
    ~DeintWidget();
    auto set(const DeintCaps &caps) -> void;
    auto get() const -> DeintCaps;
    static auto informations() -> QString;
private:
    struct Data;
    Data *d;
};

#endif // DEINTWIDGET_HPP
