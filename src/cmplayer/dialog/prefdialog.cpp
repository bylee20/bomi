#include "prefdialog.hpp"
#include "prefdialog_p.hpp"
#include "shortcutdialog.hpp"
#include "player/app.hpp"
#include "player/skin.hpp"
#include "player/pref.hpp"
#include "player/mrlstate.hpp"
#include "widget/deintwidget.hpp"
#include "misc/simplelistmodel.hpp"
#include "ui_prefdialog.h"

#ifdef None
#undef None
#endif

using MouseAction = MouseActionGroupBox::Action;

class MrlStatePropertyListModel
        : public SimpleListModel<MrlState::PropertyInfo,
                                 QVector<MrlState::PropertyInfo>> {
public:
    MrlStatePropertyListModel() { setCheckable(0, true); }
    auto flags(int row, int column) const -> Qt::ItemFlags
        { return Super::flags(row, column) | Qt::ItemIsUserCheckable; }
    auto displayData(int row, int /*column*/) const -> QVariant
        { return at(row).description; }
};

/******************************************************************************/

struct PrefDialog::Data {
    Ui::PrefDialog ui;
    QVector<MouseAction> actionInfo;
    QVector<PrefMenuTreeItem*> actionItems;
    QButtonGroup *shortcutGroup, *saveQuickSnapshot;
    QMap<int, QCheckBox*> hwdec;
    QMap<DeintMethod, QCheckBox*> hwdeint;
    QStringList imports;
    DeintWidget *deint_swdec = nullptr, *deint_hwdec = nullptr;
    MrlStatePropertyListModel properties;
    auto updateCodecCheckBox() -> void
    {
        const auto data = ui.hwacc_backend->currentData().toInt();
        const auto type = static_cast<HwAcc::Type>(data);
        const auto codecs = HwAcc::fullCodecList();
        for (const auto codec : codecs) {
            auto box = hwdec[codec];
            const auto supported = HwAcc::supports(type, codec);
            const QString desc(_L(avcodec_descriptor_get(codec)->long_name));
            if (supported)
                box->setText(desc);
            else
                box->setText(desc % ' '_q % tr("Not supported") % ')'_q);
            box->setEnabled(supported);
        }
    }
    auto retranslate() -> void
    {
        ui.sub_ext->setItemText(0, tr("All"));
        properties.setList(MrlState::restorableProperties());
    }
    auto setShortcuts(const Shortcuts &shortcuts) -> void
    {
        for (auto item : actionItems)
            item->setShortcuts(shortcuts[item->id()]);
    }
    auto shortcuts() -> Shortcuts
    {
        Shortcuts shortcuts;
        for (auto item : actionItems) {
            const auto keys = item->shortcuts();
            if (!keys.isEmpty())
                shortcuts[item->id()] = keys;
        }
        return shortcuts;
    }
};

