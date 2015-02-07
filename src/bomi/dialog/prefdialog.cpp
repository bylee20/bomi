#include "prefdialog.hpp"
#include "prefdialog_p.hpp"
#include "mbox.hpp"
#include "shortcutdialog.hpp"
#include "player/app.hpp"
#include "player/skin.hpp"
#include "player/pref.hpp"
#include "player/mrlstate.hpp"
#include "misc/simplelistmodel.hpp"
#include "ui_prefdialog.h"

#ifdef None
#undef None
#endif

/******************************************************************************/

struct ValueWatcher {
    QMetaProperty property;
    QQmlProperty editor;
    std::function<bool()> isModified = nullptr;
};

struct PrefDialog::Data {
    PrefDialog *p;
    Ui::PrefDialog ui;
    QButtonGroup *shortcutGroup;
    DataButtonGroup *saveQuickSnapshot;
    QMap<int, QCheckBox*> hwdec;
    QMap<DeintMethod, QCheckBox*> hwdeint;
    QStringList imports;
    MrlStatePropertyListModel *properties = nullptr;
    QVector<ValueWatcher> watchers;
    QSet<ValueWatcher*> modified;
    QHash<QObject*, ValueWatcher*> editorToWatcher;
    Pref orig;

    auto retranslate() -> void
    {
        ui.sub_ext->setItemText(0, tr("All"));
    }

    auto fillEditors(const Pref *pref) -> void
    {
        for (auto &w : watchers) {
            if (!w.editor.isValid())
                continue;
            w.editor.write(w.property.read(pref));
        }
//        d->ui.app_unique->setChecked(cApp.isUnique());
//        d->ui.app_style->setCurrentText(cApp.styleName(), Qt::MatchFixedString);
//        d->ui.locale->setCurrentLocale(cApp.locale());
//        d->modified.clear();
//        setWindowModified(false);
    }

    auto sync() -> void
    {
        for (auto &w : watchers) {
            if (!w.editor.isValid())
                continue;
            w.property.write(&orig, w.editor.read());
            if (w.isModified())
                qDebug() << w.property.typeName() << w.property.name();
        }
    //    d->ui.app_unique->setChecked(cApp.isUnique());
    //    d->ui.app_style->setCurrentText(cApp.styleName(), Qt::MatchFixedString);
    //    d->ui.locale->setCurrentLocale(cApp.locale());
        modified.clear();
        p->setWindowModified(false);
    }
};

