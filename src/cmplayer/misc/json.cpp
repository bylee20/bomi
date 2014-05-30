#include "json.hpp"

auto json_io(const QPoint*) -> const decltype(JSON_IO_POINT(QPoint))*
{ static const auto io = JSON_IO_POINT(QPoint); return &io; }

auto json_io(const QPointF *) -> const decltype(JSON_IO_POINT(QPointF))*
{ static const auto io = JSON_IO_POINT(QPointF); return &io; }

auto json_io(const QSize*) -> const decltype(JSON_IO_SIZE(QSize))*
{ static const auto io = JSON_IO_SIZE(QSize); return &io; }

auto json_io(const QSizeF*) -> const decltype(JSON_IO_SIZE(QSizeF))*
{ static const auto io = JSON_IO_SIZE(QSizeF); return &io; }

auto json_io(const QStringList*) -> const JsonArrayIO<QString, QStringList>*
{ static const JsonArrayIO<QString, QStringList> io{}; return &io; }
