#include "favoritesview.hpp"
#include "playengine.hpp"
#include <QtGui/QTreeWidget>
#include <QtCore/QSettings>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QMenu>
#include <QtGui/QDialog>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QDialogButtonBox>
#include <QtGui/QCheckBox>

class FavoritesView::Item : public QTreeWidgetItem {
public:
	virtual bool isFolder() const = 0;
	QString name() const {return text(0);}
	void setName(const QString &name) {setText(0, name);}
};

class FavoritesView::FolderItem : public FavoritesView::Item {
	Q_DECLARE_TR_FUNCTIONS(FavoritesView::FolderItem)
public:
	FolderItem(const QString &name = QString()) {
		setName(name);
		setText(1, tr("Folder"));
	}
	bool isFolder() const {return true;}
};

class FavoritesView::MrlItem : public FavoritesView::Item {
	Q_DECLARE_TR_FUNCTIONS(FavoritesView::MrlItem)
public:
	MrlItem() {}
	MrlItem(const Mrl &mrl, const QString &name = QString()) {
		setName(name);
		setMrl(mrl);
	}
	bool isFolder() const {return false;}
	void setMrl(const Mrl &mrl) {
		if (m_mrl != mrl) {
			if (mrl.isPlaylist())
				setText(1, tr("Playlist"));
			else
				setText(1, tr("Media"));
			setText(2, mrl.toString());
			m_mrl = mrl;
		}
	}
	const Mrl &mrl() const {return m_mrl;}
private:
	Mrl m_mrl;
};

class FavoritesView::ItemDialog : public QDialog {
	Q_DECLARE_TR_FUNCTIONS(FavoritesView::ItemDialog)
public:
	ItemDialog(Item *item, QWidget *parent)
	: QDialog(parent), m_folder(item->isFolder()) {init(item);}
	ItemDialog(bool folder, QWidget *parent)
	: QDialog(parent), m_folder(folder) {init();}
	bool isTopLevel() const {return m_top->isChecked();}
	bool isFolder() const {return m_folder;}
	void setLocation(const QString &location) {if (!m_folder) m_loc->setText(location);}
	void setName(const QString &name) {m_name->setText(name);}
	QString name() const {return m_name->text();}
	QString location() const {return m_folder ? QString() : m_loc->text();}
private:
	void init(Item *item = 0) {
		QGridLayout *grid = new QGridLayout;
		m_name = new QLineEdit(this);
		grid->addWidget(new QLabel(tr("Name"), this), 0, 0);
		grid->addWidget(m_name, 0, 1);
		if (!m_folder) {
			m_loc = new QLineEdit(this);
			grid->addWidget(new QLabel(tr("Location"), this), 1, 0);
			grid->addWidget(m_loc, 1, 1);
		}
		m_top = new QCheckBox(tr("Add to top level"), this);
		m_top->setVisible(!item);
		QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel
			, Qt::Horizontal, this);
		QVBoxLayout *vbox = new QVBoxLayout(this);
		vbox->addLayout(grid);
		vbox->addWidget(m_top);
		vbox->addWidget(buttons);
		connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
		connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
		if (item) {
			setName(item->name());
			if (!item->isFolder())
				setLocation(static_cast<MrlItem*>(item)->mrl().toString());
		}
	}
	QLineEdit *m_name, *m_loc;
	QCheckBox *m_top;
	bool m_folder;
};

struct FavoritesView::Data {
	QTreeWidget *tree;
	QMenu *context;
	PlayEngine *engine;
};

FavoritesView::FavoritesView(PlayEngine *engine, QWidget *parent)
: QWidget(parent), d(new Data) {
	setStyleSheet("\
		QTreeView::branch:has-children:!has-siblings:closed,\
		QTreeView::branch:closed:has-children:has-siblings {\
			border-image: none;\
			image: url(:/img/folder.png);\
		}\
		QTreeView::branch:open:has-children:!has-siblings,\
		QTreeView::branch:open:has-children:has-siblings {\
			border-image: none;\
			image: url(:/img/document-open-folder.png);\
		}"
	);
	setContextMenuPolicy(Qt::CustomContextMenu);
	d->engine = engine;
	d->tree = new QTreeWidget(this);

	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->addWidget(d->tree);

	d->context = new QMenu(this);
	QAction *addNew = d->context->addAction(tr("Add New Media"));
	QAction *addCur = d->context->addAction(tr("Add Current Media"));
	QAction *addFolder = d->context->addAction(tr("Add Folder"));
	QAction *modify = d->context->addAction(tr("Modify"));
	QAction *erase = d->context->addAction(tr("Erase"));

	d->tree->setHeaderLabels(QStringList() << tr("Name") << tr("Category") << tr("Location"));
	connect(addNew, SIGNAL(triggered()), this, SLOT(addNew()));
	connect(addCur, SIGNAL(triggered()), this, SLOT(addCurrent()));
	connect(addFolder, SIGNAL(triggered()), this, SLOT(addFolder()));
	connect(erase, SIGNAL(triggered()), this, SLOT(erase()));
	connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContext(QPoint)));
	connect(d->tree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int))
		, this, SLOT(slotDblClick(QTreeWidgetItem*)));
	connect(modify, SIGNAL(triggered()), this, SLOT(modify()));

	load();

	setWindowTitle(tr("Favorites"));
}

