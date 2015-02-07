#include "deintoption.hpp"
#include "deintcaps.hpp"
#include "misc/json.hpp"

#define JSON_CLASS DeintOption
static const auto jioOpt = JIO(JE(method), JE(processor), JE(doubler));
JSON_DECLARE_FROM_TO_FUNCTIONS_IO(jioOpt)

auto DeintOption::toString() const -> QString
{
    return _EnumName(method) % '|'_q % _N(doubler) % '|'_q % _EnumName(processor);
}

auto DeintOption::fromString(const QString &string) -> DeintOption
{
    QStringList tokens = string.split('|'_q, QString::SkipEmptyParts);
    if (tokens.size() != 3)
        return DeintOption();
    DeintOption opt;
    opt.method = _EnumFrom(tokens[0], opt.method);
    opt.doubler = tokens[1].toInt();
    opt.processor = _EnumFrom(tokens[2], opt.processor);
    return opt;
}

#undef JSON_CLASS
#define JSON_CLASS DeintOptionSet
static const auto jio = JIO(JE(swdec), JE(hwdec));
JSON_DECLARE_FROM_TO_FUNCTIONS

DeintOptionSet::DeintOptionSet()
    : hwdec(DeintCaps::defaultOption(Processor::GPU))
    , swdec(DeintCaps::defaultOption(Processor::CPU))
{

}
