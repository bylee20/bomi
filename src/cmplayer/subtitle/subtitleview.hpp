#ifndef SUBTITLEVIEW_HPP
#define SUBTITLEVIEW_HPP

#include "stdafx.hpp"
#include "dialogs.hpp"

class PlayEngine;        class Subtitle;
class SubCompModel;

class SubtitleView : public QDialog {
    Q_OBJECT
public:
    SubtitleView(QWidget *parent = 0);
    ~SubtitleView();
    auto setModels(const QVector<SubCompModel*> &model) -> void;
private:
    auto setTimeVisible(bool visible) -> void;
    auto setAutoScrollEnabled(bool enabled) -> void;
    auto showEvent(QShowEvent *event) -> void;
    auto updateModels() -> void;
    class CompView;
    struct Data;
    Data *d;
};

#endif // SUBTITLEVIEW_HPP
