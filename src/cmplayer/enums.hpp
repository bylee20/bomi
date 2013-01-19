#ifndef ENUMS_HPP
#define ENUMS_HPP

#include <QtCore/QCoreApplication>
#include <QtCore/QMap>

class EnumClass {
	Q_DECLARE_TR_FUNCTIONS(EnumClass)
public:
	EnumClass(const EnumClass &rhs): m_id(rhs.m_id) {}
	virtual ~EnumClass() {}
	EnumClass &operator = (const EnumClass &rhs) {m_id = rhs.m_id; return *this;}
	bool operator == (const EnumClass &rhs) const {return m_id == rhs.m_id;}
	bool operator != (const EnumClass &rhs) const {return m_id != rhs.m_id;}
	bool operator < (const EnumClass &rhs) const {return m_id < rhs.m_id;}
	bool operator == (int rhs) const {return m_id == rhs;}
	bool operator != (int rhs) const {return m_id != rhs;}
	bool operator < (int rhs) const {return m_id < rhs;}
	int id() const {return m_id;}
	virtual QString name() const = 0;
	virtual QString description() const = 0;
	virtual void setById(int id) = 0;
	virtual void setByName(const QString &name) = 0;
protected:
	EnumClass(int id): m_id(id) {}
	void setId(int id) {m_id = id;}
private:
	int m_id = 0;
};

static inline bool operator == (int lhs, const EnumClass &rhs) {return rhs == lhs;}
static inline bool operator != (int lhs, const EnumClass &rhs) {return rhs != lhs;}

namespace Enum {
class StaysOnTop : public EnumClass {
	Q_DECLARE_TR_FUNCTIONS(StaysOnTop)
public:
	typedef QList<StaysOnTop> List;
	static const int count = 3;
	static const StaysOnTop Always;
	static const StaysOnTop Playing;
	static const StaysOnTop Never;

	StaysOnTop(): EnumClass(0) {}
	StaysOnTop(const StaysOnTop &rhs): EnumClass(rhs) {}
	StaysOnTop &operator = (const StaysOnTop &rhs) {setId(rhs.id()); return *this;}
	StaysOnTop &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
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
		if (id == Always.id())
			return QString();
		if (id == Playing.id())
			return QString();
		if (id == Never.id())
			return QString();
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count] = {};
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	StaysOnTop(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = _L(name), id);
		_map.list.append(*this);
	}
	StaysOnTop(int id): EnumClass(id) {}
};
}

namespace Enum {
class SeekingStep : public EnumClass {
	Q_DECLARE_TR_FUNCTIONS(SeekingStep)
public:
	typedef QList<SeekingStep> List;
	static const int count = 3;
	static const SeekingStep Step1;
	static const SeekingStep Step2;
	static const SeekingStep Step3;

	SeekingStep(): EnumClass(0) {}
	SeekingStep(const SeekingStep &rhs): EnumClass(rhs) {}
	SeekingStep &operator = (const SeekingStep &rhs) {setId(rhs.id()); return *this;}
	SeekingStep &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
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
		if (id == Step1.id())
			return QString();
		if (id == Step2.id())
			return QString();
		if (id == Step3.id())
			return QString();
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count] = {};
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	SeekingStep(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = _L(name), id);
		_map.list.append(*this);
	}
	SeekingStep(int id): EnumClass(id) {}
};
}

namespace Enum {
class GeneratePlaylist : public EnumClass {
	Q_DECLARE_TR_FUNCTIONS(GeneratePlaylist)
public:
	typedef QList<GeneratePlaylist> List;
	static const int count = 2;
	static const GeneratePlaylist Similar;
	static const GeneratePlaylist Folder;

