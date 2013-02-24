#include "prefdialog.hpp"
#include "translator.hpp"
#include "dialogs.hpp"
#include "info.hpp"
#include "app.hpp"
#include "ui_prefdialog.h"
#include "pref.hpp"
#include "rootmenu.hpp"
#include "skin.hpp"
#include "hwacc.hpp"

// from clementine's preferences dialog
typedef QDialogButtonBox DBB;

static const int CategoryRole = Qt::UserRole + 1;
static const int WidgetRole = Qt::UserRole + 1;

class PrefOpenMediaGroup : public QGroupBox {
	Q_DECLARE_TR_FUNCTIONS(PrefOpenMediaGroup)
public:
	PrefOpenMediaGroup(const QString &title, QWidget *parent)
	: QGroupBox(title, parent) {
		auto layout = new QVBoxLayout(this);
		start = new QCheckBox(tr("Start the playback"), this);
		playlist = new EnumComboBox<Enum::PlaylistBehaviorWhenOpenMedia>(this);
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
	EnumComboBox<Enum::PlaylistBehaviorWhenOpenMedia> *playlist;
};

template <typename Enum>
class PrefMouseGroup : public QGroupBox {
public:
	typedef ::Enum::KeyModifier Modifier;
	typedef EnumComboBox<Enum> ComboBox;
	typedef Pref::ActionEnumInfo<Enum> ActionInfo;
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
				check->setText(QKeySequence(mods[i].id()).toString(QKeySequence::NativeText));
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
			const int idx = combos[i]->findData(info.action.id());
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
	enum Column {Discription = 0, Shortcut1, Shortcut2, Shortcut3, Shortcut4};
	bool isMenu() const{return m_action->menu() != 0;}
	bool isSeparator() const{return m_action->isSeparator();}
	const QList<QKeySequence> &shortcuts() const {return m_shortcuts;}
	void setShortcut(Column column, const QKeySequence &shortcut) {
		m_shortcuts[column - 1] = shortcut;
		setText(column, shortcut.toString(QKeySequence::NativeText));
	}
	void applyShortcut() {
		for (int i=0; i<childCount(); ++i)
			static_cast<MenuTreeItem *>(child(i))->applyShortcut();
		m_action->setShortcuts(m_shortcuts);
	}
	static void makeRoot(QTreeWidget *parent) {
		RootMenu &root = cMenu;
		QList<QAction*> actions = root.actions();
		for (int i=0; i<actions.size(); ++i) {
			QAction *const act = actions[i];
			if (act->menu()) {
				Q_ASSERT(qobject_cast<Menu*>(act->menu()) != 0);
				parent->addTopLevelItem(create(static_cast<Menu*>(act->menu())));
			} else if (!root.id(act).isEmpty())
				parent->addTopLevelItem(new MenuTreeItem(act, 0));
		}
	}
private:
	static MenuTreeItem *create(Menu *menu) {
		QList<QAction*> actions = menu->actions();
		QList<QTreeWidgetItem*> children;
		for (int i=0; i<actions.size(); ++i) {
			QAction *const act = actions[i];
			if (act->menu()) {
				Q_ASSERT(qobject_cast<Menu*>(act->menu()) != 0);
				MenuTreeItem *child = create(static_cast<Menu*>(act->menu()));
				if (child)
					children.push_back(child);
			} else if (!menu->id(act).isEmpty())
				children.push_back(new MenuTreeItem(act, 0));
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
		m_shortcuts = m_action->shortcuts();
		while (m_shortcuts.size() < 4)
			m_shortcuts.append(QKeySequence());
		for (int i=0; i<m_shortcuts.size(); ++i)
			setText(i+1, m_shortcuts[i].toString(QKeySequence::NativeText));
	}
	QAction *m_action;
	QList<QKeySequence> m_shortcuts;
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

struct PrefDialog::Data {
	Ui::PrefDialog ui;
	QButtonGroup *shortcuts;
	PrefMouseGroup<Enum::ClickAction> *dbl, *mdl;
	PrefMouseGroup<Enum::WheelAction> *whl;
	QMap<int, QCheckBox*> HwAcc;
	PrefOpenMediaGroup *open_media_from_file_manager;
	PrefOpenMediaGroup *open_media_by_drag_and_drop;
	QStringList imports;
};

PrefDialog::PrefDialog(QWidget *parent)
#ifdef Q_OS_MAC
: QDialog(parent, Qt::Tool)
#else
: QDialog(parent)
#endif
, d(new Data) {
	d->ui.setupUi(this);
	d->ui.tree->setItemDelegate(new Delegate(d->ui.tree));
	d->ui.tree->setIconSize(QSize(32, 32));

	connect(d->ui.tree, SIGNAL(itemSelectionChanged()), this, SLOT(onCategoryChanged()));
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
	auto open = addPage(tr("Open"), d->ui.open_media, ":/img/document-open-32.png", general);
	addPage(tr("Playback"), d->ui.playback, ":/img/media-playback-start-32.png", general);
	addPage(tr("Application"), d->ui.application, ":/img/cmplayer-32.png", general);
	addPage(tr("Advanced"), d->ui.advanced, ":/img/applications-education-miscellaneous-32.png", general);

	auto subtitle = addCategory(tr("Subtitle"));
	addPage(tr("Load"), d->ui.sub_load, ":/img/application-x-subrip-32.png", subtitle);
	addPage(tr("Appearance"), d->ui.sub_appearance, ":/img/format-text-color-32.png", subtitle);
	addPage(tr("Priority"), d->ui.sub_unified, ":/img/view-sort-descending-32.png", subtitle);

	auto ui = addCategory(tr("User interface"));
	addPage(tr("Keyboard shorcuts"), d->ui.ui_shortcut, ":/img/preferences-desktop-keyboard-32.png", ui);
	addPage(tr("Mouse actions"), d->ui.ui_mouse, ":/img/input-mouse-32.png", ui);
	addPage(tr("Control step"), d->ui.ui_step, ":/img/run-build-32.png", ui);
//	addPage(tr("Skin"), d->ui.ui_skin, ":/img/preferences-desktop-theme-32.png", ui);

	open->setSelected(true);

	d->open_media_from_file_manager = new PrefOpenMediaGroup(tr("Open from file manager"), d->ui.open_media);
	d->open_media_by_drag_and_drop = new PrefOpenMediaGroup(tr("Open by drag-and-drop"), d->ui.open_media);

	auto vbox = new QVBoxLayout;
	vbox->setContentsMargins(20, 0, 0, 0);
	const auto codecs = HwAcc::fullCodecList();
	for (const auto codec : codecs) {
		QCheckBox *ch = new QCheckBox;
		const auto supports = HwAcc::supports(codec);
		const auto desc = avcodec_descriptor_get(codec)->long_name;
		if (supports)
			ch->setText(desc);
		else
			ch->setText(_L(desc) % " (" % tr("Not supported") % ')');
		ch->setEnabled(supports);
		vbox->addWidget(ch);
		d->HwAcc[codec] = ch;
	}
	d->ui.HwAcc_list->setLayout(vbox);

	d->ui.sub_ext->addItem(QString(), QString());
	d->ui.sub_ext->addItemTextData(Info::subtitleExt());
	d->ui.locale->addItemData(Translator::availableLocales());
	d->ui.window_style->addItemTextData(cApp.availableStyleNames());

	d->dbl = new PrefMouseGroup<Enum::ClickAction>(d->ui.ui_mouse_layout);
	d->mdl = new PrefMouseGroup<Enum::ClickAction>(d->ui.ui_mouse_layout);
	d->whl = new PrefMouseGroup<Enum::WheelAction>(d->ui.ui_mouse_layout);

	d->shortcuts = new QButtonGroup(this);
	d->shortcuts->addButton(d->ui.shortcut1, MenuTreeItem::Shortcut1);
	d->shortcuts->addButton(d->ui.shortcut2, MenuTreeItem::Shortcut2);
	d->shortcuts->addButton(d->ui.shortcut3, MenuTreeItem::Shortcut3);
	d->shortcuts->addButton(d->ui.shortcut4, MenuTreeItem::Shortcut4);

	MenuTreeItem::makeRoot(d->ui.shortcut_tree);

	d->ui.shortcut_tree->header()->resizeSection(0, 200);

	d->ui.sub_priority->setAddingAndErasingEnabled(true);
	checkSubAutoselect(d->ui.sub_autoselect->currentData());

	d->ui.skin_name->addItems(Skin::names(true));
	onSkinIndexChanged(d->ui.skin_name->currentIndex());

	connect(d->ui.skin_name, SIGNAL(currentIndexChanged(int)), this, SLOT(onSkinIndexChanged(int)));
	connect(d->ui.sub_autoselect, SIGNAL(currentDataChanged(QVariant)), this, SLOT(checkSubAutoselect(QVariant)));
	connect(d->ui.sub_autoload, SIGNAL(currentDataChanged(QVariant)), this, SLOT(checkSubAutoselect(QVariant)));
	connect(d->shortcuts, SIGNAL(buttonClicked(int)), this, SLOT(getShortcut(int)));
	connect(d->ui.shortcut_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)), this, SLOT(onCurrentMenuChanged(QTreeWidgetItem*)));

