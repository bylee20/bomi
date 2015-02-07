#ifndef DEINTCAPS_HPP
#define DEINTCAPS_HPP

#include "enum/deintmethod.hpp"
#include "enum/processor.hpp"

template<class T> struct JsonIO;
struct DeintOption;

class DeintCaps {
public:
    DeintCaps() { }
    DECL_EQ(DeintCaps, &T::m_method, &T::m_procs, &T::m_doubler)
    auto method() const -> DeintMethod { return m_method; }
    auto hwdec() const -> bool { return supports(Processor::GPU); }
    auto swdec() const -> bool { return supports(Processor::CPU); }
    auto doubler() const -> bool { return m_doubler; }
    auto supports(Processor proc) const -> bool
        { return m_procs.contains(proc); }
    auto isAvailable() const -> bool
        { return m_method != DeintMethod::None && m_procs != 0; }
    auto toOption(Processor proc) const -> DeintOption;
    static auto supports(const DeintOption &option) -> bool;
    static auto list(Processors procs) -> QList<DeintCaps>;
    static auto default_(Processor proc) -> DeintCaps;
    static auto defaultOption(Processor proc) -> DeintOption;
private:
    static auto list() -> const QList<DeintCaps>&;
    DeintMethod m_method = DeintMethod::None;
    Processors m_procs = Processor::None;
    bool m_doubler = false;
    friend struct JsonIO<DeintCaps>;
};

Q_DECLARE_METATYPE(DeintCaps);

#endif // DEINTCAPS_HPP
