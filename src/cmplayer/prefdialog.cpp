#include "prefdialog.hpp"
#include "trayicon.hpp"
#include "deintinfo.hpp"
#include "dialogs.hpp"
#include "ui_prefdialog.h"
#include "info.hpp"
#include "app.hpp"
#include "pref.hpp"
#include "rootmenu.hpp"
#include "skin.hpp"
#include "hwacc.hpp"
#include <array>
#include "hwacc_vaapi.hpp"

#ifdef None
#undef None
#endif

#include "simplelistmodel.hpp"

class MrlStatePropertyListModel : public SimpleListModel<MrlStateProperty> {
public:
	MrlStatePropertyListModel() {
		setCheckable(0, true);
	}
	Qt::ItemFlags flags(int row, int column) const {
		return Super::flags(row, column) | Qt::ItemIsUserCheckable;
	}
	QVariant displayData(int row, int /*column*/) const {
		return at(row).info();
	}
private:
};

// from clementine's preferences dialog
typedef QDialogButtonBox DBB;

static const int CategoryRole = Qt::UserRole + 1;
static const int WidgetRole = Qt::UserRole + CategoryRole;

class PrefOpenMediaGroup : public QGroupBox {
	Q_DECLARE_TR_FUNCTIONS(PrefOpenMediaGroup)
public:
	PrefOpenMediaGroup(const QString &title, QWidget *parent)
	: QGroupBox(title, parent) {
		auto layout = new QVBoxLayout(this);
		start = new QCheckBox(tr("Start the playback"), this);
		playlist = new EnumComboBox<PlaylistBehaviorWhenOpenMedia>(this);
		layout->addWidget(start);
		layout->addWidget(playlist);
		auto vbox = static_cast<QVBoxLayout*>(parent->layout());
		vbox->insertWidget(vbox->count()-1, this);
	}
	void setValue(const Pref::OpenMedia &open) {
		start->setChecked(open.start_playback);
		playlist->setCurrentValue(open.playlist_behavior);
	}
	Pref::OpenMedia value() const {return Pref::OpenMedia(start->isChecked(), playlist->currentValue());}
private:
	QCheckBox *start;
	EnumComboBox<PlaylistBehaviorWhenOpenMedia> *playlist;
};

template <typename Enum>
class PrefMouseGroup : public QGroupBox {
public:
	typedef ::KeyModifier Modifier;
	typedef EnumComboBox<Enum> ComboBox;
	typedef ActionEnumInfo<Enum> ActionInfo;
	typedef Pref::KeyModifierMap<Enum> ActionMap;
	PrefMouseGroup(QVBoxLayout *form, QWidget *parent = 0)
	: QGroupBox(parent), form(form) {
		mods << Modifier::None << Modifier::Ctrl << Modifier::Shift << Modifier::Alt;
		QGridLayout *grid = new QGridLayout(this);
		grid->setMargin(0);
		for (int i=0; i<mods.size(); ++i) {
			ComboBox *combo = new ComboBox(this);
			QCheckBox *check = new QCheckBox(this);
			combos.append(combo);
			checks.append(check);
			if (mods[i] != Modifier::None)
				check->setText(QKeySequence((int)mods[i]).toString(QKeySequence::NativeText));
			grid->addWidget(check, i, 0, 1, 1);
			grid->addWidget(combo, i, 1, 1, 1);
			combo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
			combo->setEnabled(check->isChecked());
			connect(check, SIGNAL(toggled(bool)), combo, SLOT(setEnabled(bool)));
		}
		form->addWidget(this);
	}
	void setValues(const ActionMap &map) {
		for (int i=0; i<mods.size(); ++i) {
			const ActionInfo info = map[mods[i]];
			const int idx = combos[i]->findData((int)info.action);
			Q_ASSERT(idx != -1);
			combos[i]->setCurrentIndex(idx);
			checks[i]->setChecked(info.enabled);
		}
	}
	ActionMap values() const {
		ActionMap map;
		for (int i=0; i<mods.size(); ++i) {
			ActionInfo &info = map[mods[i]];
			info.enabled = checks[i]->isChecked();
			info.action = combos[i]->currentValue();
		}
		return map;
	}
	void retranslate(const QString &name) {setTitle(name);}
private:
	QList<ComboBox*> combos;
	QList<Modifier> mods;
	QList<QCheckBox*> checks;
	QVBoxLayout *form;
};

