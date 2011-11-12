#ifndef ENUMS_HPP
#define ENUMS_HPP

#include <QtCore/QCoreApplication>
#include <QtCore/QMap>

namespace Enum {
class StaysOnTop {
	Q_DECLARE_TR_FUNCTIONS(StaysOnTop)
public:
	typedef QList<StaysOnTop> List;
	static const int count = 3;
	static const StaysOnTop Always;
	static const StaysOnTop Playing;
	static const StaysOnTop Never;

	StaysOnTop(): m_id(0) {}
	StaysOnTop(const StaysOnTop &rhs): m_id(rhs.m_id) {}
	StaysOnTop &operator = (const StaysOnTop &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const StaysOnTop &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const StaysOnTop &rhs) const {return m_id != rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (const StaysOnTop &rhs) const {return m_id < rhs.m_id;}
	int id() const {return m_id;}
	QString name() const {return map().name[m_id];}
	QString description() const {return description(m_id);}
	void set(int id) {if (isCompatible(id)) m_id = id;}
	void set(const QString &name) {m_id = map().value.value(name, m_id);}
	static bool isCompatible(int id) {return 0 <= id && id < count;}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static StaysOnTop from(const QString &name, const StaysOnTop &def = StaysOnTop()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? StaysOnTop(*it) : def;
	}
	static StaysOnTop from(int id, const StaysOnTop &def = StaysOnTop()) {
		return isCompatible(id) ? StaysOnTop(id) : def;
	}
	static QString description(int id) {
		if (id == Always.m_id)
			return QString();
		if (id == Playing.m_id)
			return QString();
		if (id == Never.m_id)
			return QString();
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count];
		QMap<QString, int> value;
		List list;
	};
	static Map _map;
	static const Map &map() {return _map;}
	StaysOnTop(int id, const char *name): m_id(id) {
		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
		_map.list.append(*this);
	}
	StaysOnTop(int id): m_id(id) {}
	int m_id;
};
}
inline bool operator == (int lhs, const Enum::StaysOnTop &rhs) {return lhs == rhs.id();}
inline bool operator != (int lhs, const Enum::StaysOnTop &rhs) {return lhs != rhs.id();}

namespace Enum {
class SeekingStep {
	Q_DECLARE_TR_FUNCTIONS(SeekingStep)
public:
	typedef QList<SeekingStep> List;
	static const int count = 3;
	static const SeekingStep Step1;
	static const SeekingStep Step2;
	static const SeekingStep Step3;

	SeekingStep(): m_id(0) {}
	SeekingStep(const SeekingStep &rhs): m_id(rhs.m_id) {}
	SeekingStep &operator = (const SeekingStep &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const SeekingStep &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const SeekingStep &rhs) const {return m_id != rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (const SeekingStep &rhs) const {return m_id < rhs.m_id;}
	int id() const {return m_id;}
	QString name() const {return map().name[m_id];}
	QString description() const {return description(m_id);}
	void set(int id) {if (isCompatible(id)) m_id = id;}
	void set(const QString &name) {m_id = map().value.value(name, m_id);}
	static bool isCompatible(int id) {return 0 <= id && id < count;}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static SeekingStep from(const QString &name, const SeekingStep &def = SeekingStep()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? SeekingStep(*it) : def;
	}
	static SeekingStep from(int id, const SeekingStep &def = SeekingStep()) {
		return isCompatible(id) ? SeekingStep(id) : def;
	}
	static QString description(int id) {
		if (id == Step1.m_id)
			return QString();
		if (id == Step2.m_id)
			return QString();
		if (id == Step3.m_id)
			return QString();
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count];
		QMap<QString, int> value;
		List list;
	};
	static Map _map;
	static const Map &map() {return _map;}
	SeekingStep(int id, const char *name): m_id(id) {
		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
		_map.list.append(*this);
	}
	SeekingStep(int id): m_id(id) {}
	int m_id;
};
}
inline bool operator == (int lhs, const Enum::SeekingStep &rhs) {return lhs == rhs.id();}
inline bool operator != (int lhs, const Enum::SeekingStep &rhs) {return lhs != rhs.id();}

namespace Enum {
class Overlay {
	Q_DECLARE_TR_FUNCTIONS(Overlay)
public:
	typedef QList<Overlay> List;
	static const int count = 3;
	static const Overlay Auto;
	static const Overlay FramebufferObject;
	static const Overlay Pixmap;

