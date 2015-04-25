#ifndef SUBTITLEVIEWER_HPP
#define SUBTITLEVIEWER_HPP

class PlayEngine;                       class Subtitle;
class SubCompModel;                     class SubComp;

class SubtitleViewer : public QDialog {
    Q_DECLARE_TR_FUNCTIONS(SubtitleViewer)
    using Seek = std::function<void(int)>;
public:
    SubtitleViewer(QWidget *parent = 0);
    ~SubtitleViewer();
    auto setComponents(const QVector<SubComp> &comps) -> void;
    auto setSeekFunc(Seek &&func) -> void;
    auto setCurrentTime(int time) -> void;
private:
    auto showEvent(QShowEvent *event) -> void;
    auto updateModels() -> void;
    class CompView;
    struct Data;
    Data *d;
};

#endif // SUBTITLEVIEWER_HPP
