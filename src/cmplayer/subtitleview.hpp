#ifndef SUBTITLEVIEW_HPP
#define SUBTITLEVIEW_HPP

#include "dialogs.hpp"

class PlayEngine;		class Subtitle;
class SubtitleComponentModel;

class SubtitleView : public ToggleDialog {
	Q_OBJECT
public:
	SubtitleView(QWidget *parent = 0);
	~SubtitleView();
	void setModel(const QVector<SubtitleComponentModel*> &model);
private slots:
	void setTimeVisible(bool visible);
	void setAutoScrollEnabled(bool enabled);
private:
	class CompView;
	struct Data;
	Data *d;
};

#endif // SUBTITLEVIEW_HPP
