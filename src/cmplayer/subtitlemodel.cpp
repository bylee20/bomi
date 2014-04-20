#include "subtitlemodel.hpp"
#include "global.hpp"

#define ITEM(row) (static_cast<SubCompModel::Item*>(at(row)))
#define C_ITEM(row) (static_cast<const SubCompModel::Item*>(at(row)))

class SubCompModel::Item : public ListModel::Item {
public:
    Item(): m_end(-1) {}
    Item(c_iterator it): m_end(-1), m_it(it) {}
    int start() const {return m_it.key();}
    int end() const {return m_end;}
    QString text() const {return m_it->toPlainText();}
    QVariant data(const int column, int role) const {
        if (role == Qt::DisplayRole) {
            switch (column) {
            case Start:
                return _MSecToString(start());
            case End:
                return _MSecToString(end());
            case Text:
                return text();
            default:
                return QVariant();
            }
        } else if (role == Qt::FontRole && column == Text)
            return m_font;
        return QVariant();
    }
    void setFont(const QFont &font) {
        if (m_font != font) {
            m_font = font;
            emitDataChanged(Text);
        }
    }

    int m_end;
    c_iterator m_it;
    QFont m_font;
};

struct SubCompModel::Data {
    int curRow;
    QFont curFont, defFont;
    bool visible;
    const SubComp *comp;
    const SubCapt *pended;
};

SubCompModel::SubCompModel(const SubComp *comp, QObject *parent)
: ListModel(ColumnCount, parent), d(new Data) {
    d->comp = comp;
    d->curRow = -1;
    d->visible = false;
    d->pended = 0;
    d->curFont.setBold(true);
    d->curFont.setItalic(true);

    c_iterator it = comp->begin();
    for (; it != comp->end(); ++it) {
        if (it->hasWords()) {
            it->index = 0;
            append(new Item(it));
            break;
        }
    }
    if (!isEmpty()) {
        for (++it; it != comp->end(); ++it) {
            Item *last = static_cast<Item*>(this->last());
            if (last->m_end < 0)
                last->m_end = it.key();
            if (it->hasWords())
                append(new Item(it));
            it->index = size() - 1;
        }
    }

    setColumnTitle(Start, tr("Start"));
    setColumnTitle(End, tr("End"));
    setColumnTitle(Text, tr("Text"));
}

QString SubCompModel::name() const {
    return d->comp->name();
}

void SubCompModel::setVisible(bool visible) {
    if (d->visible != visible) {
        d->visible = visible;
        if (d->visible && d->pended)
            setCurrentCaption(d->pended);
    }
}

void SubCompModel::setCurrentCaption(const SubCapt *caption) {
    if (!d->visible) {
        d->pended = caption;
    } else {
        d->pended = 0;
        const int row = caption ? caption->index : -1;
        if (d->curRow == row)
            return;
        const int old = d->curRow;
        d->curRow = row;
        if (isValidRow(d->curRow))
            ITEM(d->curRow)->setFont(d->curFont);
        if (isValidRow(old))
            ITEM(old)->setFont(d->defFont);
        emit currentRowChanged(d->curRow);
    }
}

int SubCompModel::currentRow() const {
    return d->curRow;
}

/**********************************************************************/

struct SubtitleComponentView::Data {
    SubCompModel *model;
    bool autoScroll;
};

SubtitleComponentView::SubtitleComponentView(QWidget *parent)
: QTreeView(parent), d(new Data) {
    d->model = 0;
    d->autoScroll = false;
    setAlternatingRowColors(true);
    setRootIsDecorated(false);
}

void SubtitleComponentView::setModelToNull() {
    if (sender() == d->model)
        d->model = 0;
}

void SubtitleComponentView::setModel(QAbstractItemModel *model) {
    if (d->model)
        d->model->disconnect(this);
    d->model = qobject_cast<SubCompModel*>(model);
    QTreeView::setModel(d->model);
    if (d->model) {
        connect(d->model, SIGNAL(destroyed()), this, SLOT(setModelToNull()));
        connect(d->model, SIGNAL(currentRowChanged(int)), this, SLOT(updateCurrentRow(int)));
        d->model->setVisible(isVisible());
        for (int i=0; i<SubCompModel::ColumnCount; ++i)
            resizeColumnToContents(i);
    }
}

void SubtitleComponentView::updateCurrentRow(int row) {
    if (!d->model || !d->autoScroll)
        return;
    const QModelIndex idx = d->model->index(row, SubCompModel::Text);
    if (idx.isValid())
        scrollTo(idx);
}

void SubtitleComponentView::setAutoScrollEnabled(bool enabled) {
    if (d->autoScroll != enabled) {
        d->autoScroll = enabled;
        if (d->autoScroll && d->model)
            updateCurrentRow(d->model->currentRow());
    }
}

void SubtitleComponentView::setTimeVisible(bool visible) {
    setColumnHidden(SubCompModel::Start, !visible);
    setColumnHidden(SubCompModel::End, !visible);
}

void SubtitleComponentView::showEvent(QShowEvent *event) {
    QTreeView::showEvent(event);
    if (d->model)
        d->model->setVisible(true);
}

void SubtitleComponentView::hideEvent(QHideEvent *event) {
    QTreeView::hideEvent(event);
    if (d->model)
        d->model->setVisible(false);
}

#undef ITEM
#undef C_ITEM
