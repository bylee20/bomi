#include "logoutput.hpp"

const std::array<LogOutputInfo::Item, 5> LogOutputInfo::info{{
    {LogOutput::Off, u"Off"_q, u""_q, (int)0},
    {LogOutput::StdOut, u"StdOut"_q, u""_q, (int)1},
    {LogOutput::StdErr, u"StdErr"_q, u""_q, (int)2},
    {LogOutput::File, u"File"_q, u""_q, (int)4},
    {LogOutput::Journal, u"Journal"_q, u""_q, (int)8}
}};