PrefDialog::PrefDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
    d->p = this;
    d->properties = new MrlStatePropertyListModel(this);
    d->properties->setObjectName(u"restore_properties"_q);
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

    QTreeWidgetItem *categoryItem = nullptr;
    auto addCategory = [&] (const QString &name) {
        auto item = new QTreeWidgetItem;
        item->setText(0, name);
        item->setData(0, CategoryRole, true);
        item->setFlags(Qt::ItemIsEnabled);
        d->ui.tree->invisibleRootItem()->addChild(item);
        item->setExpanded(true);
        return categoryItem = item;
    };

    auto addPage = [&] (const QString &name, QWidget *widget,
                        const QString &icon) {
        auto item = new QTreeWidgetItem(categoryItem);
        item->setText(0, name);
        item->setIcon(0, QIcon(icon));
        item->setData(0, CategoryRole, false);
        item->setData(0, WidgetRole, QVariant::fromValue(widget));
        return item;
    };

    addCategory(tr("General"));
    addPage(tr("Application"), d->ui.gen_behaviours, u":/img/bomi-32.png"_q)->setSelected(true);
    addPage(tr("Open"), d->ui.open_media, u":/img/document-open-32.png"_q);
    addPage(tr("Playback"), d->ui.playback, u":/img/media-playback-start-32.png"_q);
    addPage(tr("Cache"), d->ui.cache, u":/img/preferences-web-browser-cache.png"_q);
    addPage(tr("Language"), d->ui.language, u":/img/preferences-desktop-locale.png"_q);
    addPage(tr("Miscellaneous"), d->ui.misc, u":/img/applications-education-miscellaneous-32.png"_q);

    addCategory(tr("Appearance"));
    addPage(tr("OSD"), d->ui.osd, u":/img/view-multiple-objects.png"_q);
    addPage(tr("Skin & Style"), d->ui.skin_style, u":/img/preferences-desktop-theme-32.png"_q);

    addCategory(tr("Video"));
    addPage(tr("Hardware acceleration"), d->ui.video_hwacc, u":/img/apps-hardware-icon.png"_q);
    addPage(tr("Video Processing"), d->ui.video_deint, u":/img/tool-animator.png"_q);

    addCategory(tr("Audio"));
    addPage(tr("Sound"), d->ui.audio_sound, u":/img/audio-volume-high.png"_q);
    addPage(tr("Audio filter"), d->ui.audio_filter, u":/img/applications-multimedia.png"_q);
    addPage(tr("Load"), d->ui.audio_load, u":/img/audio-x-generic.png"_q);

    addCategory(tr("Subtitle"));
    addPage(tr("Load"), d->ui.sub_load, u":/img/application-x-subrip-32.png"_q);
    addPage(tr("Display"), d->ui.sub_appearance, u":/img/format-text-color-32.png"_q);

    addCategory(tr("User interface"));
    addPage(tr("Keyboard shortcuts"), d->ui.ui_shortcut, u":/img/preferences-desktop-keyboard-32.png"_q);
    addPage(tr("Mouse actions"), d->ui.ui_mouse, u":/img/input-mouse-32.png"_q);
    addPage(tr("Control step"), d->ui.ui_step, u":/img/run-build-32.png"_q);

    auto vbox = new QVBoxLayout;
    vbox->setMargin(0);

    void(QComboBox::*curIdxChanged)(int) = &QComboBox::currentIndexChanged;
    d->ui.enable_hwaccel->setEnabled(HwAcc::isAvailable());
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

    d->saveQuickSnapshot = new DataButtonGroup(this);
    d->saveQuickSnapshot->setObjectName(u"quick_snapshot_save"_q);
    d->saveQuickSnapshot->setExclusive(true);
    d->saveQuickSnapshot->addButton(d->ui.save_quick_snapshot_folder,
                                    QVariant::fromValue(QuickSnapshotSave::Fixed));
    d->saveQuickSnapshot->addButton(d->ui.save_quick_snapshot_current,
                                    QVariant::fromValue(QuickSnapshotSave::Current));
    d->saveQuickSnapshot->addButton(d->ui.save_quick_snapshot_ask,
                                    QVariant::fromValue(QuickSnapshotSave::Ask));
    d->saveQuickSnapshot->setCurrentData(QVariant::fromValue(QuickSnapshotSave::Current));
    d->ui.quick_snapshot_format->addItems(_ExtList(WritableImageExt));

    vbox = new QVBoxLayout;
    vbox->setMargin(0);

    d->ui.sub_ext->addItem(QString(), QString());
    d->ui.sub_ext->addItemTextData(_ExtList(SubtitleExt));
    d->ui.app_style->addItems(cApp.availableStyleNames());

    d->shortcutGroup = new QButtonGroup(this);
    d->shortcutGroup->addButton(d->ui.shortcut1, 0);
    d->shortcutGroup->addButton(d->ui.shortcut2, 1);
    d->shortcutGroup->addButton(d->ui.shortcut3, 2);
    d->shortcutGroup->addButton(d->ui.shortcut4, 3);

    d->ui.mouse_action_map->setActionList(&d->ui.shortcuts->actionInfoList());

    connect(d->ui.audio_device, curIdxChanged, [this] (int idx)
        { d->ui.audio_device_desc->setText(d->ui.audio_device->itemData(idx).toString()); });

    d->ui.network_folders->setAddingAndErasingEnabled(true);

    auto checkSubAutoselectMode = [this] (const QVariant &data) {
        const bool enabled = data.toInt() == AutoselectMode::Matched;
        d->ui.sub_ext_label->setEnabled(enabled);
        d->ui.sub_ext->setEnabled(enabled);
    };

    d->ui.sub_priority->setAddingAndErasingEnabled(true);
    d->ui.sub_priority->setChangingOrderEnabled(true);
    checkSubAutoselectMode(d->ui.sub_autoselect->currentData());
    d->ui.audio_priority->setAddingAndErasingEnabled(true);
    d->ui.audio_priority->setChangingOrderEnabled(true);

    auto updateSkinPath = [this] (int idx) {
        if (idx >= 0) {
            const auto name = d->ui.skin_name->itemText(idx);
            const auto skin = Skin::source(name);
            d->ui.skin_path->setText(skin.absolutePath());
        }
    };

    d->ui.skin_name->addItems(Skin::names(true));
    updateSkinPath(d->ui.skin_name->currentIndex());

    connect(d->ui.skin_name, curIdxChanged, this, updateSkinPath);

    auto currentDataChanged = &DataComboBox::currentDataChanged;
    connect(d->ui.sub_autoselect, currentDataChanged, checkSubAutoselectMode);
    void(QButtonGroup::*buttonClicked)(int) = &QButtonGroup::buttonClicked;
    connect(d->shortcutGroup, buttonClicked, [this] (int idx) {
        auto treeItem = d->ui.shortcuts->currentItem();
        auto item = static_cast<PrefMenuTreeItem*>(treeItem);
        if (item && !item->isMenu()) {
            ShortcutDialog dlg(item->shortcut(idx), this);
            dlg.setQueryFunction([=] (const QString &id,
                                      const QKeySequence &key)
            {
                auto item = d->ui.shortcuts->item(key);
                if (!item || item->id() == id)
                    return QString();
                return item->description();
            }, item->id());
            if (dlg.exec())
                item->setShortcut(idx, dlg.shortcut());
        }
    });
    connect(d->ui.shortcuts, &QTreeWidget::currentItemChanged,
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
            d->sync();
            emit applyRequested();
            break;
        case BBox::Cancel:
            hide();
        case BBox::Reset:
            d->fillEditors(&d->orig);
            break;
        case BBox::RestoreDefaults: {
            Pref pref;
            d->fillEditors(&pref);
            break;
        } default:
            break;
        }
    });

    static constexpr auto bomi = static_cast<int>(KeyMapPreset::Bomi);
    static constexpr auto movist = static_cast<int>(KeyMapPreset::Movist);
    d->ui.shortcut_preset->addItem(cApp.displayName(), bomi);
    d->ui.shortcut_preset->addItem(tr("Movist"), movist);

    connect(d->ui.load_preset, &QPushButton::clicked, [this] () {
        const int idx = d->ui.shortcut_preset->currentIndex();
        if (idx != -1) {
            const auto data = d->ui.shortcut_preset->itemData(idx).toInt();
            d->ui.shortcuts->set(Pref::preset(static_cast<KeyMapPreset>(data)));
        }
    });

    d->ui.audio_priority->setDragEnabled(true);
    d->ui.sub_priority->setDragEnabled(true);

    d->retranslate();
    d->ui.restore_properties_view->setModel(d->properties);
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

    cApp.setWindowTitle(this, tr("Preferences") % _L("[*]"));

    auto &mo = Pref::staticMetaObject;
    d->watchers.resize(mo.propertyCount() - mo.propertyOffset());
    for (int i = mo.propertyOffset(); i < mo.propertyCount(); ++i) {
        auto &w = d->watchers[i - mo.propertyOffset()];
        w.property = mo.property(i);
        auto editor = findChild<QObject*>(_L(w.property.name()));
        if (!editor) {
            qDebug() << "no editor for" << w.property.name();
            continue;
        }
        d->editorToWatcher[editor] = &w;
        const auto name = "editor_" + QByteArray(w.property.name()) ;
        QString editor_p;
        mo.invokeMethod(&d->orig, name.constData(), Q_RETURN_ARG(QString, editor_p));
        Q_ASSERT(!editor_p.isEmpty());
        w.editor = QQmlProperty(editor, editor_p);
        if (!w.editor.hasNotifySignal())
            qDebug() << "No notify signal in" << editor->metaObject()->className() << "for" << w.property.name();
        else
            w.editor.connectNotifySignal(this, SLOT(checkModified()));
#define CHECK_W(W) if (qobject_cast<W*>(editor)) { w.isModified = [=, &w] () { \
            return !static_cast<W*>(w.editor.object())->compare(w.property.read(&d->orig)); }; continue; }
        CHECK_W(PrefMenuTreeWidget);
        CHECK_W(MrlStatePropertyListModel);
        CHECK_W(PrefMouseActionTree);
        const auto compare = "compare_" + QByteArray(w.property.name()) + "(QVariant)";
        const int idx = mo.indexOfMethod(compare.constData());
        Q_ASSERT(idx != -1);
        const auto method = mo.method(idx);
        w.isModified = [=] () {
            bool ret = false;
            method.invoke(&d->orig, Q_RETURN_ARG(bool, ret), Q_ARG(QVariant, w.editor.read()));
            return !ret;
        };
    }
}