class PrefDialog::MenuTreeItem : public QTreeWidgetItem {
public:
	enum Column {Discription = 0, Shortcut1, Shortcut2, Shortcut3, Shortcut4, Id};
	bool isMenu() const{return m_action->menu() != 0;}
	bool isSeparator() const{return m_action->isSeparator();}
	QKeySequence shortcut(int i) const {return m_shortcuts[i];}
	void setShortcut(int idx, const QKeySequence &shortcut) {
		m_shortcuts[idx] = shortcut;
		setText(idx + Shortcut1, shortcut.toString(QKeySequence::NativeText));
	}
	void setShortcuts(const QList<QKeySequence> &keys) {
		for (int i = 0; i<(int)m_shortcuts.size(); ++i) {
			m_shortcuts[i] = (i < keys.size()) ? keys[i] : QKeySequence();
			setText(i+1, m_shortcuts[i].toString(QKeySequence::NativeText));
		}
	}
	QList<QKeySequence> shortcuts() const {
		QList<QKeySequence> shortcuts;
		for (auto key : m_shortcuts) {
			if (!key.isEmpty())
				shortcuts << key;
		}
		return shortcuts;
	}
	QString id() const {return m_id;}
	static QList<MenuTreeItem*> makeRoot(QTreeWidget *parent) {
		RootMenu &root = RootMenu::instance();
		QList<MenuTreeItem*> items;
		auto item = create(&root, items);
		parent->addTopLevelItems(item->takeChildren());
		delete item;
		return items;
	}
private:
	static MenuTreeItem *create(Menu *menu, QList<MenuTreeItem*> &items) {
		RootMenu &root = RootMenu::instance();
		QList<QAction*> actions = menu->actions();
		QList<QTreeWidgetItem*> children;
		for (int i=0; i<actions.size(); ++i) {
			const auto action = actions[i];
			const auto id = root.longId(action);
			if (!id.isEmpty()) {
				if (action->menu()) {
					Q_ASSERT(qobject_cast<Menu*>(action->menu()) != 0);
					if (auto child = create(static_cast<Menu*>(action->menu()), items))
						children.push_back(child);
				} else {
					auto child = new MenuTreeItem(action, 0);
					child->setText(Id, child->m_id = id);
					items.push_back(child);
					children.push_back(child);
				}
			}
		}
		if (children.isEmpty())
			return 0;
		MenuTreeItem *item = new MenuTreeItem(menu, 0);
		item->addChildren(children);
		return item;
	}
	MenuTreeItem(Menu *menu, MenuTreeItem *parent)
	: QTreeWidgetItem(parent), m_action(menu->menuAction()) {
		setText(Discription, menu->title());
	}
	MenuTreeItem(QAction *action, MenuTreeItem *parent)
	: QTreeWidgetItem(parent), m_action(action) {
		Q_ASSERT(action->menu() == 0);
		setText(Discription, m_action->text());
//		m_shortcuts.resize(4);
	}
	QAction *m_action; QString m_id;
	std::array<QKeySequence, 4> m_shortcuts;
};


class PrefDialog::Delegate : public QStyledItemDelegate {
public:
	Delegate(QObject* parent): QStyledItemDelegate(parent) {}
	QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const {
		QSize size = QStyledItemDelegate::sizeHint(option, index);
		if (index.data(CategoryRole).toBool())
			size.rheight() *= 2;
		return size;
	}
	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
		if (index.data(CategoryRole).toBool())
			drawHeader(painter, option.rect, option.font, option.palette, index.data().toString());
		else
			QStyledItemDelegate::paint(painter, option, index);
	}
private:
	static const int kBarThickness = 2;
	static const int kBarMarginTop = 3;
	static void drawHeader(QPainter *painter, const QRect &rect, const QFont &font, const QPalette &palette, const QString &text) {
	  painter->save();

	  // Bold font
	  QFont bold_font(font);
	  bold_font.setBold(true);
	  QFontMetrics metrics(bold_font);

	  QRect text_rect(rect);
	  text_rect.setHeight(metrics.height());
	  text_rect.moveTop(rect.top() + (rect.height() - text_rect.height() - kBarThickness - kBarMarginTop) / 2);
	  text_rect.setLeft(text_rect.left() + 3);

	  // Draw text
	  painter->setFont(bold_font);
	  painter->drawText(text_rect, text);

	  // Draw a line underneath
	  const QPoint start(rect.left(), text_rect.bottom() + kBarMarginTop);
	  const QPoint end(rect.right(), start.y());

	  painter->setRenderHint(QPainter::Antialiasing, true);
	  painter->setPen(QPen(palette.color(QPalette::Disabled, QPalette::Text), kBarThickness, Qt::SolidLine, Qt::RoundCap));
	  painter->setOpacity(0.5);
	  painter->drawLine(start, end);

	  painter->restore();
	}
};


/********************************************************************************/


struct PrefDialog::Data {
	Ui::PrefDialog ui;
	QButtonGroup *shortcuts;
	PrefMouseGroup<ClickAction> *dbl, *mdl;
	PrefMouseGroup<WheelAction> *whl;
	QMap<int, QCheckBox*> hwdec;
	QMap<DeintMethod, QCheckBox*> hwdeint;
	PrefOpenMediaGroup *open_media_from_file_manager;
	PrefOpenMediaGroup *open_media_by_drag_and_drop;
	QStringList imports;
	QList<MenuTreeItem*> actionItems;
	DeintWidget *deint_swdec = nullptr, *deint_hwdec = nullptr;
	MrlStatePropertyListModel properties;
	void updateCodecCheckBox() {
		const auto type = static_cast<HwAcc::Type>(ui.hwacc_backend->currentData().toInt());
		const auto codecs = HwAcc::fullCodecList();
		for (const auto codec : codecs) {
			auto box = hwdec[codec];
			const auto supported = HwAcc::supports(type, codec);
			const QString desc = avcodec_descriptor_get(codec)->long_name;
			if (supported)
				box->setText(desc);
			else
				box->setText(desc % _L(" (") % tr("Not supported") % _L(')'));
			box->setEnabled(supported);
		}
	}
};

