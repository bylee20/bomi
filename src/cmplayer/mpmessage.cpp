#include "mpmessage.hpp"
#include "richtexthelper.hpp"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <mpvcore/mp_msg.h>
#define MSGSIZE_MAX 6144
//int mp_msg_levels[MSGT_MAX]; // verbose level of this module. initialized to -2
//int mp_msg_level_all = MSGL_STATUS;
//int verbose = 0;
//int mp_msg_color = 0;
//int mp_msg_module = 0;

//void mp_msg_va(int mod, int lev, const char *format, va_list va) {
//	if (!mp_msg_test(mod, lev))
//		return; // do not display
//	char tmp[MSGSIZE_MAX];
//	vsnprintf(tmp, MSGSIZE_MAX, format, va);
//	tmp[MSGSIZE_MAX-2] = '\n';
//	tmp[MSGSIZE_MAX-1] = 0;
//	fprintf(stdout, "%s", tmp);
//	fflush(stdout);
//}

//void mp_msg(int mod, int lev, const char *format, ...) {
//	va_list va;
//	va_start(va, format);
//	mp_msg_va2(mod, lev, format, va);
//	va_end(va);
//}

//void mp_tmsg(int mod, int lev, const char *format, ...) {
//	va_list va;
//	va_start(va, format);
//	mp_msg_va2(mod, lev, mp_gtext(format), va);
//	va_end(va);
//}

void mp_msg_log_va2(struct mp_log *log, int lev, const char *format, va_list va) {
	if (!mp_msg_test_log(log, lev))
		return; // do not display
	if (format[0] == '[')
		return;
	char tmp[MSGSIZE_MAX];
	vsnprintf(tmp, MSGSIZE_MAX, format, va);
	tmp[MSGSIZE_MAX-2] = '\n';
	tmp[MSGSIZE_MAX-1] = '\0';
	const int len = strlen(tmp);
	static QString line;
	if (tmp[len-1] == '\n') {
		line += QString::fromLocal8Bit(tmp, len-1);
		const auto list = line.split('\n', QString::SkipEmptyParts);
		for (auto &s : list)
			MpMessage::_parse(s);
		line.clear();
	} else
		line += QString::fromLocal8Bit(tmp, len);
}

}

QList<MpMessage*> MpMessage::parsers;

MpMessage::MpMessage() {
	parsers.append(this);
}

MpMessage::~MpMessage() {
	parsers.removeAll(this);
}

void MpMessage::_parse(const QString &line) {
	auto id = MpMessage::id(line);
	qDebug() << "mpv:" << qPrintable(line);
	if (id.name.isEmpty()) {
	} else {
		for (auto p : parsers)
			if (p->parse(id))
				return;
	}
}

bool MpMessage::parse(const Id &) {return false;}

MpMessage::Id MpMessage::id(const QString &line) {
	static QRegExp rx("^ID_([^=]+)=([^=]+)$");
	return rx.indexIn(line) != -1 ? Id(rx.cap(1), rx.cap(2)) : Id();
}