	Overlay(): m_id(0) {}
	Overlay(const Overlay &rhs): m_id(rhs.m_id) {}
	Overlay &operator = (const Overlay &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const Overlay &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const Overlay &rhs) const {return m_id != rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (const Overlay &rhs) const {return m_id < rhs.m_id;}
	int id() const {return m_id;}
	QString name() const {return map().name[m_id];}
	QString description() const {return description(m_id);}
	void set(int id) {if (isCompatible(id)) m_id = id;}
	void set(const QString &name) {m_id = map().value.value(name, m_id);}
	static bool isCompatible(int id) {return 0 <= id && id < count;}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static Overlay from(const QString &name, const Overlay &def = Overlay()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? Overlay(*it) : def;
	}
	static Overlay from(int id, const Overlay &def = Overlay()) {
		return isCompatible(id) ? Overlay(id) : def;
	}
	static QString description(int id) {
		if (id == Auto.m_id)
			return tr("Auto");
		if (id == FramebufferObject.m_id)
			return tr("Framebuffer Object");
		if (id == Pixmap.m_id)
			return tr("Pixmap");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count];
		QMap<QString, int> value;
		List list;
	};
	static Map _map;
	static const Map &map() {return _map;}
	Overlay(int id, const char *name): m_id(id) {
		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
		_map.list.append(*this);
	}
	Overlay(int id): m_id(id) {}
	int m_id;
};
}
inline bool operator == (int lhs, const Enum::Overlay &rhs) {return lhs == rhs.id();}
inline bool operator != (int lhs, const Enum::Overlay &rhs) {return lhs != rhs.id();}

namespace Enum {
class GeneratePlaylist {
	Q_DECLARE_TR_FUNCTIONS(GeneratePlaylist)
public:
	typedef QList<GeneratePlaylist> List;
	static const int count = 3;
	static const GeneratePlaylist Similar;
	static const GeneratePlaylist Folder;
	static const GeneratePlaylist None;

	GeneratePlaylist(): m_id(0) {}
	GeneratePlaylist(const GeneratePlaylist &rhs): m_id(rhs.m_id) {}
	GeneratePlaylist &operator = (const GeneratePlaylist &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const GeneratePlaylist &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const GeneratePlaylist &rhs) const {return m_id != rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (const GeneratePlaylist &rhs) const {return m_id < rhs.m_id;}
	int id() const {return m_id;}
	QString name() const {return map().name[m_id];}
	QString description() const {return description(m_id);}
	void set(int id) {if (isCompatible(id)) m_id = id;}
	void set(const QString &name) {m_id = map().value.value(name, m_id);}
	static bool isCompatible(int id) {return 0 <= id && id < count;}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static GeneratePlaylist from(const QString &name, const GeneratePlaylist &def = GeneratePlaylist()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? GeneratePlaylist(*it) : def;
	}
	static GeneratePlaylist from(int id, const GeneratePlaylist &def = GeneratePlaylist()) {
		return isCompatible(id) ? GeneratePlaylist(id) : def;
	}
	static QString description(int id) {
		if (id == Similar.m_id)
			return tr("Add files which have similar names");
		if (id == Folder.m_id)
			return tr("Add all files in the same folder");
		if (id == None.m_id)
			return tr("Do not add any other files");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count];
		QMap<QString, int> value;
		List list;
	};
	static Map _map;
	static const Map &map() {return _map;}
	GeneratePlaylist(int id, const char *name): m_id(id) {
		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
		_map.list.append(*this);
	}
	GeneratePlaylist(int id): m_id(id) {}
	int m_id;
};
}
inline bool operator == (int lhs, const Enum::GeneratePlaylist &rhs) {return lhs == rhs.id();}
inline bool operator != (int lhs, const Enum::GeneratePlaylist &rhs) {return lhs != rhs.id();}

namespace Enum {
class SubtitleAutoload {
	Q_DECLARE_TR_FUNCTIONS(SubtitleAutoload)
public:
	typedef QList<SubtitleAutoload> List;
	static const int count = 4;
	static const SubtitleAutoload Matched;
	static const SubtitleAutoload Contain;
	static const SubtitleAutoload Folder;
	static const SubtitleAutoload None;