FavoritesView::~FavoritesView() {
	save();
	delete d;
}

void FavoritesView::addItem(Item *item, bool topLevel) {
	QTreeWidgetItem *parent = d->tree->invisibleRootItem();
	if (!topLevel) {
		Item *item = static_cast<Item*>(d->tree->currentItem());
		if (item)
			parent = item->isFolder() ? item : item->parent();
	}
	if (!parent)
		parent = d->tree->invisibleRootItem();
	parent->addChild(item);
}

void FavoritesView::addNew() {
	ItemDialog dlg(false, this);
	if (dlg.exec())
		addItem(new MrlItem(dlg.location(), dlg.name()), dlg.isTopLevel());
}

void FavoritesView::addFolder() {
	ItemDialog dlg(true, this);
	if (dlg.exec())
		addItem(new FolderItem(dlg.name()), dlg.isTopLevel());
}

void FavoritesView::addCurrent() {
	ItemDialog dlg(false, this);
	const Mrl mrl = d->engine->mrl();
	dlg.setName(mrl.isLocalFile() ? mrl.fileName() : mrl.toString());
	dlg.setLocation(mrl.toString());
	if (dlg.exec())
		addItem(new MrlItem(dlg.location(), dlg.name()), dlg.isTopLevel());
}

void FavoritesView::erase() {
	delete d->tree->currentItem();
}

void FavoritesView::showContext(const QPoint &pos) {
	d->context->exec(mapToGlobal(pos));
}

void FavoritesView::slotDblClick(QTreeWidgetItem *i) {
	if (!i)
		return;
	Item *item = static_cast<Item*>(i);
	if (item->isFolder())
		item->setExpanded(item->isExpanded());
	else {
		d->engine->stop();
		d->engine->setMrl(static_cast<MrlItem*>(item)->mrl());
		d->engine->play();
	}
}

void FavoritesView::modify(Item *item) {
	if (!item)
		item = static_cast<Item*>(d->tree->currentItem());
	if (item && item != d->tree->invisibleRootItem()) {
		ItemDialog dlg(item, this);
		if (dlg.exec()) {
			item->setName(dlg.name());
			if (!item->isFolder())
				static_cast<MrlItem*>(item)->setMrl(Mrl(dlg.location()));
		}
	}
}

void FavoritesView::save() const {
	QSettings set;
	set.beginGroup("favorites");
	save(d->tree->invisibleRootItem(), &set);
	set.endGroup();
}

void FavoritesView::save(QTreeWidgetItem *item, QSettings *set) const {
	if (item != d->tree->invisibleRootItem()) {
		Item *itm = static_cast<Item*>(item);
		set->setValue("name", itm->name());
		set->setValue("is-folder", itm->isFolder());
		if (!itm->isFolder())
			set->setValue("mrl", static_cast<MrlItem*>(itm)->mrl().toString());
	}
	const int count = item->childCount();
	set->beginWriteArray("items", count);
	for (int i=0; i<count; ++i) {
		set->setArrayIndex(i);
		save(item->child(i), set);
	}
	set->endArray();
}

void FavoritesView::load() {
	QSettings set;
	set.beginGroup("favorites");
	load(d->tree->invisibleRootItem(), &set);
	set.endGroup();
}

void FavoritesView::load(QTreeWidgetItem *parent, QSettings *set) {
	const int count = set->beginReadArray("items");
	for (int i=0; i<count; ++i) {
		set->setArrayIndex(i);
		const bool isFolder = set->value("is-folder", false).toBool();
		const QString name = set->value("name", QString()).toString();
		Item *item = 0;
		if (isFolder) {
			item = new FolderItem(name);
		} else {
			const Mrl mrl = set->value("url", QString()).toString();
			item = new MrlItem(mrl, name);
		}
		parent->addChild(item);
		load(item, set);
	}
	set->endArray();
}