PrefDialog::PrefDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
    d->ui.setupUi(this);
    d->ui.tree->setItemDelegate(new PrefDelegate(d->ui.tree));
    d->ui.tree->setIconSize(QSize(16, 16));

    connect(d->ui.tree, &QTreeWidget::itemSelectionChanged, [this] () {
        auto items = d->ui.tree->selectedItems();
        if (items.isEmpty())
            return;
        auto item = items.first();
        if (item->data(0, CategoryRole).toBool())
            return;
        const QString text = item->parent()->text(0) % " > "_a % item->text(0);
        const auto widget = item->data(0, WidgetRole).value<QWidget*>();
        d->ui.page_name->setText(text);
        d->ui.stack->setCurrentWidget(widget);
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

    auto addPage = [this] (const QString &name, QWidget *widget,
                           const QString &icon, QTreeWidgetItem *parent) {
        auto item = new QTreeWidgetItem(parent);
        item->setText(0, name);
        item->setIcon(0, QIcon(icon));
        item->setData(0, CategoryRole, false);
        item->setData(0, WidgetRole, QVariant::fromValue(widget));
        return item;
    };

    auto general = addCategory(tr("General"));
    addPage(tr("Application"), d->ui.gen_behaviours,
            u":/img/cmplayer-32.png"_q, general)->setSelected(true);
    addPage(tr("Open"), d->ui.open_media,
            u":/img/document-open-32.png"_q, general);
    addPage(tr("Playback"), d->ui.playback,
            u":/img/media-playback-start-32.png"_q, general);
    addPage(tr("Cache"), d->ui.cache,
            u":/img/preferences-web-browser-cache.png"_q, general);
    addPage(tr("Miscellaneous"), d->ui.misc,
            u":/img/applications-education-miscellaneous-32.png"_q, general);

    auto appear = addCategory(tr("Appearance"));
    addPage(tr("OSD"), d->ui.osd,
            u":/img/view-multiple-objects.png"_q, appear);
    addPage(tr("Skin & Style"), d->ui.skin_style,
            u":/img/preferences-desktop-theme-32.png"_q, appear);

    auto video = addCategory(tr("Video"));
    addPage(tr("Hardware acceleration"), d->ui.video_hwacc,
            u":/img/apps-hardware-icon.png"_q, video);
    addPage(tr("Deinterlace"), d->ui.video_deint,
            u":/img/format-line-spacing-double.png"_q, video);
    addPage(tr("Video filter"), d->ui.video_filter,
            u":/img/draw-brush.png"_q, video);

    auto audio = addCategory(tr("Audio"));
    addPage(tr("Sound"), d->ui.audio_sound,
            u":/img/audio-volume-high.png"_q, audio);
    addPage(tr("Audio filter"), d->ui.audio_filter,
            u":/img/applications-multimedia.png"_q, audio);

    auto subtitle = addCategory(tr("Subtitle"));
    addPage(tr("Load"), d->ui.sub_load,
            u":/img/application-x-subrip-32.png"_q, subtitle);
    addPage(tr("Appearance"), d->ui.sub_appearance,
            u":/img/format-text-color-32.png"_q, subtitle);
    addPage(tr("Priority"), d->ui.sub_unified,
            u":/img/view-sort-descending-32.png"_q, subtitle);

    auto ui = addCategory(tr("User interface"));
    addPage(tr("Keyboard shortcuts"), d->ui.ui_shortcut,
            u":/img/preferences-desktop-keyboard-32.png"_q, ui);
    addPage(tr("Mouse actions"), d->ui.ui_mouse,
            u":/img/input-mouse-32.png"_q, ui);
    addPage(tr("Control step"), d->ui.ui_step,
            u":/img/run-build-32.png"_q, ui);

    auto vbox = new QVBoxLayout;
    vbox->setMargin(0);

    auto backends = HwAcc::availableBackends();
    for (auto type : backends) {
        const auto desc = HwAcc::backendDescription(type);
        d->ui.hwacc_backend->addItem(desc, (int)type);
    }

    const auto codecs = HwAcc::fullCodecList();
    for (const auto codec : codecs)
        vbox->addWidget(d->hwdec[codec] = new QCheckBox);
    d->ui.hwdec_list->setLayout(vbox);

    void(QComboBox::*currentIdxChanged)(int) = &QComboBox::currentIndexChanged;
    connect(d->ui.hwacc_backend, currentIdxChanged,
            [this] () { d->updateCodecCheckBox(); });
    d->updateCodecCheckBox();

    auto checkHearbeat = [this] () {
        const auto enable = d->ui.use_heartbeat->isChecked()
                            && d->ui.disable_screensaver->isChecked();
        d->ui.heartbeat_widget->setEnabled(enable);
    };
    const auto CheckBoxToggled = &QCheckBox::toggled;
    connect(d->ui.use_heartbeat, CheckBoxToggled, this, checkHearbeat);
    connect(d->ui.disable_screensaver, CheckBoxToggled, this, checkHearbeat);

    connect(d->ui.quick_snapshot_folder_browse, &QPushButton::clicked,
            this, [this] () {
        _SetLastOpenPath(d->ui.quick_snapshot_folder->text() % '/'_q, u"snapshot"_q);
        auto dir = _GetOpenDir(this, tr("Browse for Folder"), u"snapshot"_q);
        if (!dir.isEmpty())
            d->ui.quick_snapshot_folder->setText(dir);
    });

    d->saveQuickSnapshot = new QButtonGroup(this);
    d->saveQuickSnapshot->setExclusive(true);
    d->saveQuickSnapshot->addButton(d->ui.save_quick_snapshot_folder,
                                    (int)QuickSnapshotSave::Fixed);
    d->saveQuickSnapshot->addButton(d->ui.save_quick_snapshot_current,
                                    (int)QuickSnapshotSave::Current);
    d->saveQuickSnapshot->addButton(d->ui.save_quick_snapshot_ask
                                    , (int)QuickSnapshotSave::Ask);
    d->ui.quick_snapshot_format->addItems(_ExtList(WritableImageExt));

    vbox = new QVBoxLayout;
    vbox->setMargin(0);

    d->deint_swdec = new DeintWidget(DecoderDevice::CPU);
    d->deint_hwdec = new DeintWidget(DecoderDevice::GPU);
    d->ui.deint_tabs->addTab(d->deint_swdec, tr("For S/W decoding"));
    d->ui.deint_tabs->addTab(d->deint_hwdec, tr("For H/W decoding"));
    d->ui.deint_desc->setText(DeintWidget::informations());

    d->ui.sub_ext->addItem(QString(), QString());
    d->ui.sub_ext->addItemTextData(_ExtList(SubtitleExt));
    d->ui.window_style->addItemTextData(cApp.availableStyleNames());

    d->shortcutGroup = new QButtonGroup(this);
    d->shortcutGroup->addButton(d->ui.shortcut1, 0);
    d->shortcutGroup->addButton(d->ui.shortcut2, 1);
    d->shortcutGroup->addButton(d->ui.shortcut3, 2);
    d->shortcutGroup->addButton(d->ui.shortcut4, 3);

    _R(d->actionItems, d->actionInfo)
            = PrefMenuTreeItem::makeRoot(d->ui.shortcut_tree);
    d->ui.shortcut_tree->header()->resizeSection(0, 200);

    d->ui.mouse_double_click->set(d->actionInfo);
    d->ui.mouse_middle_click->set(d->actionInfo);
    d->ui.mouse_scroll_up->set(d->actionInfo);
    d->ui.mouse_scroll_down->set(d->actionInfo);

    QVector<AudioDriver> audioDrivers;
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

    connect(d->ui.skin_name, currentIdxChanged, this, updateSkinPath);

    auto currentDataChanged = &DataComboBox::currentDataChanged;
    connect(d->ui.sub_autoselect, currentDataChanged, checkSubAutoselect);
    connect(d->ui.sub_autoload, currentDataChanged, checkSubAutoselect);
    void(QButtonGroup::*buttonClicked)(int) = &QButtonGroup::buttonClicked;
    connect(d->shortcutGroup, buttonClicked, [this] (int idx) {
        auto treeItem = d->ui.shortcut_tree->currentItem();
        auto item = static_cast<PrefMenuTreeItem*>(treeItem);
        if (item && !item->isMenu()) {
            ShortcutDialog dlg(item->shortcut(idx), this);
            dlg.setQueryFunction([=] (const QString &id,
                                      const QKeySequence &key)
            {
                for (auto item : d->actionItems) {
                    if (item->hasShortcut(key)) {
                        if (item->id() == id)
                            return QString();
                        return item->description();
                    }
                }
                return QString();
            }, item->id());
            if (dlg.exec())
                item->setShortcut(idx, dlg.shortcut());
        }
    });
    connect(d->ui.shortcut_tree, &QTreeWidget::currentItemChanged,
            [this] (QTreeWidgetItem *it) {
        auto item = static_cast<PrefMenuTreeItem*>(it);
        const auto buttons = d->shortcutGroup->buttons();
        for (auto b : buttons)
            b->setEnabled(item && !item->isMenu());
    });

    auto onBlurKernelChanged = [this] () {
        const auto number = d->ui.blur_kern_c->value()
                            + d->ui.blur_kern_n->value()*4
                            + d->ui.blur_kern_d->value()*4;
        d->ui.blur_sum->setText(QString::number(number));
    };
    auto onSharpenKernelChanged = [this] () {
        const auto number = d->ui.sharpen_kern_c->value()
                            + d->ui.sharpen_kern_n->value()*4
                            + d->ui.sharpen_kern_d->value()*4;
        d->ui.sharpen_sum->setText(QString::number(number));
    };
    void(QSpinBox::*valueChanged)(int) = &QSpinBox::valueChanged;
    connect(d->ui.blur_kern_c, valueChanged, onBlurKernelChanged);
    connect(d->ui.blur_kern_n, valueChanged, onBlurKernelChanged);
    connect(d->ui.blur_kern_d, valueChanged, onBlurKernelChanged);

    connect(d->ui.sharpen_kern_c, valueChanged, onSharpenKernelChanged);
    connect(d->ui.sharpen_kern_n, valueChanged, onSharpenKernelChanged);
    connect(d->ui.sharpen_kern_d, valueChanged, onSharpenKernelChanged);

    connect(d->ui.dbb, &BBox::clicked, [this] (QAbstractButton *button) {
        switch (d->ui.dbb->standardButton(button)) {
        case BBox::Ok:
            hide();
        case BBox::Apply:
            emit applyRequested();
            break;
        case BBox::Cancel:
            hide();
        case BBox::Reset:
            emit resetRequested();
            break;
        case BBox::RestoreDefaults:
            set(Pref());
            break;
        default:
            break;
        }
    });

    static constexpr auto cmplayer = static_cast<int>(KeyMapPreset::CMPlayer);
    static constexpr auto movist = static_cast<int>(KeyMapPreset::Movist);
    d->ui.shortcut_preset->addItem(tr("CMPlayer"), cmplayer);
    d->ui.shortcut_preset->addItem(tr("Movist"), movist);

    connect(d->ui.load_preset, &QPushButton::clicked, [this] () {
        const int idx = d->ui.shortcut_preset->currentIndex();
        if (idx != -1) {
            const auto data = d->ui.shortcut_preset->itemData(idx).toInt();
            d->setShortcuts(Pref::preset(static_cast<KeyMapPreset>(data)));
        }
    });

    d->retranslate();
    d->ui.restore_properties->setModel(&d->properties);
#ifdef Q_OS_MAC
    d->ui.system_tray_group->hide();
#endif
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

template<class T>
static void setHw(QGroupBox *group, bool enabled,
                  QMap<T, QCheckBox*> &map, const QVector<T> &keys) {
    group->setChecked(enabled);
    for (auto key : keys) {
        if (auto ch = map.value(key))
            ch->setChecked(true);
    }
}

template<class T>
static void getHw(bool &enabled, QGroupBox *group,
                  QVector<T> &keys, const QMap<T, QCheckBox*> &map) {
    enabled = group->isChecked();
    keys.clear();
    for (auto it = map.begin(); it != map.end(); ++it) {
        if (*it && (*it)->isChecked())
            keys << it.key();
    }
}

auto PrefDialog::set(const Pref &p) -> void
{
    d->ui.open_media_from_file_manager->setValue(p.open_media_from_file_manager);
    d->ui.open_media_by_drag_and_drop->setValue(p.open_media_by_drag_and_drop);

    d->saveQuickSnapshot->button((int)p.quick_snapshot_save)->setChecked(true);
    d->ui.quick_snapshot_folder->setText(p.quick_snapshot_folder);
    d->ui.quick_snapshot_format->setCurrentText(p.quick_snapshot_format);
    d->ui.quick_snapshot_quality->setValue(p.quick_snapshot_quality);

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
    d->ui.lion_style_fullscreen->setChecked(p.lion_style_fullscreen);
    if (p.show_logo)
        d->ui.show_logo->setChecked(true);
    else
        d->ui.fill_bg_color->setChecked(true);
    d->ui.bg_color->setColor(p.bg_color, false);
    d->ui.use_heartbeat->setChecked(p.use_heartbeat);
    d->ui.heartbeat_command->setText(p.heartbeat_command);
    d->ui.heartbeat_interval->setValue(p.heartbeat_interval);

    d->ui.osd_font->setCurrentFont(QFont(p.osd_theme.font));
    d->ui.osd_color->setColor(p.osd_theme.color, true);
    d->ui.osd_font_option->set(p.osd_theme.bold, p.osd_theme.italic,
                               p.osd_theme.underline, p.osd_theme.strikeout);
    d->ui.osd_scale->setValue(p.osd_theme.scale*100.0);
    d->ui.osd_style_color->setColor(p.osd_theme.styleColor, true);
    d->ui.osd_style->setCurrentValue(p.osd_theme.style);
    d->ui.show_osd_timeline->setChecked(p.show_osd_timeline);

    const auto data = d->ui.hwacc_backend->findData(p.hwaccel_backend);
    d->ui.hwacc_backend->setCurrentIndex(data);
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
    d->ui.sub_priority->setList(p.sub_priority);

    d->ui.single_app->setChecked(cApp.isUnique());
    d->ui.window_style->setCurrentText(cApp.styleName(), Qt::MatchFixedString);
    d->ui.locale->setCurrentLocale(cApp.locale());
    d->ui.skin_name->setCurrentText(p.skin_name);
    d->ui.enable_system_tray->setChecked(p.enable_system_tray);
    d->ui.hide_rather_close->setChecked(p.hide_rather_close);

    d->ui.mouse_double_click->setValues(p.double_click_map);
    d->ui.mouse_middle_click->setValues(p.middle_click_map);
    d->ui.mouse_scroll_up->setValues(p.scroll_up_map);
    d->ui.mouse_scroll_down->setValues(p.scroll_down_map);
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

    d->ui.audio_driver->setCurrentData((int)p.audio_driver);
    d->ui.clipping_method->setCurrentValue(p.clipping_method);

    d->ui.cache_local->setValue(p.cache_local);
    d->ui.cache_network->setValue(p.cache_network);
    d->ui.cache_disc->setValue(p.cache_disc);
    d->ui.cache_min_playback->setValue(p.cache_min_playback);
    d->ui.cache_min_seeking->setValue(p.cache_min_seeking);
    d->ui.network_folders->setList(p.network_folders);

    d->setShortcuts(p.shortcuts);

    QVector<bool> restores(d->properties.size(), false);
    for (int i=0; i<d->properties.size(); ++i) {
        const auto property = d->properties.at(i).property;
        restores[i] = p.restore_properties.contains(property);
    }
    d->properties.setChecked(0, restores);
}

auto PrefDialog::get(Pref &p) -> void
{
    p.open_media_from_file_manager = d->ui.open_media_from_file_manager->value();
    p.open_media_by_drag_and_drop = d->ui.open_media_by_drag_and_drop->value();

    p.quick_snapshot_save = (QuickSnapshotSave)d->saveQuickSnapshot->checkedId();
    p.quick_snapshot_folder = d->ui.quick_snapshot_folder->text();
    p.quick_snapshot_format = d->ui.quick_snapshot_format->currentText();
    p.quick_snapshot_quality = d->ui.quick_snapshot_quality->value();

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
    p.lion_style_fullscreen = d->ui.lion_style_fullscreen->isChecked();
    p.show_logo = d->ui.show_logo->isChecked();
    p.bg_color = d->ui.bg_color->color();
    p.use_heartbeat = d->ui.use_heartbeat->isChecked();
    p.heartbeat_command = d->ui.heartbeat_command->text();
    p.heartbeat_interval = d->ui.heartbeat_interval->value();

    p.osd_theme.font = d->ui.osd_font->currentFont().family();
    p.osd_theme.color = d->ui.osd_color->color();
    p.osd_theme.underline = d->ui.osd_font_option->underline();
    p.osd_theme.italic = d->ui.osd_font_option->italic();
    p.osd_theme.bold = d->ui.osd_font_option->bold();
    p.osd_theme.strikeout = d->ui.osd_font_option->strikeOut();
    p.osd_theme.scale = d->ui.osd_scale->value()*1e-2;
    p.osd_theme.style = d->ui.osd_style->currentValue();
    p.osd_theme.styleColor = d->ui.osd_style_color->color();
    p.show_osd_timeline = d->ui.show_osd_timeline->isChecked();

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
    p.sub_priority = d->ui.sub_priority->list();

    cApp.setUnique(d->ui.single_app->isChecked());
    cApp.setLocale(d->ui.locale->currentLocale());
    cApp.setStyleName(d->ui.window_style->currentData().toString());
    p.enable_system_tray = d->ui.enable_system_tray->isChecked();
    p.hide_rather_close = d->ui.hide_rather_close->isChecked();

    p.double_click_map = d->ui.mouse_double_click->values();
    p.middle_click_map = d->ui.mouse_middle_click->values();
    p.scroll_up_map    = d->ui.mouse_scroll_up->values();
    p.scroll_down_map  = d->ui.mouse_scroll_down->values();
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

    const auto data = d->ui.audio_driver->currentData().toInt();
    p.audio_driver = AudioDriverInfo::from(data);
    p.clipping_method = d->ui.clipping_method->currentValue();

    p.cache_local = d->ui.cache_local->value();
    p.cache_network = d->ui.cache_network->value();
    p.cache_disc = d->ui.cache_disc->value();
    p.cache_min_playback = d->ui.cache_min_playback->value();
    p.cache_min_seeking = d->ui.cache_min_seeking->value();
    p.network_folders = d->ui.network_folders->list();

    p.shortcuts = d->shortcuts();

    auto restores = d->properties.checkedList(0);
    p.restore_properties.clear();
    for (int i=0; i<restores.size(); ++i) {
        if (restores[i])
            p.restore_properties.append(d->properties.at(i).property);
    }
}

auto PrefDialog::changeEvent(QEvent *event) -> void
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
        d->retranslate();
    }
}

auto PrefDialog::showEvent(QShowEvent *event) -> void
{
    QDialog::showEvent(event);
}
