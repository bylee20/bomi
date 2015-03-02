#include "jrconnection.hpp"

const std::array<JrConnectionInfo::Item, 2> JrConnectionInfo::info{{
    {JrConnection::Tcp, u"Tcp"_q, u""_q, (int)0},
    {JrConnection::Local, u"Local"_q, u""_q, (int)1}
}};