	connect(d->ui.blur_kern_c, SIGNAL(valueChanged(int)), this, SLOT(onBlurKernelChanged()));
	connect(d->ui.blur_kern_n, SIGNAL(valueChanged(int)), this, SLOT(onBlurKernelChanged()));
	connect(d->ui.blur_kern_d, SIGNAL(valueChanged(int)), this, SLOT(onBlurKernelChanged()));

	connect(d->ui.sharpen_kern_c, SIGNAL(valueChanged(int)), this, SLOT(onSharpenKernelChanged()));
	connect(d->ui.sharpen_kern_n, SIGNAL(valueChanged(int)), this, SLOT(onSharpenKernelChanged()));
	connect(d->ui.sharpen_kern_d, SIGNAL(valueChanged(int)), this, SLOT(onSharpenKernelChanged()));

	connect(d->ui.dbb, SIGNAL(clicked(QAbstractButton*)), this, SLOT(onDialogButtonClicked(QAbstractButton*)));

	retranslate();
	fill(cPref);

//	d->ui.skin_prev->setWindow

//	d->ui.sub_shadow_blur->hide();
#ifdef Q_OS_MAC
	d->ui.system_tray_group->hide();
#endif
	adjustSize();
}

PrefDialog::~PrefDialog() {
	delete d;
}

void PrefDialog::onSkinIndexChanged(int idx) {
	if (idx >= 0) {
		const auto name = d->ui.skin_name->itemText(idx);
		const auto skin = Skin::source(name);
		d->ui.skin_path->setText(skin.absolutePath());
	}
}

void PrefDialog::getShortcut(int id) {
	MenuTreeItem *item = static_cast<MenuTreeItem*>(d->ui.shortcut_tree->currentItem());
	if (!item || item->isMenu())
		return;
	GetShortcutDialog dlg(item->shortcuts()[id - 1], this);
	if (dlg.exec())
		item->setShortcut(MenuTreeItem::Column(id), dlg.shortcut());
}

void PrefDialog::onCurrentMenuChanged(QTreeWidgetItem *it) {
	MenuTreeItem *item = static_cast<MenuTreeItem*>(it);
	const QList<QAbstractButton*> buttons = d->shortcuts->buttons();
	for (int i=0; i<buttons.size(); ++i)
		buttons[i]->setEnabled(item && !item->isMenu());
}


void PrefDialog::onCategoryChanged() {
	auto items = d->ui.tree->selectedItems();
	if (items.isEmpty())
		return;
	auto item = items.first();
	if (item->data(0, CategoryRole).toBool())
		return;
	d->ui.page_name->setText(item->parent()->text(0) % " > " % item->text(0));
	d->ui.stack->setCurrentWidget(item->data(0, WidgetRole).value<QWidget*>());
}

void PrefDialog::onBlurKernelChanged() {
	d->ui.blur_sum->setText(QString::number(d->ui.blur_kern_c->value() + d->ui.blur_kern_n->value()*4 + d->ui.blur_kern_d->value()*4));
}

void PrefDialog::onSharpenKernelChanged() {
	d->ui.sharpen_sum->setText(QString::number(d->ui.sharpen_kern_c->value() + d->ui.sharpen_kern_n->value()*4 + d->ui.sharpen_kern_d->value()*4));
}

void PrefDialog::checkSubAutoselect(const QVariant &data) {
	const bool enabled = data.toInt() == Enum::SubtitleAutoselect::Matched.id();
	d->ui.sub_ext_label->setEnabled(enabled);
	d->ui.sub_ext->setEnabled(enabled);
}

QString PrefDialog::toString(const QLocale &locale) {
	QString text;
	bool addName = true;
	switch (locale.language()) {
	case QLocale::C:
		text = tr("Use the system default language");
		addName = false;
		break;
	case QLocale::English:
		text = tr("English");
		break;
	case QLocale::Japanese:
		text = tr("Japanese");
		break;
	case QLocale::Korean:
		text = tr("Korean");
		break;
	case QLocale::Russian:
		text = tr("Russian");
		break;
	default:
		text = QLocale::languageToString(locale.language());
		break;
	}
	if (addName)
		text += " (" + locale.name() + ')';
	return text;
}

void PrefDialog::retranslate() {
	d->dbl->retranslate(tr("Double Click"));
	d->mdl->retranslate(tr("Middle Click"));
	d->whl->retranslate(tr("Wheel Scroll"));
	d->ui.sub_ext->setItemText(0, tr("All"));
	for (int i=0; i<d->ui.locale->count(); ++i)
		d->ui.locale->setItemText(i, toString(d->ui.locale->itemData(i).toLocale()));
	d->ui.dbb->button(DBB::Ok)->setText(tr("Ok"));
	d->ui.dbb->button(DBB::Cancel)->setText(tr("Cancel"));
	d->ui.dbb->button(DBB::Apply)->setText(tr("Apply"));
	d->ui.dbb->button(DBB::RestoreDefaults)->setText(tr("Restore Defaults"));
	d->ui.dbb->button(DBB::Reset)->setText(tr("Reset"));
}

void PrefDialog::fill(const Pref &p) {
	d->open_media_from_file_manager->setValue(p.open_media_from_file_manager);
	d->open_media_by_drag_and_drop->setValue(p.open_media_by_drag_and_drop);

	d->ui.pause_minimized->setChecked(p.pause_minimized);
	d->ui.pause_video_only->setChecked(p.pause_video_only);
	d->ui.remember_stopped->setChecked(p.remember_stopped);
	d->ui.ask_record_found->setChecked(p.ask_record_found);
	d->ui.enable_generate_playlist->setChecked(p.enable_generate_playist);
	d->ui.generate_playlist->setCurrentData(p.generate_playlist.id());
	d->ui.hide_cursor->setChecked(p.hide_cursor);
	d->ui.hide_delay->setValue(p.hide_cursor_delay/1000);
	d->ui.disable_screensaver->setChecked(p.disable_screensaver);

	d->ui.enable_hwaccel->setChecked(p.enable_hwaccel);
	for (auto codec : p.hwaccel_codecs) {
		auto ch = d->HwAcc[codec];
		if (ch)
			ch->setChecked(true);
	}

	d->ui.normalizer_silence->setValue(p.normalizer_silence);
	d->ui.normalizer_target->setValue(p.normalizer_target);
	d->ui.normalizer_min->setValue(p.normalizer_min*100.0);
	d->ui.normalizer_max->setValue(p.normalizer_max*100.0);

	d->ui.blur_kern_c->setValue(p.blur_kern_c);
	d->ui.blur_kern_n->setValue(p.blur_kern_n);
	d->ui.blur_kern_d->setValue(p.blur_kern_d);
	d->ui.sharpen_kern_c->setValue(p.sharpen_kern_c);
	d->ui.sharpen_kern_n->setValue(p.sharpen_kern_n);
	d->ui.sharpen_kern_d->setValue(p.sharpen_kern_d);
	d->ui.min_luma->setValue(p.adjust_contrast_min_luma);
	d->ui.max_luma->setValue(p.adjust_contrast_max_luma);

	d->ui.sub_enable_autoload->setChecked(p.sub_enable_autoload);
	d->ui.sub_enable_autoselect->setChecked(p.sub_enable_autoselect);
	d->ui.sub_autoload->setCurrentData(p.sub_autoload.id());
	d->ui.sub_autoselect->setCurrentData(p.sub_autoselect.id());
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
	d->ui.sub_font_scale->setCurrentData(p.sub_style.font.scale.id());
	d->ui.sub_font_size->setValue(p.sub_style.font.size*100.0);
	d->ui.sub_shadow->setChecked(p.sub_style.shadow.enabled);
	d->ui.sub_shadow_color->setColor(p.sub_style.shadow.color, false);
	d->ui.sub_shadow_opacity->setValue(p.sub_style.shadow.color.alphaF()*100.0);
	d->ui.sub_shadow_offset_x->setValue(p.sub_style.shadow.offset.x()*100.0);
	d->ui.sub_shadow_offset_y->setValue(p.sub_style.shadow.offset.y()*100.0);
//	d->ui.sub_shadow_blur->setChecked(p.sub_style.shadow_blur);
	d->ui.sub_spacing_line->setValue(p.sub_style.spacing.line*100.0);
	d->ui.sub_spacing_paragraph->setValue(p.sub_style.spacing.paragraph*100.0);
	d->ui.ms_per_char->setValue(p.ms_per_char);
	d->ui.sub_priority->setValues(p.sub_priority);

	d->ui.single_app->setChecked(cApp.isUnique());
	d->ui.locale->setCurrentData(p.locale);
	d->ui.window_style->setCurrentData(cApp.styleName());
	d->ui.enable_system_tray->setChecked(p.enable_system_tray);
	d->ui.hide_rather_close->setChecked(p.hide_rather_close);

	d->dbl->setValues(p.double_click_map);
	d->mdl->setValues(p.middle_click_map);
	d->whl->setValues(p.wheel_scroll_map);

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
	d->ui.sync_delay_step->setValue(p.sync_delay_step*0.001);

	d->ui.skin_name->setCurrentText(p.skin_name);
}

void PrefDialog::apply() {
	Pref &p = Pref::get();

	p.open_media_from_file_manager = d->open_media_from_file_manager->value();
	p.open_media_by_drag_and_drop = d->open_media_by_drag_and_drop->value();

	p.pause_minimized = d->ui.pause_minimized->isChecked();
	p.pause_video_only = d->ui.pause_video_only->isChecked();
	p.remember_stopped = d->ui.remember_stopped->isChecked();
	p.ask_record_found = d->ui.ask_record_found->isChecked();
	p.enable_generate_playist = d->ui.enable_generate_playlist->isChecked();
	p.generate_playlist = d->ui.generate_playlist->currentValue();
	p.hide_cursor = d->ui.hide_cursor->isChecked();
	p.hide_cursor_delay = d->ui.hide_delay->value()*1000;
	p.disable_screensaver = d->ui.disable_screensaver->isChecked();

	p.enable_hwaccel = d->ui.enable_hwaccel->isChecked();
	p.hwaccel_codecs.clear();
	for (auto it = d->HwAcc.begin(); it != d->HwAcc.end(); ++it) {
		if ((*it)->isChecked())
			p.hwaccel_codecs.append(it.key());
	}

	p.blur_kern_c = d->ui.blur_kern_c->value();
	p.blur_kern_n = d->ui.blur_kern_n->value();
	p.blur_kern_d = d->ui.blur_kern_d->value();
	p.sharpen_kern_c = d->ui.sharpen_kern_c->value();
	p.sharpen_kern_n = d->ui.sharpen_kern_n->value();
	p.sharpen_kern_d = d->ui.sharpen_kern_d->value();
	p.adjust_contrast_min_luma = d->ui.min_luma->value();
	p.adjust_contrast_max_luma = d->ui.max_luma->value();

	p.normalizer_target = d->ui.normalizer_target->value();
	p.normalizer_silence = d->ui.normalizer_silence->value();
	p.normalizer_min = d->ui.normalizer_min->value()/100.0;
	p.normalizer_max = d->ui.normalizer_max->value()/100.0;

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
//	p.sub_style.shadow_blur = d->ui.sub_shadow_blur->isChecked();
	p.sub_style.spacing.line = d->ui.sub_spacing_line->value()/100.0;
	p.sub_style.spacing.paragraph = d->ui.sub_spacing_paragraph->value()/100.0;
	p.ms_per_char = d->ui.ms_per_char->value();
	p.sub_priority = d->ui.sub_priority->values();

	cApp.setUnique(d->ui.single_app->isChecked());
	p.locale = d->ui.locale->currentData().toLocale();
	cApp.setStyleName(d->ui.window_style->currentData().toString());
	p.enable_system_tray = d->ui.enable_system_tray->isChecked();
	p.hide_rather_close = d->ui.hide_rather_close->isChecked();

	for (int i=0; i<d->ui.shortcut_tree->topLevelItemCount(); ++i)
		((MenuTreeItem*)(d->ui.shortcut_tree->topLevelItem(i)))->applyShortcut();

	p.double_click_map = d->dbl->values();
	p.middle_click_map = d->mdl->values();
	p.wheel_scroll_map = d->whl->values();

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
	p.sync_delay_step = qRound(d->ui.sync_delay_step->value()*1000.0);

	p.skin_name = d->ui.skin_name->currentText();

	p.save();

	emit applicationRequested();
}

void PrefDialog::changeEvent(QEvent *event) {
	QWidget::changeEvent(event);
	if (event->type() == QEvent::LanguageChange) {
		d->ui.retranslateUi(this);
		retranslate();
	}
}

void PrefDialog::onDialogButtonClicked(QAbstractButton *button) {
	auto reset = [this] () {fill(cPref);};
	auto restore = [this] () {fill(Pref());};

	switch (d->ui.dbb->standardButton(button)) {
	case DBB::Ok:
		hide();
	case DBB::Apply:
		apply();
		break;
	case DBB::Cancel:
		hide();
	case DBB::Reset:
		reset();
		break;
	case DBB::RestoreDefaults:
		restore();
		break;
	default:
		break;
	}
}

void PrefDialog::showEvent(QShowEvent *event) {
	QDialog::showEvent(event);
}
