#include "prefwidgets.hpp"
#include "video/hwacc.hpp"


HwAccCodecBox::HwAccCodecBox(QWidget *parent)
    : QGroupBox(parent)
{
    auto vbox = new QVBoxLayout;
//    for (const auto codec : HwAcc::fullCodecList())
//        vbox->addWidget(m_checks[codec] = new QCheckBox);
    setLayout(vbox);
}

auto HwAccCodecBox::value() const -> QVector<int>
{
    QVector<int> list;
    for (auto it = m_checks.begin(); it != m_checks.end(); ++it)
        if (it.value()->isChecked())
            list.push_back(it.key());
    return list;
}

auto HwAccCodecBox::setValue(const QVector<int> &list) -> void
{
    for (auto it = m_checks.begin(); it != m_checks.end(); ++it)
        it.value()->setChecked(list.contains(it.key()));
}

auto HwAccCodecBox::setBackend(int t) -> void
{
    const auto type = static_cast<HwAcc::Type>(t);
//    const auto codecs = HwAcc::fullCodecList();
//    for (const auto codec : codecs) {
//        auto box = m_checks[codec];
//        const auto supported = HwAcc::supports(type, codec);
//        const QString desc(_L(avcodec_descriptor_get(codec)->long_name));
//        if (supported)
//            box->setText(desc);
//        else
//            box->setText(desc % ' '_q % tr("Not supported") % ')'_q);
//        box->setEnabled(supported);
//    }
}



//template<class T>
//static void setHw(QGroupBox *group, bool enabled,
//                  QMap<T, QCheckBox*> &map, const QVector<T> &keys) {
//    group->setChecked(enabled);
//    for (auto key : keys) {
//        if (auto ch = map.value(key))
//            ch->setChecked(true);
//    }
//}

//template<class T>
//static void getHw(bool &enabled, QGroupBox *group,
//                  QVector<T> &keys, const QMap<T, QCheckBox*> &map) {
//    enabled = group->isChecked();
//    keys.clear();
//    for (auto it = map.begin(); it != map.end(); ++it) {
//        if (*it && (*it)->isChecked())
//            keys << it.key();
//    }
//}
/******************************************************************************/

DataButtonGroup::DataButtonGroup(QObject *parent)
    : QButtonGroup(parent)
{
    connect(this, static_cast<Signal<QButtonGroup, int>>(&QButtonGroup::buttonClicked), this, [=] () {
        if (_Change(m_button, checkedButton()))
            emit currentDataChanged(currentData());
    });
}

auto DataButtonGroup::addButton(QAbstractButton *button, const QVariant &data) -> void
{
    QButtonGroup::addButton(button);
    m_data[button] = data;
}

auto DataButtonGroup::button(const QVariant &data) const -> QAbstractButton*
{
    for (auto it = m_data.begin(); it != m_data.end(); ++it) {
        if (*it == data)
            return it.key();
    }
    return nullptr;
}

auto DataButtonGroup::setCurrentData(const QVariant &data) -> void
{
    auto button = this->button(data);
    if (button)
        button->setChecked(true);
    if (_Change(m_button, button))
        emit currentDataChanged(currentData());
}