PrefDialog::~PrefDialog() {
    delete d;
}

auto PrefDialog::checkModified() -> void
{
    auto w = d->editorToWatcher.value(sender());
    if (!w)
        return;
    if (w->isModified())
        d->modified.insert(w);
    else
        d->modified.remove(w);
    setWindowModified(!d->modified.isEmpty());
}

auto PrefDialog::setAudioDeviceList(const QList<AudioDevice> &devices) -> void {
    d->ui.audio_device->clear();
    for (auto &dev : devices)
        d->ui.audio_device->addItem(dev.name, dev.description);
    if (!devices.isEmpty()) {
        d->ui.audio_device->setCurrentIndex(0);
        d->ui.audio_device_desc->setText(d->ui.audio_device->itemData(0).toString());
    }
}

auto PrefDialog::set(const Pref *p) -> void
{
    d->fillEditors(p);
    d->sync();
//    for (auto &w : d->watchers) {
//        if (!w.editor.isValid())
//            continue;
//        w.editor.write(w.property.read(p));
//        w.property.write(&d->orig, w.editor.read());
//        if (w.isModified())
//            qDebug() << w.property.typeName() << w.property.name();
//    }
////    d->ui.app_unique->setChecked(cApp.isUnique());
////    d->ui.app_style->setCurrentText(cApp.styleName(), Qt::MatchFixedString);
////    d->ui.locale->setCurrentLocale(cApp.locale());
//    d->modified.clear();
//    setWindowModified(false);
}

auto PrefDialog::get(Pref *p) -> void
{
    for (auto &w : d->watchers)
        w.property.write(p, w.property.read(&d->orig));
//    cApp.setUnique(d->ui.app_unique->isChecked());
//    cApp.setLocale(d->ui.locale->currentLocale());
//    cApp.setStyleName(d->ui.app_style->currentData().toString());
}

auto PrefDialog::changeEvent(QEvent *event) -> void
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
        d->retranslate();
    } else if (event->type() == QEvent::ModifiedChange) {
        const auto modified = isWindowModified();
        d->ui.dbb->button(BBox::Ok)->setEnabled(modified);
        d->ui.dbb->button(BBox::Apply)->setEnabled(modified);
        d->ui.dbb->button(BBox::Reset)->setEnabled(modified);
    }
}

auto PrefDialog::showEvent(QShowEvent *event) -> void
{
    QDialog::showEvent(event);
}
