#ifndef VIDEOCOLORDIALOG_HPP
#define VIDEOCOLORDIALOG_HPP

class VideoColor;

class VideoColorDialog : public QDialog {
    Q_OBJECT
public:
    VideoColorDialog(QWidget *parent = nullptr);
    ~VideoColorDialog();
    auto setColor(const VideoColor &eq) -> void;
    auto color() const -> VideoColor;
signals:
    void colorChanged(const VideoColor &eq);
private:
    struct Data;
    Data *d;
};

#endif // VIDEOCOLORDIALOG_HPP
