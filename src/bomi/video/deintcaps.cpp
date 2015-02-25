#include "deintcaps.hpp"
#include "deintoption.hpp"
#include "os/os.hpp"
#include "misc/json.hpp"

auto DeintCaps::list() -> const QList<DeintCaps>&
{
    static QList<DeintCaps> caps;
    if (!caps.isEmpty())
        return caps;
    for (int i=0; i<DeintMethodInfo::size(); ++i) {
        caps.push_back(DeintCaps());
        caps.back().m_method = (DeintMethod)i;
    }

    auto push = [] (DeintMethod method, bool cpu, bool gpu, bool doubler) {
        auto &cap = caps[(int)method];
        cap.m_method = method;
        if (cpu)
            cap.m_procs |= Processor::CPU;
        if (gpu && OS::hwAcc()->supports(method))
            cap.m_procs |= Processor::GPU;
        cap.m_doubler = doubler;
        return caps[(int)method];
    };
    //  method                        cpu    gpu    doubler
    push(DeintMethod::Bob           , true , true, true );
    push(DeintMethod::LinearBob     , true , true, true );
    push(DeintMethod::CubicBob      , true , true, true );
    push(DeintMethod::Yadif         , true , true, true );
    return caps;
}

auto DeintCaps::list(Processors procs) -> QList<DeintCaps>
{
    QList<DeintCaps> ret;
    for (auto &caps : list()) {
        if (caps.m_procs & procs)
            ret.push_back(caps);
    }
    return ret;
}

auto DeintCaps::default_(Processor proc) -> DeintCaps
{
    for (auto &caps : list()) {
        if (caps.supports(proc))
            return caps;
    }
    return DeintCaps();
}

auto DeintCaps::defaultOption(Processor proc) -> DeintOption
{
    return default_(proc).toOption(proc);
}

auto DeintCaps::toOption(Processor proc) const -> DeintOption
{
    DeintOption option;
    option.method = m_method;
    option.processor = proc & m_procs;
    option.doubler = m_doubler;
    return option;
}

auto DeintCaps::supports(const DeintOption &option) -> bool
{
    for (auto &caps : list()) {
        if (caps.method() != option.method)
            continue;
        if (!caps.supports(option.processor))
            return false;
        if (option.doubler && !caps.doubler())
            return false;
        return true;
    }
    return false;
}