	GeneratePlaylist(): EnumClass(0) {}
	GeneratePlaylist(const GeneratePlaylist &rhs): EnumClass(rhs) {}
	GeneratePlaylist &operator = (const GeneratePlaylist &rhs) {setId(rhs.id()); return *this;}
	GeneratePlaylist &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
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
		if (id == Similar.id())
			return tr("Add files which have similar names");
		if (id == Folder.id())
			return tr("Add all files in the same folder");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count] = {};
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	GeneratePlaylist(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = _L(name), id);
		_map.list.append(*this);
	}
	GeneratePlaylist(int id): EnumClass(id) {}
};
}

namespace Enum {
class PlaylistBehaviorWhenOpenMedia : public EnumClass {
	Q_DECLARE_TR_FUNCTIONS(PlaylistBehaviorWhenOpenMedia)
public:
	typedef QList<PlaylistBehaviorWhenOpenMedia> List;
	static const int count = 3;
	static const PlaylistBehaviorWhenOpenMedia AppendToPlaylist;
	static const PlaylistBehaviorWhenOpenMedia ClearAndAppendToPlaylist;
	static const PlaylistBehaviorWhenOpenMedia ClearAndGenerateNewPlaylist;

	PlaylistBehaviorWhenOpenMedia(): EnumClass(0) {}
	PlaylistBehaviorWhenOpenMedia(const PlaylistBehaviorWhenOpenMedia &rhs): EnumClass(rhs) {}
	PlaylistBehaviorWhenOpenMedia &operator = (const PlaylistBehaviorWhenOpenMedia &rhs) {setId(rhs.id()); return *this;}
	PlaylistBehaviorWhenOpenMedia &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
	static bool isCompatible(int id) {return 0 <= id && id < count;}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static PlaylistBehaviorWhenOpenMedia from(const QString &name, const PlaylistBehaviorWhenOpenMedia &def = PlaylistBehaviorWhenOpenMedia()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? PlaylistBehaviorWhenOpenMedia(*it) : def;
	}
	static PlaylistBehaviorWhenOpenMedia from(int id, const PlaylistBehaviorWhenOpenMedia &def = PlaylistBehaviorWhenOpenMedia()) {
		return isCompatible(id) ? PlaylistBehaviorWhenOpenMedia(id) : def;
	}
	static QString description(int id) {
		if (id == AppendToPlaylist.id())
			return tr("Append the open media to the playlist");
		if (id == ClearAndAppendToPlaylist.id())
			return tr("Clear the playlist and append the open media to the playlist");
		if (id == ClearAndGenerateNewPlaylist.id())
			return tr("Clear the playlist and generate new playlist");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count] = {};
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	PlaylistBehaviorWhenOpenMedia(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = _L(name), id);
		_map.list.append(*this);
	}
	PlaylistBehaviorWhenOpenMedia(int id): EnumClass(id) {}
};
}

namespace Enum {
class SubtitleAutoload : public EnumClass {
	Q_DECLARE_TR_FUNCTIONS(SubtitleAutoload)
public:
	typedef QList<SubtitleAutoload> List;
	static const int count = 3;
	static const SubtitleAutoload Matched;
	static const SubtitleAutoload Contain;
	static const SubtitleAutoload Folder;

	SubtitleAutoload(): EnumClass(0) {}
	SubtitleAutoload(const SubtitleAutoload &rhs): EnumClass(rhs) {}
	SubtitleAutoload &operator = (const SubtitleAutoload &rhs) {setId(rhs.id()); return *this;}
	SubtitleAutoload &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
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
		if (id == Matched.id())
			return tr("Subtitles which have the same name as that of playing file");
		if (id == Contain.id())
			return tr("Subtitles whose names contain the name of playing file");
		if (id == Folder.id())
			return tr("All subtitles in the folder where the playing file is located");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count] = {};
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	SubtitleAutoload(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = _L(name), id);
		_map.list.append(*this);
	}
	SubtitleAutoload(int id): EnumClass(id) {}
};
}

