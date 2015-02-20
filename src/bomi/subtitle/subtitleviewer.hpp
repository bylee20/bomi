#ifndef SUBTITLEVIEWER_HPP
#define SUBTITLEVIEWER_HPP

class PlayEngine;                       class Subtitle;
class SubCompModel;                     class SubComp;

class SubtitleViewer : public QDialog {
    Q_OBJECT
public:
    SubtitleViewer(QWidget *parent = 0);
    ~SubtitleViewer();
    auto setComponents(const QVector<SubComp> &comps) -> void;
signals:
    void seekRequested(int time);
private:
    auto showEvent(QShowEvent *event) -> void;
    auto updateModels() -> void;
    class CompView;
    struct Data;
    Data *d;
};

#endif // SUBTITLEVIEWER_HPP