PrefDialog::PrefDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
	d->ui.setupUi(this);
	d->ui.tree->setItemDelegate(new Delegate(d->ui.tree));
	d->ui.tree->setIconSize(QSize(22, 22));

	connect(d->ui.tree, &QTreeWidget::itemSelectionChanged, [this] () {
		auto items = d->ui.tree->selectedItems();
		if (items.isEmpty())
			return;
		auto item = items.first();
		if (item->data(0, CategoryRole).toBool())
			return;
		d->ui.page_name->setText(item->parent()->text(0) % " > " % item->text(0));
		d->ui.stack->setCurrentWidget(item->data(0, WidgetRole).value<QWidget*>());
	});
	auto addCategory = [this] (const QString &name) {
		auto item = new QTreeWidgetItem;
		item->setText(0, name);
		item->setData(0, CategoryRole, true);
		item->setFlags(Qt::ItemIsEnabled);
		d->ui.tree->invisibleRootItem()->addChild(item);
		item->setExpanded(true);
		return item;
	};

	auto addPage = [this] (const QString &name, QWidget *widget, const QString &icon, QTreeWidgetItem *parent) {
		auto item = new QTreeWidgetItem(parent);
		item->setText(0, name);
		item->setIcon(0, QIcon(icon));
		item->setData(0, CategoryRole, false);
		item->setData(0, WidgetRole, QVariant::fromValue(widget));
		return item;
	};

	auto general = addCategory(tr("General"));
	addPage(tr("Open"), d->ui.open_media, ":/img/document-open-32.png", general)->setSelected(true);
	addPage(tr("Playback"), d->ui.playback, ":/img/media-playback-start-32.png", general);
	addPage(tr("Behaviour"), d->ui.gen_behaviours, ":/img/preferences-system-session-services.png", general);
	addPage(tr("Look"), d->ui.gen_appearance, ":/img/preferences-desktop-theme-32.png", general);
	addPage(tr("Cache"), d->ui.cache, ":/img/preferences-web-browser-cache.png", general);
//	addPage(tr("Advanced"), d->ui.advanced, ":/img/applications-education-miscellaneous-32.png", general);

	auto video = addCategory(tr("Video"));
	addPage(tr("Hardware acceleration"), d->ui.video_hwacc, ":/img/apps-hardware-icon.png", video);
	addPage(tr("Deinterlace"), d->ui.video_deint, ":/img/format-line-spacing-double.png", video);
	addPage(tr("Video filter"), d->ui.video_filter, ":/img/draw-brush.png", video);

	auto audio = addCategory(tr("Audio"));
	addPage(tr("Sound"), d->ui.audio_sound, ":/img/audio-volume-high.png", audio);
	addPage(tr("Audio filter"), d->ui.audio_filter, ":/img/applications-multimedia.png", audio);

	auto subtitle = addCategory(tr("Subtitle"));
	addPage(tr("Load"), d->ui.sub_load, ":/img/application-x-subrip-32.png", subtitle);
	addPage(tr("Appearance"), d->ui.sub_appearance, ":/img/format-text-color-32.png", subtitle);
	addPage(tr("Priority"), d->ui.sub_unified, ":/img/view-sort-descending-32.png", subtitle);

	auto ui = addCategory(tr("User interface"));
	addPage(tr("Keyboard shortcuts"), d->ui.ui_shortcut, ":/img/preferences-desktop-keyboard-32.png", ui);
	addPage(tr("Mouse actions"), d->ui.ui_mouse, ":/img/input-mouse-32.png", ui);
	addPage(tr("Control step"), d->ui.ui_step, ":/img/run-build-32.png", ui);

	d->open_media_from_file_manager = new PrefOpenMediaGroup(tr("Open from file manager"), d->ui.open_media);
	d->open_media_by_drag_and_drop = new PrefOpenMediaGroup(tr("Open by drag-and-drop"), d->ui.open_media);

	auto vbox = new QVBoxLayout;
	vbox->setMargin(0);

	auto backends = HwAcc::availableBackends();
	for (auto type : backends)
		d->ui.hwacc_backend->addItem(HwAcc::backendDescription(type), (int)type);

	const auto codecs = HwAcc::fullCodecList();
	for (const auto codec : codecs)
		vbox->addWidget(d->hwdec[codec] = new QCheckBox);
	d->ui.hwdec_list->setLayout(vbox);

	connect(d->ui.hwacc_backend, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged)
		, [this] () { d->updateCodecCheckBox(); });
	d->updateCodecCheckBox();

	vbox = new QVBoxLayout;
	vbox->setMargin(0);

	d->ui.deint_tabs->addTab(d->deint_swdec = new DeintWidget(DecoderDevice::CPU), tr("For S/W decoding"));
	d->ui.deint_tabs->addTab(d->deint_hwdec = new DeintWidget(DecoderDevice::GPU), tr("For H/W decoding"));
	d->ui.deint_desc->setText(DeintWidget::informations());

	d->ui.sub_ext->addItem(QString(), QString());
	d->ui.sub_ext->addItemTextData(Info::subtitleExt());
	d->ui.window_style->addItemTextData(cApp.availableStyleNames());

	d->dbl = new PrefMouseGroup<ClickAction>(d->ui.ui_mouse_layout);
	d->mdl = new PrefMouseGroup<ClickAction>(d->ui.ui_mouse_layout);
	d->whl = new PrefMouseGroup<WheelAction>(d->ui.ui_mouse_layout);

	d->shortcuts = new QButtonGroup(this);
	d->shortcuts->addButton(d->ui.shortcut1, 0);
	d->shortcuts->addButton(d->ui.shortcut2, 1);
	d->shortcuts->addButton(d->ui.shortcut3, 2);
	d->shortcuts->addButton(d->ui.shortcut4, 3);

	d->actionItems = MenuTreeItem::makeRoot(d->ui.shortcut_tree);

	d->ui.shortcut_tree->header()->resizeSection(0, 200);

	QList<AudioDriver> audioDrivers;
	audioDrivers << AudioDriver::Auto;
