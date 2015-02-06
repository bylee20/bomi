#ifndef CHANNELMANIPULATIONWIDGET_HPP
#define CHANNELMANIPULATIONWIDGET_HPP

#include "audio/channellayoutmap.hpp"

class ChannelLayoutMap;
enum class ChannelLayout;

class ChannelManipulationWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(ChannelLayoutMap value READ map WRITE setMap NOTIFY mapChanged)
public:
    ChannelManipulationWidget(QWidget *parent = nullptr);
    ~ChannelManipulationWidget();
    auto setMap(const ChannelLayoutMap &map) -> void;
    auto map() const -> ChannelLayoutMap;
    auto setCurrentLayouts(ChannelLayout src, ChannelLayout dst) -> void;
signals:
    void mapChanged();
private:
    struct Data;
    Data *d;
};

#endif // CHANNELMANIPULATIONWIDGET_HPP
