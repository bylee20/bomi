#include "mousebehavior.hpp"

const std::array<MouseBehaviorInfo::Item, 13> MouseBehaviorInfo::info{{
    {MouseBehavior::DoubleClick, u"DoubleClick"_q, u""_q, -1},
    {MouseBehavior::MiddleClick, u"MiddleClick"_q, u""_q, Qt::MiddleButton},
    {MouseBehavior::ScrollUp, u"ScrollUp"_q, u""_q, -2},
    {MouseBehavior::ScrollDown, u"ScrollDown"_q, u""_q, -3},
    {MouseBehavior::Extra1Click, u"Extra1Click"_q, u""_q, Qt::ExtraButton1},
    {MouseBehavior::Extra2Click, u"Extra2Click"_q, u""_q, Qt::ExtraButton2},
    {MouseBehavior::Extra3Click, u"Extra3Click"_q, u""_q, Qt::ExtraButton3},
    {MouseBehavior::Extra4Click, u"Extra4Click"_q, u""_q, Qt::ExtraButton4},
    {MouseBehavior::Extra5Click, u"Extra5Click"_q, u""_q, Qt::ExtraButton5},
    {MouseBehavior::Extra6Click, u"Extra6Click"_q, u""_q, Qt::ExtraButton6},
    {MouseBehavior::Extra7Click, u"Extra7Click"_q, u""_q, Qt::ExtraButton7},
    {MouseBehavior::Extra8Click, u"Extra8Click"_q, u""_q, Qt::ExtraButton8},
    {MouseBehavior::Extra9Click, u"Extra9Click"_q, u""_q, Qt::ExtraButton9}
}};
