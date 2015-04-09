#ifndef VIDEOCOLORDIALOG_HPP
#define VIDEOCOLORDIALOG_HPP

class VideoColor;

class VideoColorDialog : public QDialog {
    Q_OBJECT
    using Update = std::function<void(const VideoColor&)>;
public:
    VideoColorDialog(QWidget *parent = nullptr);
    ~VideoColorDialog();
    auto setColor(const VideoColor &eq) -> void;
    auto color() const -> VideoColor;
    auto setUpdateFunc(Update &&func) -> void;
private:
    struct Data;
    Data *d;
};

#endif // VIDEOCOLORDIALOG_HPP