#ifdef Q_OS_LINUX
	audioDrivers << AudioDriver::ALSA << AudioDriver::OSS;
#endif
#ifdef Q_OS_MAC
	audioDrivers << AudioDriver::CoreAudio;
#endif
#if HAVE_PORTAUDIO
	audioDrivers << AudioDriver::PortAudio;
#endif
#if HAVE_PULSEAUDIO
	audioDrivers << AudioDriver::PulseAudio;
#endif
#if HAVE_JACK
	audioDrivers << AudioDriver::JACK;
#endif
	for (auto driver : audioDrivers)
		d->ui.audio_driver->addItem(AudioDriverInfo::name(driver), (int)driver);

	d->ui.network_folders->setAddingAndErasingEnabled(true);

	auto checkSubAutoselect = [this] (const QVariant &data) {
		const bool enabled = data.toInt() == SubtitleAutoselect::Matched;
		d->ui.sub_ext_label->setEnabled(enabled);
		d->ui.sub_ext->setEnabled(enabled);
	};

	d->ui.sub_priority->setAddingAndErasingEnabled(true);
	d->ui.sub_priority->setChangingOrderEnabled(true);
	checkSubAutoselect(d->ui.sub_autoselect->currentData());

	auto updateSkinPath = [this] (int idx) {
		if (idx >= 0) {
			const auto name = d->ui.skin_name->itemText(idx);
			const auto skin = Skin::source(name);
			d->ui.skin_path->setText(skin.absolutePath());
		}
	};

	d->ui.skin_name->addItems(Skin::names(true));
	updateSkinPath(d->ui.skin_name->currentIndex());

	connect(d->ui.skin_name, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), updateSkinPath);
	connect(d->ui.sub_autoselect, &DataComboBox::currentDataChanged, checkSubAutoselect);
	connect(d->ui.sub_autoload, &DataComboBox::currentDataChanged, checkSubAutoselect);
	connect(d->shortcuts, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked), [this] (int idx) {
		auto item = static_cast<MenuTreeItem*>(d->ui.shortcut_tree->currentItem());
		if (item && !item->isMenu()) {
			GetShortcutDialog dlg(item->shortcut(idx), this);
			if (dlg.exec())
				item->setShortcut(idx, dlg.shortcut());
		}
	});
	connect(d->ui.shortcut_tree, &QTreeWidget::currentItemChanged, [this] (QTreeWidgetItem *it) {
		MenuTreeItem *item = static_cast<MenuTreeItem*>(it);
		const QList<QAbstractButton*> buttons = d->shortcuts->buttons();
		for (int i=0; i<buttons.size(); ++i)
			buttons[i]->setEnabled(item && !item->isMenu());
	});

	auto onBlurKernelChanged = [this] () {
		d->ui.blur_sum->setText(QString::number(d->ui.blur_kern_c->value() + d->ui.blur_kern_n->value()*4 + d->ui.blur_kern_d->value()*4));
	};
	auto onSharpenKernelChanged = [this] () {
		d->ui.sharpen_sum->setText(QString::number(d->ui.sharpen_kern_c->value() + d->ui.sharpen_kern_n->value()*4 + d->ui.sharpen_kern_d->value()*4));
	};
	using ValueChanged = void(QSpinBox::*)(int);
	connect(d->ui.blur_kern_c, static_cast<ValueChanged>(&QSpinBox::valueChanged), onBlurKernelChanged);
	connect(d->ui.blur_kern_n, static_cast<ValueChanged>(&QSpinBox::valueChanged), onBlurKernelChanged);
	connect(d->ui.blur_kern_d, static_cast<ValueChanged>(&QSpinBox::valueChanged), onBlurKernelChanged);

	connect(d->ui.sharpen_kern_c, static_cast<ValueChanged>(&QSpinBox::valueChanged), onSharpenKernelChanged);
	connect(d->ui.sharpen_kern_n, static_cast<ValueChanged>(&QSpinBox::valueChanged), onSharpenKernelChanged);
	connect(d->ui.sharpen_kern_d, static_cast<ValueChanged>(&QSpinBox::valueChanged), onSharpenKernelChanged);

	connect(d->ui.dbb, &DBB::clicked, [this] (QAbstractButton *button) {
		switch (d->ui.dbb->standardButton(button)) {
		case DBB::Ok:		hide();
		case DBB::Apply:	emit applyRequested();		break;
		case DBB::Cancel:	hide();
		case DBB::Reset:	emit resetRequested();		break;
		case DBB::RestoreDefaults:	set(Pref());		break;
		default:										break;
		}
	});

	d->ui.shortcut_preset->addItem(tr("CMPlayer"), Pref::CMPlayer);
	d->ui.shortcut_preset->addItem(tr("Movist"), Pref::Movist);

	connect(d->ui.load_preset, &QPushButton::clicked, [this] () {
		const int idx = d->ui.shortcut_preset->currentIndex();
		if (idx != -1) {
			const auto preset = static_cast<Pref::ShortcutPreset>(d->ui.shortcut_preset->itemData(idx).toInt());
			setShortcuts(Pref::preset(preset));
		}
	});

	retranslate();
	d->ui.restore_properties->setModel(&d->properties);
	if (!TrayIcon::isAvailable())
		d->ui.system_tray_group->hide();
