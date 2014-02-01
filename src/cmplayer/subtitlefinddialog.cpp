#include "subtitlefinddialog.hpp"
#include "global.hpp"
#include "info.hpp"
#include "appstate.hpp"
#include "opensubtitlesfinder.hpp"
#include "mrl.hpp"
#include "ui_subtitlefinddialog.h"
#include "downloader.hpp"

static constexpr int UrlRole = Qt::UserRole + 1;
static constexpr int FileNameRole = UrlRole + 1;

class SubtitleLinkModel : public QAbstractListModel {
public:
	enum Column { Language, FileName, Date, ColumnCount };
	SubtitleLinkModel(QObject *parent = nullptr): QAbstractListModel(parent) { }
	int rowCount(const QModelIndex &parent = QModelIndex()) const { return parent.isValid() ? 0 : m_list.size(); }
	int columnCount(const QModelIndex &parent = QModelIndex()) const { Q_UNUSED(parent); return ColumnCount; }
	void setList(const QList<SubtitleLink> &list) { beginResetModel(); m_list = list; endResetModel(); }
	QVariant headerData(int section, Qt::Orientation orientation, int role) const {
		if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
			return QVariant();
		switch (section) {
			case Language: return tr("Language");
			case FileName: return tr("File Name");
			case Date:     return tr("Date");
			default:       return QVariant();
		}
	}
	QVariant data(const QModelIndex &index, int role) const {
		const int row = index.row(), col = index.column();
		if (!(0 <= row && row < m_list.size() && 0 <= col && col < ColumnCount))
			return QVariant();
		if (role == UrlRole)
			return m_list[row].url;
		if (role == FileNameRole)
			return m_list[row].fileName;
		else if (role != Qt::DisplayRole)
			return QVariant();
		const auto &link = m_list[row];
		switch (col) {
			case Language: return link.language;
			case FileName: return link.fileName;
			case Date:     return link.date;
			default:       return QVariant();
		}
	}
	SubtitleLink link(int row) const { return m_list.value(row); }
private:
	QList<SubtitleLink> m_list;
};

struct SubtitleFindDialog::Data {
	SubtitleFindDialog *p = nullptr;
	Ui::SubtitleFindDialog ui;
	Downloader downloader;
	OpenSubtitlesFinder *finder = nullptr;
	Mrl pending;
	SubtitleLinkModel model;
	QSortFilterProxyModel proxy;
	void updateState() {
		const bool ok = finder->isAvailable() && !downloader.isRunning();
		ui.open->setEnabled(ok);
		ui.state->setEnabled(ok);
		ui.language->setEnabled(ok);
		ui.download->setEnabled(ok);
		ui.view->setEnabled(ok);
		if (ok) {
			ui.prog->setMaximum(1);
			ui.prog->setValue(0);
			if (!pending.isEmpty()) {
				auto mrl = pending;
				pending = Mrl();
				p->find(mrl);
			}
		} else {
			ui.prog->setMaximum(0);
		}
		switch (finder->state()) {
		case OpenSubtitlesFinder::Connecting:
			ui.state->setText(tr("Connecting..."));
			break;
		case OpenSubtitlesFinder::Finding:
			ui.state->setText(tr("Finding..."));
			break;
		default:
			ui.state->setText(tr("Find"));
			break;
		}
	}
};

SubtitleFindDialog::SubtitleFindDialog(QWidget *parent)
: QDialog(parent), d(new Data) {
	d->p = this;
	d->ui.setupUi(this);
	d->proxy.setSourceModel(&d->model);
	d->proxy.setFilterKeyColumn(d->model.Language);
	d->ui.view->setModel(&d->proxy);
	d->ui.view->setRootIsDecorated(false);
	d->finder = new OpenSubtitlesFinder;
	connect(&d->downloader, &Downloader::started, [this] () { d->updateState(); });
	connect(&d->downloader, &Downloader::downloaded, [this] (qint64 written, qint64 total) {
		d->ui.prog->setMaximum(total);
		d->ui.prog->setValue(written);
	});
	connect(&d->downloader, &Downloader::finished, [this] () {
		auto data = _Uncompress(d->downloader.data());
		if (!data.isEmpty()) {
			QFileInfo info(d->ui.file->text());
			QFile file(info.absolutePath() + "/" + d->ui.view->currentIndex().data(FileNameRole).toString());
			file.open(QFile::WriteOnly | QFile::Truncate);
			file.write(data);
			file.close();
			emit loadRequested(file.fileName());
		}
		d->updateState();
	});
	connect(d->ui.open, &QPushButton::clicked, [this] () {
		auto file = _GetOpenFileName(this, tr("Open"), AppState::get().open_last_folder, Info::videoExt().toFilter());
		if (!file.isEmpty())
			find(QUrl::fromLocalFile(file));
	});
	connect(d->ui.language, static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this] (int index) {
		d->proxy.setFilterFixedString(index > 0 ? d->ui.language->itemText(index) : "");
	});
	connect(d->ui.download, &QPushButton::clicked, [this] () {
		auto url = d->ui.view->currentIndex().data(UrlRole).toUrl();
		d->downloader.start(url);
	});
	connect(d->finder, &OpenSubtitlesFinder::stateChanged, [this] () { d->updateState(); });
	connect(d->finder, &OpenSubtitlesFinder::found, [this] (const QList<SubtitleLink> &links) {
		d->model.setList(links);
		std::set<QString> set;
		for (auto &it : links)
			set.insert(it.language);
		d->ui.language->clear();
		d->ui.language->addItem(tr("All"));
		d->ui.language->insertSeparator(1);
		for (auto &it : set)
			d->ui.language->addItem(it);
	});
	d->updateState();
}

SubtitleFindDialog::~SubtitleFindDialog() {
	delete d->finder;
	delete d;
}

void SubtitleFindDialog::find(const Mrl &mrl) {
	d->ui.file->setText(mrl.toLocalFile());
	if (!d->finder->isAvailable()) {
		d->pending = mrl;
	} else {
		if (!d->finder->find(mrl))
			QMessageBox::warning(this, tr("Find Subtitle"), tr("Cannot find subtitles for %1.").arg(mrl.displayName()));
	}
}
