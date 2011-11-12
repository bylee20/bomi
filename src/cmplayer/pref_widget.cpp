#include "pref_widget.hpp"
#include "application.hpp"
#include <QtCore/QDebug>
#include "translator.hpp"
#include "info.hpp"
#include "dialogs.hpp"
#include <QtGui/QColorDialog>
#include <QtGui/QFontDialog>
#include <QtGui/QGroupBox>
#include "ui_pref_widget.h"
#include "rootmenu.hpp"

template <typename Enum>
class PrefWidgetMouseGroup : public QWidget {
public:
	typedef ::Enum::KeyModifier Modifier;
	typedef EnumComboBox<Enum> ComboBox;
	typedef Pref::ActionEnumInfo<Enum> ActionInfo;
	typedef Pref::KeyModifierMap<Enum> ActionMap;
	PrefWidgetMouseGroup(QFormLayout *form, bool line, QWidget *parent = 0)
	: QWidget(parent), form(form) {
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
			connect(check, SIGNAL(toggled(bool)), combo, SLOT(setEnabled(bool)));
		}

		form->addRow(QLatin1String(":"), this);
		if (line) {
			QFrame *line = new QFrame;
			line->setFrameShadow(QFrame::Plain);
			line->setFrameShape(QFrame::HLine);
			form->addRow(line);
		}
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
			info.action.set(combos[i]->itemData(combos[i]->currentIndex()).toInt());
		}
		return map;
	}
	void retranslate(const QString &n) {static_cast<QLabel*>(form->labelForField(this))->setText(n);}
private:
	QList<ComboBox*> combos;
	QList<Modifier> mods;
	QList<QCheckBox*> checks;
	QFormLayout *form;
};


struct Pref::Widget::Data {
	Ui::Pref_Widget ui;
	QButtonGroup *shortcuts;
	PrefWidgetMouseGroup<Enum::ClickAction> *dbl, *mdl;
	PrefWidgetMouseGroup<Enum::WheelAction> *whl;
};

class Pref::Widget::MenuTreeItem : public QTreeWidgetItem {
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
		RootMenu &root = RootMenu::get();
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

Pref::Widget::Widget(QWidget *parent)
: QWidget(parent), d(new Data) {
	QString css = QLatin1String("QFrame[frameShape=\"%1\"] {color:#888;}");
	css = css.arg((int)QFrame::HLine);
#ifdef Q_WS_MAC
	css += QLatin1String("QTabWidget::pane {border-top: 1px solid #777;"
	    "position: absolute; top: -0.5em; padding-top: 5px;}");
#endif
	setStyleSheet(css);
	d->ui.setupUi(this);

	d->ui.sub_ext->addItem(QString(), QString());
	d->ui.sub_ext->addItemTextData(Info::subtitleExt());
	d->ui.locale->addItemData(Translator::availableLocales());
	d->ui.window_style->addItemTextData(app()->availableStyleNames());

	d->dbl = new PrefWidgetMouseGroup<Enum::ClickAction>(d->ui.mouse_form, true);
	d->mdl = new PrefWidgetMouseGroup<Enum::ClickAction>(d->ui.mouse_form, true);
	d->whl = new PrefWidgetMouseGroup<Enum::WheelAction>(d->ui.mouse_form, false);

	d->shortcuts = new QButtonGroup(this);
	d->shortcuts->addButton(d->ui.shortcut1, MenuTreeItem::Shortcut1);
	d->shortcuts->addButton(d->ui.shortcut2, MenuTreeItem::Shortcut2);
	d->shortcuts->addButton(d->ui.shortcut3, MenuTreeItem::Shortcut3);
	d->shortcuts->addButton(d->ui.shortcut4, MenuTreeItem::Shortcut4);

	MenuTreeItem::makeRoot(d->ui.shortcut_tree);

	d->ui.shortcut_tree->header()->resizeSection(0, 200);

	d->ui.sub_priority->setAddingAndErasingEnabled(true);
	checkSubAutoselect(d->ui.sub_autoselect->currentData());

	connect(d->ui.sub_autoselect, SIGNAL(currentDataChanged(QVariant)), this, SLOT(checkSubAutoselect(QVariant)));
	connect(d->shortcuts, SIGNAL(buttonClicked(int)), this, SLOT(getShortcut(int)));
	connect(d->ui.shortcut_tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*))
		, this, SLOT(checkCurrentMenu(QTreeWidgetItem*)));

	retranslate();
	fill();
}

Pref::Widget::~Widget() {
	delete d;
}

void Pref::Widget::retranslate() {
	d->dbl->retranslate(tr("Double Click:"));
	d->mdl->retranslate(tr("Middle Click:"));
	d->whl->retranslate(tr("Wheel Scroll:"));
	d->ui.sub_ext->setItemText(0, tr("All"));
	for (int i=0; i<d->ui.locale->count(); ++i)
		d->ui.locale->setItemText(i, toString(d->ui.locale->itemData(i).toLocale()));
}