#ifndef Q_OS_LINUX
	d->ui.use_mpris2->hide();
#endif
#ifndef Q_OS_MAC
	d->ui.lion_style_fullscreen->hide();
#endif

	adjustSize();

	auto group = new QButtonGroup(this);
	group->addButton(d->ui.show_logo);
	group->addButton(d->ui.fill_bg_color);
	group->setExclusive(true);

	cApp.setWindowTitle(this, tr("Preferences"));
}

PrefDialog::~PrefDialog() {
	delete d;
}


void PrefDialog::retranslate() {
	d->dbl->retranslate(tr("Double Click"));
	d->mdl->retranslate(tr("Middle Click"));
	d->whl->retranslate(tr("Wheel Scroll"));
	d->ui.sub_ext->setItemText(0, tr("All"));
	d->ui.dbb->button(DBB::Ok)->setText(tr("Ok"));
	d->ui.dbb->button(DBB::Cancel)->setText(tr("Cancel"));
	d->ui.dbb->button(DBB::Apply)->setText(tr("Apply"));
	d->ui.dbb->button(DBB::RestoreDefaults)->setText(tr("Restore Defaults"));
	d->ui.dbb->button(DBB::Reset)->setText(tr("Reset"));
	d->properties.setList(MrlState::restorableProperties());
}

template<class T>
static void setHw(QGroupBox *group, bool enabled, QMap<T, QCheckBox*> &map, const QList<T> &keys) {
	group->setChecked(enabled);
	for (auto key : keys) {
		if (auto ch = map.value(key))
			ch->setChecked(true);
	}
}

template<class T>
static void getHw(bool &enabled, QGroupBox *group, QList<T> &keys, const QMap<T, QCheckBox*> &map) {
	enabled = group->isChecked();
	keys.clear();
	for (auto it = map.begin(); it != map.end(); ++it) {
		if (*it && (*it)->isChecked())
			keys << it.key();
	}
}

