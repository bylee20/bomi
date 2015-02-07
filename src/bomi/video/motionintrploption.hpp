#ifndef MOTIONINTRPLOPTION_HPP
#define MOTIONINTRPLOPTION_HPP

struct MotionIntrplOption
{
    DECL_EQ(MotionIntrplOption, &T::sync_to_monitor, &T::target_fps)
    bool sync_to_monitor = true;
    double target_fps = 60;
    auto fps() const -> double;
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
};

Q_DECLARE_METATYPE(MotionIntrplOption)

class MotionIntrplOptionWidget : public QGroupBox {
    Q_OBJECT
    Q_PROPERTY(MotionIntrplOption value READ option WRITE setOption NOTIFY optionChanged)
public:
    MotionIntrplOptionWidget(QWidget *parent = nullptr);
    ~MotionIntrplOptionWidget();
    auto option() const -> MotionIntrplOption;
    auto setOption(const MotionIntrplOption &option) -> void;
signals:
    void optionChanged();
private:
    auto showEvent(QShowEvent *e) -> void;
    struct Data;
    Data *d;
};

#endif // MOTIONINTRPLOPTION_HPP
