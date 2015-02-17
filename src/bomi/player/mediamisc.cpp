#include "mediamisc.hpp"

auto MediaObject::typeText() const -> QString
{
    switch (m_type) {
    case File:
        return tr("File");
    case Url:
        return tr("URL");
    case Dvd:
        return tr("DVD");
    case Bluray:
        return tr("Blu-ray");
    default:
        return tr("No Media");
    }
}

EditionChapterObject::~EditionChapterObject()
{

}

auto EditionChapterObject::setRate(qreal rate) -> void
{
    if (_Change(m.rate, rate))
        emit rateChanged();
}