void PrefDialog::set(const Pref &p) {
	d->open_media_from_file_manager->setValue(p.open_media_from_file_manager);
	d->open_media_by_drag_and_drop->setValue(p.open_media_by_drag_and_drop);

	d->ui.use_mpris2->setChecked(p.use_mpris2);
	d->ui.fit_to_video->setChecked(p.fit_to_video);
	d->ui.show_osd_on_action->setChecked(p.show_osd_on_action);
	d->ui.show_osd_on_resized->setChecked(p.show_osd_on_resized);
	d->ui.pause_minimized->setChecked(p.pause_minimized);
	d->ui.pause_video_only->setChecked(p.pause_video_only);
	d->ui.remember_stopped->setChecked(p.remember_stopped);
	d->ui.ask_record_found->setChecked(p.ask_record_found);
	d->ui.enable_generate_playlist->setChecked(p.enable_generate_playist);
	d->ui.generate_playlist->setCurrentValue(p.generate_playlist);
	d->ui.hide_cursor->setChecked(p.hide_cursor);
	d->ui.hide_cursor_fs_only->setChecked(p.hide_cursor_fs_only);
	d->ui.hide_delay->setValue(p.hide_cursor_delay/1000);
	d->ui.disable_screensaver->setChecked(p.disable_screensaver);
	d->ui.remember_image->setChecked(p.remember_image);
	d->ui.image_duration->setValue(p.image_duration/1000);
	d->ui.lion_style_fullscreen->setChecked(p.lion_style_fullscreen);
	if (p.show_logo)
		d->ui.show_logo->setChecked(true);
	else
		d->ui.fill_bg_color->setChecked(true);
	d->ui.bg_color->setColor(p.bg_color, false);

	d->ui.hwacc_backend->setCurrentIndex(d->ui.hwacc_backend->findData(p.hwaccel_backend));
	setHw(d->ui.enable_hwdec, p.enable_hwaccel, d->hwdec, p.hwaccel_codecs);
	d->deint_swdec->set(p.deint_swdec);
	d->deint_hwdec->set(p.deint_hwdec);

	d->ui.normalizer_silence->setValue(p.normalizer_silence);
	d->ui.normalizer_target->setValue(p.normalizer_target);
	d->ui.normalizer_min->setValue(p.normalizer_min*100.0);
	d->ui.normalizer_max->setValue(p.normalizer_max*100.0);
	d->ui.normalizer_length->setValue(p.normalizer_length);
	d->ui.channel_manipulation->setMap(p.channel_manipulation);

	d->ui.blur_kern_c->setValue(p.blur_kern_c);
	d->ui.blur_kern_n->setValue(p.blur_kern_n);
	d->ui.blur_kern_d->setValue(p.blur_kern_d);
	d->ui.sharpen_kern_c->setValue(p.sharpen_kern_c);
	d->ui.sharpen_kern_n->setValue(p.sharpen_kern_n);
	d->ui.sharpen_kern_d->setValue(p.sharpen_kern_d);

	d->ui.sub_enable_autoload->setChecked(p.sub_enable_autoload);
	d->ui.sub_enable_autoselect->setChecked(p.sub_enable_autoselect);
	d->ui.sub_autoload->setCurrentValue(p.sub_autoload);
	d->ui.sub_autoselect->setCurrentValue(p.sub_autoselect);
	d->ui.sub_ext->setCurrentData(p.sub_ext);
	d->ui.sub_enc->setEncoding(p.sub_enc);
	d->ui.sub_enc_autodetection->setChecked(p.sub_enc_autodetection);
	d->ui.sub_enc_accuracy->setValue(p.sub_enc_accuracy);
	d->ui.sub_font_family->setCurrentFont(p.sub_style.font.family());
	d->ui.sub_font_option->set(p.sub_style.font.qfont);
	d->ui.sub_font_color->setColor(p.sub_style.font.color, false);
	d->ui.sub_outline->setChecked(p.sub_style.outline.enabled);
	d->ui.sub_outline_color->setColor(p.sub_style.outline.color, false);
	d->ui.sub_outline_width->setValue(p.sub_style.outline.width*100.0);
	d->ui.sub_font_scale->setCurrentValue(p.sub_style.font.scale);
	d->ui.sub_font_size->setValue(p.sub_style.font.size*100.0);
	d->ui.sub_shadow->setChecked(p.sub_style.shadow.enabled);
	d->ui.sub_shadow_color->setColor(p.sub_style.shadow.color, false);
	d->ui.sub_shadow_opacity->setValue(p.sub_style.shadow.color.alphaF()*100.0);
	d->ui.sub_shadow_offset_x->setValue(p.sub_style.shadow.offset.x()*100.0);
	d->ui.sub_shadow_offset_y->setValue(p.sub_style.shadow.offset.y()*100.0);
	d->ui.sub_shadow_blur->setChecked(p.sub_style.shadow.blur);
	d->ui.sub_bbox->setChecked(p.sub_style.bbox.enabled);
	d->ui.sub_bbox_color->setColor(p.sub_style.bbox.color, false);
	d->ui.sub_bbox_opacity->setValue(p.sub_style.bbox.color.alphaF()*100.0);
	d->ui.sub_bbox_hpadding->setValue(p.sub_style.bbox.padding.x()*100.0);
	d->ui.sub_bbox_vpadding->setValue(p.sub_style.bbox.padding.y()*100.0);
	d->ui.sub_spacing_line->setValue(p.sub_style.spacing.line*100.0);
	d->ui.sub_spacing_paragraph->setValue(p.sub_style.spacing.paragraph*100.0);
	d->ui.ms_per_char->setValue(p.ms_per_char);
	d->ui.sub_priority->setValues(p.sub_priority);

	d->ui.single_app->setChecked(cApp.isUnique());
	d->ui.window_style->setCurrentText(cApp.styleName(), Qt::MatchFixedString);
	d->ui.enable_system_tray->setChecked(p.enable_system_tray);
	d->ui.hide_rather_close->setChecked(p.hide_rather_close);

	d->dbl->setValues(p.double_click_map);
	d->mdl->setValues(p.middle_click_map);
	d->whl->setValues(p.wheel_scroll_map);
	d->ui.ui_mouse_invert_wheel->setChecked(p.invert_wheel);

	d->ui.seek_step1->setValue(p.seek_step1/1000);
	d->ui.seek_step2->setValue(p.seek_step2/1000);
	d->ui.seek_step3->setValue(p.seek_step3/1000);
	d->ui.speed_step->setValue(p.speed_step);
	d->ui.brightness_step->setValue(p.brightness_step);
	d->ui.contrast_step->setValue(p.contrast_step);
	d->ui.saturation_step->setValue(p.saturation_step);
	d->ui.hue_step->setValue(p.hue_step);
	d->ui.volume_step->setValue(p.volume_step);
	d->ui.amp_step->setValue(p.amp_step);
	d->ui.sub_pos_step->setValue(p.sub_pos_step);
	d->ui.sub_sync_step->setValue(p.sub_sync_step*0.001);
	d->ui.audio_sync_step->setValue(p.audio_sync_step*0.001);

	d->ui.locale->setCurrentLocale(p.locale);
	d->ui.skin_name->setCurrentText(p.skin_name);

	d->ui.audio_driver->setCurrentData((int)p.audio_driver);
	d->ui.clipping_method->setCurrentValue(p.clipping_method);

	d->ui.cache_local->setValue(p.cache_local);
	d->ui.cache_network->setValue(p.cache_network);
	d->ui.cache_disc->setValue(p.cache_disc);
	d->ui.cache_min_playback->setValue(p.cache_min_playback);
	d->ui.cache_min_seeking->setValue(p.cache_min_seeking);
	d->ui.network_folders->setValues(p.network_folders);

	setShortcuts(p.shortcuts);

	QVector<bool> restores(d->properties.size(), false);
	for (int i=0; i<d->properties.size(); ++i)
		restores[i] = p.restore_properties.contains(d->properties.at(i).property());
	d->properties.setChecked(0, restores);
}

