#ifndef CHANNELMANIPULATIONWIDGET_HPP
#define CHANNELMANIPULATIONWIDGET_HPP

class ChannelLayoutMap;
enum class ChannelLayout;

class ChannelManipulationWidget : public QWidget {
    Q_OBJECT
public:
    ChannelManipulationWidget(QWidget *parent = nullptr);
    ~ChannelManipulationWidget();
    auto setMap(const ChannelLayoutMap &map) -> void;
    auto map() const -> ChannelLayoutMap;
    auto setCurrentLayouts(ChannelLayout src, ChannelLayout dst) -> void;
private:
    struct Data;
    Data *d;
};

#endif // CHANNELMANIPULATIONWIDGET_HPP
