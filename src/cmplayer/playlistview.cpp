#include "playlistview.hpp"
#include "playengine.hpp"
#include "info.hpp"
#include "playlist.hpp"
#include "dialogs.hpp"
#include "pref.hpp"
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtGui/QTreeView>
#include "playlistmodel.hpp"
#include <QtGui/QVBoxLayout>
#include <QtGui/QMenu>

PlaylistView *PlaylistView::obj = 0;

struct PlaylistView::Data {
	PlayEngine *engine;
	QTreeView *view;
	PlaylistModel *model;
	QMenu *context;
};

PlaylistView::PlaylistView(PlayEngine *engine, QWidget *parent)
: ToggleDialog(parent), d(new Data) {
	Q_ASSERT(obj == 0);
	obj = this;
	d->engine = engine;
	d->view = new QTreeView(this);
	d->view->setContextMenuPolicy(Qt::CustomContextMenu);
	d->model = new PlaylistModel(this);
	d->view->setModel(d->model);
	d->view->setRootIsDecorated(false);
	d->view->setSelectionMode(QTreeView::ExtendedSelection);
	d->view->setAlternatingRowColors(true);
	d->view->setVerticalScrollMode(QTreeView::ScrollPerPixel);
	d->view->setHorizontalScrollMode(QTreeView::ScrollPerPixel);

	d->context = new QMenu(this);
	QAction *open = d->context->addAction(tr("Open"));
	QAction *save = d->context->addAction(tr("Save"));
	QAction *addFile = d->context->addAction(tr("Add File"));
	QAction *addUrl = d->context->addAction(tr("Add URL"));
	QAction *erase = d->context->addAction(tr("Erase"));
	QAction *up = d->context->addAction(tr("Up"));
	QAction *down = d->context->addAction(tr("Down"));
	QAction *clear = d->context->addAction(tr("Clear"));

	connect(open, SIGNAL(triggered()), this, SLOT(open()));
	connect(save, SIGNAL(triggered()), this, SLOT(save()));
	connect(addFile, SIGNAL(triggered()), this, SLOT(addFile()));
	connect(addUrl, SIGNAL(triggered()), this, SLOT(addUrl()));
	connect(erase, SIGNAL(triggered()), this, SLOT(erase()));
	connect(up, SIGNAL(triggered()), this, SLOT(up()));
	connect(down, SIGNAL(triggered()), this, SLOT(down()));
	connect(clear, SIGNAL(triggered()), this, SLOT(clear()));

	connect(d->view, SIGNAL(customContextMenuRequested(QPoint))
		, this, SLOT(showContextMenu(QPoint)));

	QVBoxLayout *vbox = new QVBoxLayout(this);
	vbox->addWidget(d->view);
	vbox->setSpacing(0);
	vbox->setMargin(0);

	connect(d->engine, SIGNAL(mrlChanged(Mrl)), this, SLOT(updateCurrentMrl(Mrl)));
	connect(d->engine, SIGNAL(finished(Mrl)), this, SLOT(handleFinished()));
	connect(d->view, SIGNAL(doubleClicked(QModelIndex))
		, this, SLOT(handleDoubleClick(QModelIndex)));

	setWindowTitle(tr("Playlist"));
}

PlaylistView::~PlaylistView() {
	delete d;
	obj = 0;
}

void PlaylistView::addFile() {
	const QString filter = Info::mediaExtFilter();
	QStringList files = getOpenFileNames(this, tr("Open File"), QString(), filter);
	Playlist list;
	for (int i=0; i<files.size(); ++i)
		list << Mrl(files[i]);
	d->model->append(list);
}

const PlaylistModel *PlaylistView::model() const {
	return d->model;
}

void PlaylistView::addUrl() {
//	GetUrlDialog dlg(this);
//	if (dlg.exec()) {
//		const Mrl mrl = dlg.url();
//		if (mrl.isPlaylist()) {
//			Playlist list;
//			list.load(mrl, dlg.encoding());
//			d->model->append(list);
//		} else
//			d->model->append(mrl);
//	}
}

void PlaylistView::erase() {
	d->model->erase(d->view->selectionModel()->selectedIndexes());
}

void PlaylistView::up() {
	move(true);
}

void PlaylistView::down() {
	move(false);
}

void PlaylistView::move(bool up) {
	const int row = d->view->currentIndex().row();
	const int target = up ? row-1 : row + 1;
	d->model->swap(row, target);
}