	SubtitleAutoload(): m_id(0) {}
	SubtitleAutoload(const SubtitleAutoload &rhs): m_id(rhs.m_id) {}
	SubtitleAutoload &operator = (const SubtitleAutoload &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const SubtitleAutoload &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const SubtitleAutoload &rhs) const {return m_id != rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (const SubtitleAutoload &rhs) const {return m_id < rhs.m_id;}
	int id() const {return m_id;}
	QString name() const {return map().name[m_id];}
	QString description() const {return description(m_id);}
	void set(int id) {if (isCompatible(id)) m_id = id;}
	void set(const QString &name) {m_id = map().value.value(name, m_id);}
	static bool isCompatible(int id) {return 0 <= id && id < count;}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static SubtitleAutoload from(const QString &name, const SubtitleAutoload &def = SubtitleAutoload()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? SubtitleAutoload(*it) : def;
	}
	static SubtitleAutoload from(int id, const SubtitleAutoload &def = SubtitleAutoload()) {
		return isCompatible(id) ? SubtitleAutoload(id) : def;
	}
	static QString description(int id) {
		if (id == Matched.m_id)
			return tr("Subtitles which have the same name as that of playing file");
		if (id == Contain.m_id)
			return tr("Subtitles whose names contain the name of playing file");
		if (id == Folder.m_id)
			return tr("All subtitles in the folder where the playing file is located");
		if (id == None.m_id)
			return tr("Do not load any other subtitles");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count];
		QMap<QString, int> value;
		List list;
	};
	static Map _map;
	static const Map &map() {return _map;}
	SubtitleAutoload(int id, const char *name): m_id(id) {
		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
		_map.list.append(*this);
	}
	SubtitleAutoload(int id): m_id(id) {}
	int m_id;
};
}
inline bool operator == (int lhs, const Enum::SubtitleAutoload &rhs) {return lhs == rhs.id();}
inline bool operator != (int lhs, const Enum::SubtitleAutoload &rhs) {return lhs != rhs.id();}

namespace Enum {
class SubtitleAutoselect {
	Q_DECLARE_TR_FUNCTIONS(SubtitleAutoselect)
public:
	typedef QList<SubtitleAutoselect> List;
	static const int count = 4;
	static const SubtitleAutoselect Matched;
	static const SubtitleAutoselect First;
	static const SubtitleAutoselect All;
	static const SubtitleAutoselect EachLanguage;

	SubtitleAutoselect(): m_id(0) {}
	SubtitleAutoselect(const SubtitleAutoselect &rhs): m_id(rhs.m_id) {}
	SubtitleAutoselect &operator = (const SubtitleAutoselect &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const SubtitleAutoselect &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const SubtitleAutoselect &rhs) const {return m_id != rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (const SubtitleAutoselect &rhs) const {return m_id < rhs.m_id;}
	int id() const {return m_id;}
	QString name() const {return map().name[m_id];}
	QString description() const {return description(m_id);}
	void set(int id) {if (isCompatible(id)) m_id = id;}
	void set(const QString &name) {m_id = map().value.value(name, m_id);}
	static bool isCompatible(int id) {return 0 <= id && id < count;}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static SubtitleAutoselect from(const QString &name, const SubtitleAutoselect &def = SubtitleAutoselect()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? SubtitleAutoselect(*it) : def;
	}
	static SubtitleAutoselect from(int id, const SubtitleAutoselect &def = SubtitleAutoselect()) {
		return isCompatible(id) ? SubtitleAutoselect(id) : def;
	}
	static QString description(int id) {
		if (id == Matched.m_id)
			return tr("Subtitle which has the same name as that of playing file");
		if (id == First.m_id)
			return tr("First subtitle from loaded ones");
		if (id == All.m_id)
			return tr("All loaded subtitles");
		if (id == EachLanguage.m_id)
			return tr("Each language subtitle");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count];
		QMap<QString, int> value;
		List list;
	};
	static Map _map;
	static const Map &map() {return _map;}
	SubtitleAutoselect(int id, const char *name): m_id(id) {
		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
		_map.list.append(*this);
	}
	SubtitleAutoselect(int id): m_id(id) {}
	int m_id;
};
}
inline bool operator == (int lhs, const Enum::SubtitleAutoselect &rhs) {return lhs == rhs.id();}
inline bool operator != (int lhs, const Enum::SubtitleAutoselect &rhs) {return lhs != rhs.id();}