namespace Enum {
class SubtitleAutoselect : public EnumClass {
	Q_DECLARE_TR_FUNCTIONS(SubtitleAutoselect)
public:
	typedef QList<SubtitleAutoselect> List;
	static const int count = 4;
	static const SubtitleAutoselect Matched;
	static const SubtitleAutoselect First;
	static const SubtitleAutoselect All;
	static const SubtitleAutoselect EachLanguage;

	SubtitleAutoselect(): EnumClass(0) {}
	SubtitleAutoselect(const SubtitleAutoselect &rhs): EnumClass(rhs) {}
	SubtitleAutoselect &operator = (const SubtitleAutoselect &rhs) {setId(rhs.id()); return *this;}
	SubtitleAutoselect &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
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
		if (id == Matched.id())
			return tr("Subtitle which has the same name as that of playing file");
		if (id == First.id())
			return tr("First subtitle from loaded ones");
		if (id == All.id())
			return tr("All loaded subtitles");
		if (id == EachLanguage.id())
			return tr("Each language subtitle");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count] = {};
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	SubtitleAutoselect(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = _L(name), id);
		_map.list.append(*this);
	}
	SubtitleAutoselect(int id): EnumClass(id) {}
};
}

namespace Enum {
class OsdScalePolicy : public EnumClass {
	Q_DECLARE_TR_FUNCTIONS(OsdScalePolicy)
public:
	typedef QList<OsdScalePolicy> List;
	static const int count = 3;
	static const OsdScalePolicy Width;
	static const OsdScalePolicy Height;
	static const OsdScalePolicy Diagonal;

	OsdScalePolicy(): EnumClass(0) {}
	OsdScalePolicy(const OsdScalePolicy &rhs): EnumClass(rhs) {}
	OsdScalePolicy &operator = (const OsdScalePolicy &rhs) {setId(rhs.id()); return *this;}
	OsdScalePolicy &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
	static bool isCompatible(int id) {return 0 <= id && id < count;}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static OsdScalePolicy from(const QString &name, const OsdScalePolicy &def = OsdScalePolicy()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? OsdScalePolicy(*it) : def;
	}
	static OsdScalePolicy from(int id, const OsdScalePolicy &def = OsdScalePolicy()) {
		return isCompatible(id) ? OsdScalePolicy(id) : def;
	}
	static QString description(int id) {
		if (id == Width.id())
			return tr("Fit to width of video");
		if (id == Height.id())
			return tr("Fit to height of video");
		if (id == Diagonal.id())
			return tr("Fit to diagonal of video");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count] = {};
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	OsdScalePolicy(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = _L(name), id);
		_map.list.append(*this);
	}
	OsdScalePolicy(int id): EnumClass(id) {}
};
}

namespace Enum {
class ClickAction : public EnumClass {
	Q_DECLARE_TR_FUNCTIONS(ClickAction)
public:
	typedef QList<ClickAction> List;
	static const int count = 4;
	static const ClickAction OpenFile;
	static const ClickAction Fullscreen;
	static const ClickAction Pause;
	static const ClickAction Mute;

	ClickAction(): EnumClass(0) {}
	ClickAction(const ClickAction &rhs): EnumClass(rhs) {}
	ClickAction &operator = (const ClickAction &rhs) {setId(rhs.id()); return *this;}
	ClickAction &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
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
		if (id == OpenFile.id())
			return tr("Open a file");
		if (id == Fullscreen.id())
			return tr("Toggle fullscreen mode");
		if (id == Pause.id())
			return tr("Toggle play/pause");
		if (id == Mute.id())
			return tr("Toggle mute/unmute");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count] = {};
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	ClickAction(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = _L(name), id);
		_map.list.append(*this);
	}
	ClickAction(int id): EnumClass(id) {}
};
}

namespace Enum {
class WheelAction : public EnumClass {
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

