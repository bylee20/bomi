#include "verticallabel.hpp"

auto VerticalLabel::changeEvent(QEvent *event) -> void
{
    if (event->type() == QEvent::FontChange)
        recalc();
}

auto VerticalLabel::paintEvent(QPaintEvent *event) -> void
{
    QFrame::paintEvent(event);
    QPainter painter(this);
    painter.translate((width())*0.5, (height())*0.5);
    painter.rotate(-90);
    QPoint p;
    auto m_alignment = Qt::AlignVCenter | Qt::AlignRight;
    switch (m_alignment & Qt::AlignVertical_Mask) {
    case Qt::AlignTop:
        p.ry() = -width()*0.5;
        break;
    case Qt::AlignBottom:
        p.ry() = width()*0.5 - m_size.height();
        break;
    default:
        p.ry() = m_size.height()*0.5;
        break;
    }
    switch (m_alignment & Qt::AlignHorizontal_Mask) {
    case Qt::AlignLeft:
        p.rx() = -height()*0.5;
        break;
    case Qt::AlignRight:
        p.rx() = height()*0.5 - m_size.width();
        break;
    default:
        p.rx() = -m_size.width()*0.5;
        break;
    }

    painter.drawText(p, m_text);
}

auto VerticalLabel::recalc() -> void
{
    m_size = fontMetrics().boundingRect(m_text).size();
    updateGeometry();
    update();
}
