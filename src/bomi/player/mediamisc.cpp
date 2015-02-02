#include "mediamisc.hpp"

auto EditionChapterObject::setRate(qreal rate) -> void
{
    if (_Change(m.rate, rate))
        emit rateChanged();
}
