#ifndef DEINTINFO_HPP
#define DEINTINFO_HPP

#include "enum/deintmethod.hpp"
#include "enum/processor.hpp"

struct DeintOption {
    DeintOption() = default;
    DeintOption(DeintMethod method, Processor proc, bool doubler)
        : method(method), processor(proc), doubler(doubler) { }
    DECL_EQ(DeintOption, &T::method, &T::processor, &T::doubler);
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    auto toString() const -> QString;
    static auto fromString(const QString &string) -> DeintOption;

    DeintMethod method = DeintMethod::None;
    Processor processor = Processor::None;
    bool doubler = false;
};

Q_DECLARE_METATYPE(DeintOption)

struct DeintOptionSet {
    DeintOptionSet();
    DECL_EQ(DeintOptionSet, &T::hwdec, &T::swdec);
    auto option(Processor proc) const -> DeintOption
    {
        return proc == Processor::CPU ? swdec
             : proc == Processor::GPU ? hwdec : DeintOption();
    }
    auto toJson() const -> QJsonObject;
    auto setFromJson(const QJsonObject &json) -> bool;
    DeintOption hwdec, swdec;
};

Q_DECLARE_METATYPE(DeintOptionSet)

#endif // DEINTINFO_HPP
