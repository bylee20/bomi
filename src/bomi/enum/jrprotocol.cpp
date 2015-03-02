#include "jrprotocol.hpp"

const std::array<JrProtocolInfo::Item, 2> JrProtocolInfo::info{{
    {JrProtocol::Raw, u"Raw"_q, u""_q, (int)0},
    {JrProtocol::Http, u"Http"_q, u""_q, (int)1}
}};