namespace Enum {
class OsdAutoSize {
	Q_DECLARE_TR_FUNCTIONS(OsdAutoSize)
public:
	typedef QList<OsdAutoSize> List;
	static const int count = 3;
	static const OsdAutoSize Width;
	static const OsdAutoSize Height;
	static const OsdAutoSize Diagonal;

	OsdAutoSize(): m_id(0) {}
	OsdAutoSize(const OsdAutoSize &rhs): m_id(rhs.m_id) {}
	OsdAutoSize &operator = (const OsdAutoSize &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const OsdAutoSize &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const OsdAutoSize &rhs) const {return m_id != rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (const OsdAutoSize &rhs) const {return m_id < rhs.m_id;}
	int id() const {return m_id;}
	QString name() const {return map().name[m_id];}
	QString description() const {return description(m_id);}
	void set(int id) {if (isCompatible(id)) m_id = id;}
	void set(const QString &name) {m_id = map().value.value(name, m_id);}
	static bool isCompatible(int id) {return 0 <= id && id < count;}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static OsdAutoSize from(const QString &name, const OsdAutoSize &def = OsdAutoSize()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? OsdAutoSize(*it) : def;
	}
	static OsdAutoSize from(int id, const OsdAutoSize &def = OsdAutoSize()) {
		return isCompatible(id) ? OsdAutoSize(id) : def;
	}
	static QString description(int id) {
		if (id == Width.m_id)
			return tr("Fit to width of video");
		if (id == Height.m_id)
			return tr("Fit to height of video");
		if (id == Diagonal.m_id)
			return tr("Fit to diagonal of video");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count];
		QMap<QString, int> value;
		List list;
	};
	static Map _map;
	static const Map &map() {return _map;}
	OsdAutoSize(int id, const char *name): m_id(id) {
		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
		_map.list.append(*this);
	}
	OsdAutoSize(int id): m_id(id) {}
	int m_id;
};
}
inline bool operator == (int lhs, const Enum::OsdAutoSize &rhs) {return lhs == rhs.id();}
inline bool operator != (int lhs, const Enum::OsdAutoSize &rhs) {return lhs != rhs.id();}

namespace Enum {
class ClickAction {
	Q_DECLARE_TR_FUNCTIONS(ClickAction)
public:
	typedef QList<ClickAction> List;
	static const int count = 4;
	static const ClickAction OpenFile;
	static const ClickAction Fullscreen;
	static const ClickAction Pause;
	static const ClickAction Mute;

	ClickAction(): m_id(0) {}
	ClickAction(const ClickAction &rhs): m_id(rhs.m_id) {}
	ClickAction &operator = (const ClickAction &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const ClickAction &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const ClickAction &rhs) const {return m_id != rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (const ClickAction &rhs) const {return m_id < rhs.m_id;}
	int id() const {return m_id;}
	QString name() const {return map().name[m_id];}
	QString description() const {return description(m_id);}
	void set(int id) {if (isCompatible(id)) m_id = id;}
	void set(const QString &name) {m_id = map().value.value(name, m_id);}
	static bool isCompatible(int id) {return 0 <= id && id < count;}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static ClickAction from(const QString &name, const ClickAction &def = ClickAction()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? ClickAction(*it) : def;
	}
	static ClickAction from(int id, const ClickAction &def = ClickAction()) {
		return isCompatible(id) ? ClickAction(id) : def;
	}
	static QString description(int id) {
		if (id == OpenFile.m_id)
			return tr("Open a file");
		if (id == Fullscreen.m_id)
			return tr("Toggle fullscreen mode");
		if (id == Pause.m_id)
			return tr("Toggle play/pause");
		if (id == Mute.m_id)
			return tr("Toggle mute/unmute");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count];
		QMap<QString, int> value;
		List list;
	};
	static Map _map;
	static const Map &map() {return _map;}
	ClickAction(int id, const char *name): m_id(id) {
		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
		_map.list.append(*this);
	}
	ClickAction(int id): m_id(id) {}
	int m_id;
};
}
inline bool operator == (int lhs, const Enum::ClickAction &rhs) {return lhs == rhs.id();}
inline bool operator != (int lhs, const Enum::ClickAction &rhs) {return lhs != rhs.id();}