	WheelAction(): EnumClass(0) {}
	WheelAction(const WheelAction &rhs): EnumClass(rhs) {}
	WheelAction &operator = (const WheelAction &rhs) {setId(rhs.id()); return *this;}
	WheelAction &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
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
		if (id == Seek1.id())
			return tr("Seek playback for step 1");
		if (id == Seek2.id())
			return tr("Seek playback for step 2");
		if (id == Seek3.id())
			return tr("Seek playback for step 3");
		if (id == PrevNext.id())
			return tr("Play previous/next");
		if (id == Volume.id())
			return tr("Volumn up/down");
		if (id == Amp.id())
			return tr("Amp. up/down");
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QString name[count] = {};
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	WheelAction(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = _L(name), id);
		_map.list.append(*this);
	}
	WheelAction(int id): EnumClass(id) {}
};
}

namespace Enum {
class KeyModifier : public EnumClass {
	Q_DECLARE_TR_FUNCTIONS(KeyModifier)
public:
	typedef QList<KeyModifier> List;
	static const int count = 4;
	static const KeyModifier None;
	static const KeyModifier Ctrl;
	static const KeyModifier Shift;
	static const KeyModifier Alt;

	KeyModifier(): EnumClass(Qt::NoModifier) {}
	KeyModifier(const KeyModifier &rhs): EnumClass(rhs) {}
	KeyModifier &operator = (const KeyModifier &rhs) {setId(rhs.id()); return *this;}
	KeyModifier &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
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
		if (id == None.id())
			return QString();
		if (id == Ctrl.id())
			return QString();
		if (id == Shift.id())
			return QString();
		if (id == Alt.id())
			return QString();
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QMap<int, QString> name = {};
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	KeyModifier(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = _L(name), id);
		_map.list.append(*this);
	}
	KeyModifier(int id): EnumClass(id) {}
};
}

namespace Enum {
class Position : public EnumClass {
	Q_DECLARE_TR_FUNCTIONS(Position)
public:
	typedef QList<Position> List;
	static const int count = 9;
	static const Position CC;
	static const Position TL;
	static const Position TC;
	static const Position TR;
	static const Position CL;
	static const Position CR;
	static const Position BL;
	static const Position BC;
	static const Position BR;

	Position(): EnumClass(Qt::AlignVCenter|Qt::AlignHCenter) {}
	Position(const Position &rhs): EnumClass(rhs) {}
	Position &operator = (const Position &rhs) {setId(rhs.id()); return *this;}
	Position &operator = (int rhs) {setById(rhs); return *this;}
	QString name() const {return map().name[id()];}
	QString description() const {return description(id());}
	void setById(int id) {if (isCompatible(id)) setId(id);}
	void setByName(const QString &name) {setId(map().value.value(name, id()));}
	static bool isCompatible(int id) {return map().name.contains(id);}
	static bool isCompatible(const QString &name) {return map().value.contains(name);}
	static Position from(const QString &name, const Position &def = Position()) {
		const QMap<QString, int>::const_iterator it = map().value.find(name);
		return it != map().value.end() ? Position(*it) : def;
	}
	static Position from(int id, const Position &def = Position()) {
		return isCompatible(id) ? Position(id) : def;
	}
	static QString description(int id) {
		if (id == CC.id())
			return QString();
		if (id == TL.id())
			return QString();
		if (id == TC.id())
			return QString();
		if (id == TR.id())
			return QString();
		if (id == CL.id())
			return QString();
		if (id == CR.id())
			return QString();
		if (id == BL.id())
			return QString();
		if (id == BC.id())
			return QString();
		if (id == BR.id())
			return QString();
		return QString();
	}
	static const List &list() {return map().list;}
private:
	struct Map {
		Map() {list.reserve(count);}
		QMap<int, QString> name = {};
		QMap<QString, int> value = {};
		List list = {};
	};
	static Map _map;
	static const Map &map() {return _map;}
	Position(int id, const char *name): EnumClass(id) {
		_map.value.insert(_map.name[id] = _L(name), id);
		_map.list.append(*this);
	}
	Position(int id): EnumClass(id) {}
};
}

#endif