void PrefDialog::setShortcuts(const Shortcuts &shortcuts) {
	for (auto item : d->actionItems)
		item->setShortcuts(shortcuts[item->id()]);
}

void PrefDialog::get(Pref &p) {
	p.open_media_from_file_manager = d->open_media_from_file_manager->value();
	p.open_media_by_drag_and_drop = d->open_media_by_drag_and_drop->value();

	p.use_mpris2 = d->ui.use_mpris2->isChecked();
	p.fit_to_video = d->ui.fit_to_video->isChecked();
	p.show_osd_on_action = d->ui.show_osd_on_action->isChecked();
	p.show_osd_on_resized = d->ui.show_osd_on_resized->isChecked();
	p.pause_minimized = d->ui.pause_minimized->isChecked();
	p.pause_video_only = d->ui.pause_video_only->isChecked();
	p.remember_stopped = d->ui.remember_stopped->isChecked();
	p.ask_record_found = d->ui.ask_record_found->isChecked();
	p.enable_generate_playist = d->ui.enable_generate_playlist->isChecked();
	p.generate_playlist = d->ui.generate_playlist->currentValue();
	p.hide_cursor = d->ui.hide_cursor->isChecked();
	p.hide_cursor_fs_only = d->ui.hide_cursor_fs_only->isChecked();
	p.hide_cursor_delay = d->ui.hide_delay->value()*1000;
	p.disable_screensaver = d->ui.disable_screensaver->isChecked();
	p.remember_image = d->ui.remember_image->isChecked();
	p.image_duration = qRound(d->ui.image_duration->value()*1000.0);
	p.lion_style_fullscreen = d->ui.lion_style_fullscreen->isChecked();
	p.show_logo = d->ui.show_logo->isChecked();
	p.bg_color = d->ui.bg_color->color();

	p.hwaccel_backend = HwAcc::Type(d->ui.hwacc_backend->currentData().toInt());
	getHw(p.enable_hwaccel, d->ui.enable_hwdec, p.hwaccel_codecs, d->hwdec);
	p.deint_swdec = d->deint_swdec->get();
	p.deint_hwdec = d->deint_hwdec->get();

	p.blur_kern_c = d->ui.blur_kern_c->value();
	p.blur_kern_n = d->ui.blur_kern_n->value();
	p.blur_kern_d = d->ui.blur_kern_d->value();
	p.sharpen_kern_c = d->ui.sharpen_kern_c->value();
	p.sharpen_kern_n = d->ui.sharpen_kern_n->value();
	p.sharpen_kern_d = d->ui.sharpen_kern_d->value();

	p.normalizer_target = d->ui.normalizer_target->value();
	p.normalizer_silence = d->ui.normalizer_silence->value();
	p.normalizer_min = d->ui.normalizer_min->value()/100.0;
	p.normalizer_max = d->ui.normalizer_max->value()/100.0;
	p.normalizer_length = d->ui.normalizer_length->value();
	p.channel_manipulation = d->ui.channel_manipulation->map();

	p.sub_enable_autoload = d->ui.sub_enable_autoload->isChecked();
	p.sub_enable_autoselect = d->ui.sub_enable_autoselect->isChecked();
	p.sub_autoload = d->ui.sub_autoload->currentValue();
	p.sub_autoselect = d->ui.sub_autoselect->currentValue();
	p.sub_ext = d->ui.sub_ext->currentData().toString();
	p.sub_enc = d->ui.sub_enc->encoding();
	p.sub_enc_autodetection = d->ui.sub_enc_autodetection->isChecked();
	p.sub_enc_accuracy = d->ui.sub_enc_accuracy->value();
	p.sub_style.font.setFamily(d->ui.sub_font_family->currentFont().family());
	d->ui.sub_font_option->apply(p.sub_style.font.qfont);
	p.sub_style.font.color = d->ui.sub_font_color->color();
	p.sub_style.font.scale = d->ui.sub_font_scale->currentValue();
	p.sub_style.font.size = d->ui.sub_font_size->value()/100.0;
	p.sub_style.outline.enabled = d->ui.sub_outline->isChecked();
	p.sub_style.outline.color = d->ui.sub_outline_color->color();
	p.sub_style.outline.width = d->ui.sub_outline_width->value()/100.0;
	p.sub_style.shadow.enabled = d->ui.sub_shadow->isChecked();
	p.sub_style.shadow.color = d->ui.sub_shadow_color->color();
	p.sub_style.shadow.color.setAlphaF(d->ui.sub_shadow_opacity->value()/100.0);
	p.sub_style.shadow.offset.rx() = d->ui.sub_shadow_offset_x->value()/100.0;
	p.sub_style.shadow.offset.ry() = d->ui.sub_shadow_offset_y->value()/100.0;
	p.sub_style.shadow.blur = d->ui.sub_shadow_blur->isChecked();
	p.sub_style.bbox.enabled = d->ui.sub_bbox->isChecked();
	p.sub_style.bbox.color = d->ui.sub_bbox_color->color();
	p.sub_style.bbox.color.setAlphaF(d->ui.sub_bbox_opacity->value()/100.0);
	p.sub_style.bbox.padding.rx() = d->ui.sub_bbox_hpadding->value()/100.0;
	p.sub_style.bbox.padding.ry() = d->ui.sub_bbox_vpadding->value()/100.0;
	p.sub_style.spacing.line = d->ui.sub_spacing_line->value()/100.0;
	p.sub_style.spacing.paragraph = d->ui.sub_spacing_paragraph->value()/100.0;
	p.ms_per_char = d->ui.ms_per_char->value();
	p.sub_priority = d->ui.sub_priority->values();

	cApp.setUnique(d->ui.single_app->isChecked());
	p.locale = d->ui.locale->currentLocale();
	cApp.setStyleName(d->ui.window_style->currentData().toString());
	p.enable_system_tray = d->ui.enable_system_tray->isChecked();
	p.hide_rather_close = d->ui.hide_rather_close->isChecked();

	p.double_click_map = d->dbl->values();
	p.middle_click_map = d->mdl->values();
	p.wheel_scroll_map = d->whl->values();
	p.invert_wheel = d->ui.ui_mouse_invert_wheel->isChecked();

	p.seek_step1 = d->ui.seek_step1->value()*1000;
	p.seek_step2 = d->ui.seek_step2->value()*1000;
	p.seek_step3 = d->ui.seek_step3->value()*1000;
	p.speed_step = d->ui.speed_step->value();
	p.brightness_step = d->ui.brightness_step->value();
	p.contrast_step = d->ui.contrast_step->value();
	p.saturation_step = d->ui.contrast_step->value();
	p.hue_step = d->ui.hue_step->value();
	p.volume_step = d->ui.volume_step->value();
	p.amp_step = d->ui.amp_step->value();
	p.sub_pos_step = d->ui.sub_pos_step->value();
	p.sub_sync_step = qRound(d->ui.sub_sync_step->value()*1000.0);
	p.audio_sync_step = qRound(d->ui.audio_sync_step->value()*1000.0);

	p.skin_name = d->ui.skin_name->currentText();

	p.audio_driver = AudioDriverInfo::from(d->ui.audio_driver->currentData().toInt());
	p.clipping_method = d->ui.clipping_method->currentValue();

	p.cache_local = d->ui.cache_local->value();
	p.cache_network = d->ui.cache_network->value();
	p.cache_disc = d->ui.cache_disc->value();
	p.cache_min_playback = d->ui.cache_min_playback->value();
	p.cache_min_seeking = d->ui.cache_min_seeking->value();
	p.network_folders = d->ui.network_folders->values();

	p.shortcuts.clear();
	for (auto item : d->actionItems) {
		const auto keys = item->shortcuts();
		if (!keys.isEmpty())
			p.shortcuts[item->id()] = keys;
	}

	auto restores = d->properties.checkedList(0);
	p.restore_properties.clear();
	for (int i=0; i<restores.size(); ++i) {
		if (restores[i])
			p.restore_properties.append(d->properties.at(i).property());
	}
}

void PrefDialog::changeEvent(QEvent *event) {
	QWidget::changeEvent(event);
	if (event->type() == QEvent::LanguageChange) {
		d->ui.retranslateUi(this);
		retranslate();
	}
}



void PrefDialog::showEvent(QShowEvent *event) {
	QDialog::showEvent(event);
}
