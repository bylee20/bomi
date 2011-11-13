#ifndef APPSTATE_HPP
#define APPSTATE_HPP

#include "enums.hpp"
#include <QtCore/QStringList>
#include <QtCore/QPointF>
#include "colorproperty.hpp"

class AppState {
public:
// play state
	double speed;

// video state
	double aspect_ratio, crop_ratio;
	int video_effects;
	ColorProperty video_color;
	Enum::Position screen_alignment;
	QPoint screen_offset;
	Enum::Overlay overlay;

// audio state
	double amp;
	int volume;
	bool muted, volume_normalized;

// subtitle state
	double sub_pos;
	int sub_sync_delay;
	bool sub_letterbox, sub_align_top;

// window state
	Enum::StaysOnTop stays_on_top;

// misc
	QString last_open_file;
	bool ask_system_tray;
	QStringList open_url_list;
	QString url_enc;


	void save() const;
	static AppState &get();
private:
	AppState();
};

#endif // APPSTATE_HPP
