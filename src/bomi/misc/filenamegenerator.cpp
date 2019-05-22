#include "filenamegenerator.hpp"

using GenFunc = QString(*)(const FileNameGenerator*);
SIA n2(int v) { return v < 10 ? QString('0'_q % _N(v)) : _N(v); }
SIA n3(int v) { return v < 10 ? QString("00"_a % _N(v)) : v < 100 ? QString('0'_q % _N(v)) : _N(v); }

auto get(const FileNameGenerator *g, const QString &ph) -> QString
{
    static const auto func = [] () {
        QMap<QString, GenFunc> get;
        get[u"%YEAR%"_q]    = [] (const FileNameGenerator *g) { return _N(g->dateTime.date().year()); };
        get[u"%YEAR_2%"_q]  = [] (const FileNameGenerator *g) { return _N(g->dateTime.date().year() % 100); };
        get[u"%MONTH%"_q]   = [] (const FileNameGenerator *g) { return _N(g->dateTime.date().month()); };
        get[u"%MONTH_0%"_q] = [] (const FileNameGenerator *g) { return n2(g->dateTime.date().month()); };
        get[u"%MONTH_S%"_q] = [] (const FileNameGenerator *g) { return QDate::shortDayName(g->dateTime.date().month()); };
        get[u"%MONTH_L%"_q] = [] (const FileNameGenerator *g) { return QDate::longMonthName(g->dateTime.date().month()); };
        get[u"%DAY%"_q]     = [] (const FileNameGenerator *g) { return _N(g->dateTime.date().day()); };
        get[u"%DAY_0%"_q]   = [] (const FileNameGenerator *g) { return n2(g->dateTime.date().day()); };
        get[u"%HOUR%"_q]    = [] (const FileNameGenerator *g) { return _N(g->dateTime.time().hour()); };
        get[u"%HOUR_0%"_q]  = [] (const FileNameGenerator *g) { return n2(g->dateTime.time().hour()); };
        get[u"%MIN%"_q]     = [] (const FileNameGenerator *g) { return _N(g->dateTime.time().minute()); };
        get[u"%MIN_0%"_q]   = [] (const FileNameGenerator *g) { return n2(g->dateTime.time().minute()); };
        get[u"%SEC%"_q]     = [] (const FileNameGenerator *g) { return _N(g->dateTime.time().second()); };
        get[u"%SEC_0%"_q]   = [] (const FileNameGenerator *g) { return n2(g->dateTime.time().second()); };
        get[u"%MSEC%"_q]    = [] (const FileNameGenerator *g) { return _N(g->dateTime.time().msec()); };
        get[u"%MSEC_0%"_q]  = [] (const FileNameGenerator *g) { return n3(g->dateTime.time().msec()); };

        get[u"%T_HOUR%"_q]   = [] (const FileNameGenerator *g) { return _N(g->start.hour()); };
        get[u"%T_HOUR_0%"_q] = [] (const FileNameGenerator *g) { return n2(g->start.hour()); };
        get[u"%T_MIN%"_q]    = [] (const FileNameGenerator *g) { return _N(g->start.minute()); };
        get[u"%T_MIN_0%"_q]  = [] (const FileNameGenerator *g) { return n2(g->start.minute()); };
        get[u"%T_SEC%"_q]    = [] (const FileNameGenerator *g) { return _N(g->start.second()); };
        get[u"%T_SEC_0%"_q]  = [] (const FileNameGenerator *g) { return n2(g->start.second()); };
        get[u"%T_MSEC%"_q]   = [] (const FileNameGenerator *g) { return _N(g->start.msec()); };
        get[u"%T_MSEC_0%"_q] = [] (const FileNameGenerator *g) { return n3(g->start.msec()); };

        get[u"%E_HOUR%"_q]   = [] (const FileNameGenerator *g) { return _N(g->end.hour()); };
        get[u"%E_HOUR_0%"_q] = [] (const FileNameGenerator *g) { return n2(g->end.hour()); };
        get[u"%E_MIN%"_q]    = [] (const FileNameGenerator *g) { return _N(g->end.minute()); };
        get[u"%E_MIN_0%"_q]  = [] (const FileNameGenerator *g) { return n2(g->end.minute()); };
        get[u"%E_SEC%"_q]    = [] (const FileNameGenerator *g) { return _N(g->end.second()); };
        get[u"%E_SEC_0%"_q]  = [] (const FileNameGenerator *g) { return n2(g->end.second()); };
        get[u"%E_MSEC%"_q]   = [] (const FileNameGenerator *g) { return _N(g->end.msec()); };
        get[u"%E_MSEC_0%"_q] = [] (const FileNameGenerator *g) { return n3(g->end.msec()); };

        get[u"%MEDIA_NAME%"_q] = [] (const FileNameGenerator *g) {
            const auto mrl = g->mrl;
            if (mrl.isLocalFile())
                return QFileInfo(mrl.toLocalFile()).fileName();
            return mrl.toString();
        };
        get[u"%MEDIA_DISPLAY_NAME%"_q] = [] (const FileNameGenerator *g) { return g->mediaName; };

        get[u"%UNIX%"_q]     = [] (const FileNameGenerator *g) { return _N(g->unix_ / 1000llu); };
        get[u"%UNIX_MS%"_q]  = [] (const FileNameGenerator *g) { return _N(g->unix_); };
        return get;
    }();
    auto ret = func.value(ph);
    if (ret)
        return ret(g);
    QRegEx rxRand(uR"(^%RAND_(\d+)%$)"_q);
    auto m = rxRand.match(ph);
    if (m.hasMatch()) {
        const int n = m.capturedRef(1).toInt();
        QString nums; nums.reserve(n);
        for (int i = 0; i < n; ++i)
            nums += _N(qrand() % 10);
        return nums;
    }
    return QString();
}