void PlaylistView::clear() {
	d->model->clear();
}

void PlaylistView::open() {
	QString enc;
	const QString filter = tr("Playlist") +' '+ Info::playlistExt().toFilter();
	const QString file = EncodingFileDialog::getOpenFileName(this
		, tr("Open File"), QString(), filter, &enc);
	if (!file.isEmpty()) {
		Playlist list;
		list.load(file, enc);
		setPlaylist(list);
	}
}

void PlaylistView::save() {
	const Playlist &list = d->model->playlist();
	if (list.isEmpty())
		return;
	QString file = getSaveFileName(this, tr("Save File"), QString(), tr("Playlist") + " (*.pls)");
	if (!file.isEmpty()) {
		if (QFileInfo(file).suffix().compare("pls", Qt::CaseInsensitive) != 0)
			file += ".pls";
		list.save(file);
	}
}

void PlaylistView::showContextMenu(const QPoint &/*pos*/) {
	d->context->exec(QCursor::pos());
}

Playlist PlaylistView::generatePlaylist(const Mrl &mrl) {
	const Enum::GeneratePlaylist mode = Pref::get().generate_playlist;
	if (!mrl.isLocalFile() || mode == Enum::GeneratePlaylist::None)
		return Playlist(mrl);
	const QFileInfo file(mrl.toLocalFile());
	const QDir dir = file.dir();
	if (mode == Enum::GeneratePlaylist::Folder)
		return Playlist().loadAll(dir);
	const QStringList filter = Info::mediaNameFilter();
	const QFileInfoList files = dir.entryInfoList(filter, QDir::Files, QDir::Name);
	const QString fileName = file.fileName();
	Playlist list;
	bool prefix = false, suffix = false;
	QFileInfoList::const_iterator it = files.begin();
	for(; it != files.end(); ++it) {
		static QRegExp rxs("(\\D*)\\d+(.*)");
		static QRegExp rxt("(\\D*)\\d+(.*)");
		if (rxs.indexIn(fileName) == -1)
			continue;
		if (rxt.indexIn(it->fileName()) == -1)
			continue;
		if (!prefix && !suffix) {
			if (rxs.cap(1) == rxt.cap(1))
				prefix = true;
			else if (rxs.cap(2) == rxt.cap(2))
				suffix = true;
			else
				continue;
		} else if (prefix) {
			if (rxs.cap(1) != rxt.cap(1))
				continue;
		} else if (suffix) {
			if (rxs.cap(2) != rxt.cap(2))
				continue;
		}
		list.append(it->absoluteFilePath());
	}
	if (list.isEmpty())
		return Playlist(mrl);
	return list;
}

void PlaylistView::load(const Mrl &mrl, const QString &enc) {
	Playlist list;
	if (mrl.isPlaylist())
		list.load(mrl, enc);
	else
		list = generatePlaylist(mrl);
	setPlaylist(list);
}

const Playlist &PlaylistView::playlist() const {
	return d->model->playlist();
}

void PlaylistView::setPlaylist(const Playlist &list) {
	d->model->setPlaylist(list);
	d->view->resizeColumnToContents(PlaylistModel::Name);
	d->view->resizeColumnToContents(PlaylistModel::Location);
}

void PlaylistView::play(const Mrl &mrl) {
	d->engine->stop();
	d->engine->setMrl(mrl);
	d->engine->play();
}

void PlaylistView::playNext() {
	const Mrl mrl = d->model->next();
	if (!mrl.isEmpty())
		play(mrl);
}

void PlaylistView::playPrevious() {
	const Mrl mrl = d->model->previous();
	if (!mrl.isEmpty())
		play(mrl);
}

void PlaylistView::updateCurrentMrl(const Mrl &mrl) {
	d->model->setCurrentRow(d->model->rowOf(mrl));
}

void PlaylistView::handleFinished() {
	const Mrl mrl = d->model->next();
	if (mrl.isEmpty())
		emit finished();
	else
		play(mrl);
}

void PlaylistView::handleDoubleClick(const QModelIndex &index) {
	const Mrl mrl = d->model->mrl(index.row());
	if (!mrl.isEmpty())
		play(mrl);
}

void PlaylistView::append(const Playlist &list) {
	d->model->append(list);
}

void PlaylistView::merge(const Playlist &list) {
	d->model->merge(list);
}