QSize Pref::Widget::sizeHint() const {
	int width = 0;
	int height = 0;
	for (int i=0; i<d->ui.stack->count(); ++i) {
		const QSize hint = d->ui.stack->widget(i)->sizeHint();
		width = qMax(width, hint.width());
		height = qMax(height, hint.height());
	}
	return QSize(width, height);
}

void Pref::Widget::checkCurrentMenu(QTreeWidgetItem *it) {
	MenuTreeItem *item = (MenuTreeItem*)it;
	const QList<QAbstractButton*> buttons = d->shortcuts->buttons();
	for (int i=0; i<buttons.size(); ++i)
		buttons[i]->setEnabled(item && !item->isMenu());
}

void Pref::Widget::getShortcut(int id) {
	MenuTreeItem *item = (MenuTreeItem*)(d->ui.shortcut_tree->currentItem());
	if (!item || item->isMenu())
		return;
	GetShortcutDialog dlg(item->shortcuts()[id - 1], this);
	if (dlg.exec())
		item->setShortcut(MenuTreeItem::Column(id), dlg.shortcut());
}

void Pref::Widget::fill() {
	const Pref &p = Pref::get();

	d->ui.pause_minimized->setChecked(p.pause_minimized);
	d->ui.pause_video_only->setChecked(p.pause_video_only);
	d->ui.remember_stopped->setChecked(p.remember_stopped);
	d->ui.ask_record_found->setChecked(p.ask_record_found);
	d->ui.generate_playlist->setCurrentData(p.generate_playlist.id());
	d->ui.hide_cursor->setChecked(p.hide_cursor);
	d->ui.hide_delay->setValue(p.hide_cursor_delay/1000);
	d->ui.disable_screensaver->setChecked(p.disable_screensaver);

	d->ui.blur_kern_c->setValue(p.blur_kern_c);
	d->ui.blur_kern_n->setValue(p.blur_kern_n);
	d->ui.blur_kern_d->setValue(p.blur_kern_d);
	d->ui.sharpen_kern_c->setValue(p.sharpen_kern_c);
	d->ui.sharpen_kern_n->setValue(p.sharpen_kern_n);
	d->ui.sharpen_kern_d->setValue(p.sharpen_kern_d);
	d->ui.min_luma->setValue(p.adjust_contrast_min_luma);
	d->ui.max_luma->setValue(p.adjust_contrast_max_luma);
	d->ui.auto_contrast_threshold->setValue(p.auto_contrast_threshold);

	d->ui.normalizer_gain->setValue(p.normalizer_gain);
	d->ui.normalizer_smoothness->setValue(p.normalizer_smoothness);

	d->ui.sub_autoload->setCurrentData(p.sub_autoload.id());
	d->ui.sub_autoselect->setCurrentData(p.sub_autoselect.id());
	d->ui.sub_ext->setCurrentData(p.sub_ext);
	d->ui.sub_enc->setEncoding(p.sub_enc);
	d->ui.sub_enc_autodetection->setChecked(p.sub_enc_autodetection);
	d->ui.sub_enc_accuracy->setValue(p.sub_enc_accuracy);
	d->ui.sub_font_family->setCurrentFont(p.sub_style.font);
	d->ui.sub_font_option->set(p.sub_style.font);
	d->ui.sub_color_fg->setColor(p.sub_style.color_fg, false);
	d->ui.sub_color_bg->setColor(p.sub_style.color_bg, false);
	d->ui.sub_auto_size->setCurrentData(p.sub_style.auto_size.id());
	d->ui.sub_size_scale->setValue(p.sub_style.text_scale*100.);
	d->ui.sub_has_shadow->setChecked(p.sub_style.has_shadow);
	d->ui.sub_shadow_color->setColor(p.sub_style.shadow_color, true);
	d->ui.sub_shadow_offset_x->setValue(p.sub_style.shadow_offset.x());
	d->ui.sub_shadow_offset_y->setValue(p.sub_style.shadow_offset.y());
	d->ui.sub_shadow_blur->setValue(p.sub_style.shadow_blur);
	d->ui.ms_per_char->setValue(p.ms_per_char);
	d->ui.sub_priority->setValues(p.sub_priority);

	d->ui.single_app->setChecked(app()->isUnique());
	d->ui.locale->setCurrentData(p.locale);
	d->ui.window_style->setCurrentData(app()->styleName());
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
}