auto FileNameGenerator::get(const QString &folder, const QString &format, const QString &suffix) const -> QString
{
    QString base;
    int pos = 0;
    auto append = [&] (int end) { base += format.midRef(pos, end - pos); pos = end; };
    while (pos < format.size()) {
        const int from = format.indexOf('%'_q, pos);
        if (from < 0) {
            append(format.size());
            break;
        }
        append(from);
        const int end = format.indexOf('%'_q, from + 1) + 1;
        if (end <= 0) {
            append(format.size());
            break;
        }
        const auto name = format.mid(from, end - from);
        const auto value = ::get(this, name);
        if (!value.isEmpty())
            base += value;
        else
            append(end);
        pos = end;
    }

    for (auto &ch : base) {
        switch (ch.unicode()) {
        case '"': case '*': case '/': case '\\':
        case '<': case '>': case ':': case '|': case '?':
            ch = '_'_q;
        default: continue;
        }
    }

    QDir dir(folder);
    dir.setNameFilters({ "*."_a % suffix });
    dir.setFilter(QDir::Files);

    QRegEx rxCounter(uR"(%COUNTER_(\d+)%)"_q);
    auto m = rxCounter.match(base);
    if (m.hasMatch()) {
        base.replace(m.capturedStart(1), m.capturedLength(1), u"XX"_q);
        base.remove(rxCounter);
        QString rxs;
        rxs += '^'_q;
        for (auto ch : base) {
            switch (ch.unicode()) {
            case '^': case '$': case '.': case '+':
            case '(': case ')': case '[': case ']': case '{': case '}':
                rxs += '\\'_q;
            default:
                rxs += ch;
            }
        }
        rxs += '$'_q;
        const int n = m.capturedRef(1).toInt();
        rxs.replace(u"%COUNTER_XX%"_q, "(\\d{"_a % _N(n) % ",})"_a);
        QRegEx rx(rxs, QRegEx::CaseInsensitiveOption);
        qint64 max = 0;
        for (auto &info : dir.entryInfoList()) {
            auto name = info.completeBaseName();
            auto m = rx.match(name);
            if (m.hasMatch())
                max = std::max(max, m.capturedRef(1).toLongLong());
        }
        base.replace(u"%COUNTER_XX%"_q, u"%1"_q.arg(max + 1, n, 10, '0'_q));
    }

    QString fileName = base % '.'_q % suffix;
    for (int i = 1; dir.exists(fileName); ++i)
        fileName = base % '_'_q % _N(i) % '.'_q % suffix;
    return dir.absoluteFilePath(fileName);
}

auto FileNameGenerator::toolTip() -> QString
{
    return qApp->translate("PrefDialog",
        "Placeholders\n\n"

        "Ones with _0 suffix correspond to with-leading-zero version.\n"
        "Small n in suffix represents a positive integer.\n\n"

        "Current date/time\n"
        "%YEAR%, %YEAR_2%(2 digits),\n"
        "%MONTH%, %MONTH_0%, %MONTH_S%(short name), %MONTH_L%(long name),\n"
        "%DAY%, %DAY_0%,\n"
        "%HOUR%, %MIN%, %SEC%, %MSEC%,\n"
        "%HOUR_0%, %MIN_0%, %SEC_0%, %MSEC_0%\n\n"

        "Playback information\n"
        "%T_HOUR%, %T_MIN%, %T_SEC%, %T_MSEC%,\n"
        "%T_HOUR_0%, %T_MIN_0%, %T_SEC_0%, %T_MSEC_0%,\n"
        "%MEDIA_NAME%(file name or remote location),\n"
        "%MEDIA_DISPLAY_NAME%(display name in player),\n"
        "%COUNTER_n%(incremental counter with n digits),\n\n"

        "%UNIX%(UNIX timestamp in seconds)\n"
        "%UNIX_MS%(UNIX timestamp in milliseconds)\n"
        "%RAND_n%(random number with n digits)"
    );
}