namespace Enum {
class WheelAction {
	Q_DECLARE_TR_FUNCTIONS(WheelAction)
public:
	typedef QList<WheelAction> List;
	static const int count = 6;
	static const WheelAction Seek1;
	static const WheelAction Seek2;
	static const WheelAction Seek3;
	static const WheelAction PrevNext;
	static const WheelAction Volume;
	static const WheelAction Amp;

	WheelAction(): m_id(0) {}
	WheelAction(const WheelAction &rhs): m_id(rhs.m_id) {}
	WheelAction &operator = (const WheelAction &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const WheelAction &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const WheelAction &rhs) const {return m_id != rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (const WheelAction &rhs) const {return m_id < rhs.m_id;}
	int id() const {return m_id;}
	QString name() const {return map().name[m_id];}
	QString description() const {return description(m_id);}
	void set(int id) {if (isCompatible(id)) m_id = id;}
	void set(const QString &name) {m_id = map().value.value(name, m_id);}
	static bool isCompatible(int id) {return 0 <= id && id < count;}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static WheelAction from(const QString &name, const WheelAction &def = WheelAction()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? WheelAction(*it) : def;
	}
	static WheelAction from(int id, const WheelAction &def = WheelAction()) {
		return isCompatible(id) ? WheelAction(id) : def;
	}
	static QString description(int id) {
		if (id == Seek1.m_id)
			return tr("Seek playback for step 1");
		if (id == Seek2.m_id)
			return tr("Seek playback for step 2");
		if (id == Seek3.m_id)
			return tr("Seek playback for step 3");
		if (id == PrevNext.m_id)
			return tr("Play previous/next");
		if (id == Volume.m_id)
			return tr("Volumn up/down");
		if (id == Amp.m_id)
			return tr("Amp. up/down");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count];
		QMap<QString, int> value;
		List list;
	};
	static Map _map;
	static const Map &map() {return _map;}
	WheelAction(int id, const char *name): m_id(id) {
		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
		_map.list.append(*this);
	}
	WheelAction(int id): m_id(id) {}
	int m_id;
};
}
inline bool operator == (int lhs, const Enum::WheelAction &rhs) {return lhs == rhs.id();}
inline bool operator != (int lhs, const Enum::WheelAction &rhs) {return lhs != rhs.id();}

namespace Enum {
class KeyModifier {
	Q_DECLARE_TR_FUNCTIONS(KeyModifier)
public:
	typedef QList<KeyModifier> List;
	static const int count = 4;
	static const KeyModifier None;
	static const KeyModifier Ctrl;
	static const KeyModifier Shift;
	static const KeyModifier Alt;

	KeyModifier(): m_id(Qt::NoModifier) {}
	KeyModifier(const KeyModifier &rhs): m_id(rhs.m_id) {}
	KeyModifier &operator = (const KeyModifier &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const KeyModifier &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const KeyModifier &rhs) const {return m_id != rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (const KeyModifier &rhs) const {return m_id < rhs.m_id;}
	int id() const {return m_id;}
	QString name() const {return map().name[m_id];}
	QString description() const {return description(m_id);}
	void set(int id) {if (isCompatible(id)) m_id = id;}
	void set(const QString &name) {m_id = map().value.value(name, m_id);}
	static bool isCompatible(int id) {return map().name.contains(id);}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static KeyModifier from(const QString &name, const KeyModifier &def = KeyModifier()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? KeyModifier(*it) : def;
	}
	static KeyModifier from(int id, const KeyModifier &def = KeyModifier()) {
		return isCompatible(id) ? KeyModifier(id) : def;
	}
	static QString description(int id) {
		if (id == None.m_id)
			return QString();
		if (id == Ctrl.m_id)
			return QString();
		if (id == Shift.m_id)
			return QString();
		if (id == Alt.m_id)
			return QString();
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QMap<int, QString> name;
		QMap<QString, int> value;
		List list;
	};
	static Map _map;
	static const Map &map() {return _map;}
	KeyModifier(int id, const char *name): m_id(id) {
		_map.value.insert(_map.name[m_id] = QLatin1String(name), m_id);
		_map.list.append(*this);
	}
	KeyModifier(int id): m_id(id) {}
	int m_id;
};
}
inline bool operator == (int lhs, const Enum::KeyModifier &rhs) {return lhs == rhs.id();}
inline bool operator != (int lhs, const Enum::KeyModifier &rhs) {return lhs != rhs.id();}

#endif