void Pref::Widget::apply() {
	Q_ASSERT(Pref::obj != 0);
	Pref &p = *Pref::obj;

	p.pause_minimized = d->ui.pause_minimized->isChecked();
	p.pause_video_only = d->ui.pause_video_only->isChecked();
	p.remember_stopped = d->ui.remember_stopped->isChecked();
	p.ask_record_found = d->ui.ask_record_found->isChecked();
	p.generate_playlist.set(d->ui.generate_playlist->currentData().toInt());
	p.hide_cursor = d->ui.hide_cursor->isChecked();
	p.hide_cursor_delay = d->ui.hide_delay->value()*1000;
	p.disable_screensaver = d->ui.disable_screensaver->isChecked();

	p.blur_kern_c = d->ui.blur_kern_c->value();
	p.blur_kern_n = d->ui.blur_kern_n->value();
	p.blur_kern_d = d->ui.blur_kern_d->value();
	p.sharpen_kern_c = d->ui.sharpen_kern_c->value();
	p.sharpen_kern_n = d->ui.sharpen_kern_n->value();
	p.sharpen_kern_d = d->ui.sharpen_kern_d->value();
	p.adjust_contrast_min_luma = d->ui.min_luma->value();
	p.adjust_contrast_max_luma = d->ui.max_luma->value();
	p.auto_contrast_threshold = d->ui.auto_contrast_threshold->value();

	p.normalizer_gain = d->ui.normalizer_gain->value();
	p.normalizer_smoothness = d->ui.normalizer_smoothness->value();

	p.sub_autoload.set(d->ui.sub_autoload->currentData().toInt());
	p.sub_autoselect.set(d->ui.sub_autoselect->currentData().toInt());
	p.sub_ext = d->ui.sub_ext->currentData().toString();
	p.sub_enc = d->ui.sub_enc->encoding();
	p.sub_enc_autodetection = d->ui.sub_enc_autodetection->isChecked();
	p.sub_enc_accuracy = d->ui.sub_enc_accuracy->value();
	p.sub_style.font = d->ui.sub_font_family->currentFont();
	p.sub_style.font.setBold(d->ui.sub_font_option->bold());
	p.sub_style.font.setItalic(d->ui.sub_font_option->italic());
	p.sub_style.font.setUnderline(d->ui.sub_font_option->underline());
	p.sub_style.font.setStrikeOut(d->ui.sub_font_option->strikeOut());
	p.sub_style.color_fg = d->ui.sub_color_fg->color();
	p.sub_style.color_bg = d->ui.sub_color_bg->color();
	p.sub_style.auto_size.set(d->ui.sub_auto_size->currentData().toInt());
	p.sub_style.text_scale = d->ui.sub_size_scale->value()/100.0;
	p.sub_style.has_shadow = d->ui.sub_has_shadow->isChecked();
	p.sub_style.shadow_color = d->ui.sub_shadow_color->color();
	p.sub_style.shadow_offset.rx() = d->ui.sub_shadow_offset_x->value();
	p.sub_style.shadow_offset.ry() = d->ui.sub_shadow_offset_y->value();
	p.sub_style.shadow_blur = d->ui.sub_shadow_blur->value();
	p.ms_per_char = d->ui.ms_per_char->value();
	p.sub_priority = d->ui.sub_priority->values();

	app()->setUnique(d->ui.single_app->isChecked());
	p.locale = d->ui.locale->currentData().toLocale();
	app()->setStyleName(d->ui.window_style->currentData().toString());
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

	p.save();
}

void Pref::Widget::changeEvent(QEvent *event) {
	QWidget::changeEvent(event);
	if (event->type() == QEvent::LanguageChange) {
		retranslate();
		d->ui.retranslateUi(this);
	}
}

void Pref::Widget::checkSubAutoselect(const QVariant &data) {
	const bool enabled = data.toInt() == Enum::SubtitleAutoselect::Matched.id();
	d->ui.sub_ext_label->setEnabled(enabled);
	d->ui.sub_ext->setEnabled(enabled);
}

QString Pref::Widget::toString(const QLocale &locale) {
	QString text;
	bool addName = true;
	switch (locale.language()) {
	case QLocale::C:
		text = tr("System Default Locale");
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
	default:
		text = QLocale::languageToString(locale.language());
		break;
	}
	if (addName)
		text += " (" + locale.name() + ')';
	return text;
}

int Pref::Widget::pageCount() const {
	return d->ui.stack->count();
}

QString Pref::Widget::pageName(int idx) const {
	return d->ui.stack->widget(idx)->property("name").toString();
}

QIcon Pref::Widget::pageIcon(int idx) const {
	return d->ui.stack->widget(idx)->property("icon").value<QIcon>();
}

void Pref::Widget::setCurrentPage(int idx) const {
	d->ui.stack->setCurrentIndex(idx);
}

int Pref::Widget::currentPage() const {
	return d->ui.stack->currentIndex();
}
