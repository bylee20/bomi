//#ifndef SIMPLELISTDELEGATE_HPP
//#define SIMPLELISTDELEGATE_HPP

//#include "simplelistmodel.hpp"
//#include <QStyledItemDelegate>

//struct ModelDataInfo {

//};

//template<class T>
//class SimpleListDelegate : public QStyledItemDelegate {
//public:
//    SimpleListDelegate(QObject *parent = nullptr)
//        : QStyledItemDelegate(parent) { }
//    auto editor(int row, int column) -> QWidget*;
//private:
//    auto createEditor(QWidget *parent, const QStyleOptionViewItem &/*option*/,
//                      const QModelIndex &index) const -> QWidget*
//    {
//        auto bm = qobject_cast<const SimpleListModelBase*>(index.model());
//        if (!bm)
//            return nullptr;
//        auto m = static_cast<const SimpleListModel<T>*>(bm);

////        switch (index.column()) {
////        case 0: {
////            auto edit = new QLineEdit(parent);
////            edit->setValidator(new SubSearchPathValidator(edit));
////            return edit;
////        } case 1: {
////            auto cbox = new QComboBox(parent);
////            cbox->addItems(QStringList() << tr("Text") << tr("RegEx"));
////            return cbox;
////        } case 2: {
////            auto cbox = new QComboBox(parent);
////            cbox->addItems(QStringList() << tr("Yes") << tr("No"));
////            return cbox;
////        } default:
////            return nullptr;
////        }
//    }
//    auto setEditorData(QWidget *editor, const QModelIndex &index) const -> void
//    {
////        auto m = qobject_cast<const SubSearchPathModel*>(index.model());
////        if (!editor || !m || !m->isValid(index))
////            return;
////        const auto &t = m->at(index.row());
////        switch (index.column()) {
////        case 0:
////            static_cast<QLineEdit*>(editor)->setText(t.string());
////            break;
////        case 1:
////            static_cast<QComboBox*>(editor)->setCurrentIndex(t.isRegEx());
////            break;
////        case 2:
////            static_cast<QComboBox*>(editor)->setCurrentIndex(t.isCaseInsensitive());
////            break;
////        default: break;
////        }
//    }
//    auto setModelData(QWidget *editor, QAbstractItemModel *model,
//                      const QModelIndex &index) const -> void
//    {
////        auto m = qobject_cast<SubSearchPathModel*>(model);
////        if (!editor || !m || !m->isValid(index))
////            return;
////        int r = index.row(), c = index.column();
////        switch (index.column()) {
////        case 0:
////            m->edit(r, c, static_cast<QLineEdit*>(editor)->text());
////            break;
////        case 1: case 2:
////            m->edit(r, c, !!static_cast<QComboBox*>(editor)->currentIndex());
////            break;
////        default:
////            break;
////        }
//    }
//    auto updateEditorGeometry(QWidget *w, const QStyleOptionViewItem &opt,
//                              const QModelIndex &/*index*/) const -> void
//    {
//        if (w)
//            w->setGeometry(opt.rect);
//    }
//};

//#endif // SIMPLELISTDELEGATE_HPP
