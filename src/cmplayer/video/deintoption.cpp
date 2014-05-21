#include "deintoption.hpp"

auto DeintOption::toString() const -> QString
{
    return DeintMethodInfo::name(method) % "|"
           % _N(doubler) % "|" % DeintDeviceInfo::name(device);
}

auto DeintOption::fromString(const QString &string) -> DeintOption
{
    QStringList tokens = string.split("|", QString::SkipEmptyParts);
    if (tokens.size() != 3)
        return DeintOption();
    DeintOption opt;
    opt.method = DeintMethodInfo::from(tokens[0]);
    opt.doubler = tokens[1].toInt();
    opt.device = DeintDeviceInfo::from(tokens[2]);
    return opt;
}


