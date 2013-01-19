#include "mpmessage.hpp"
#include "richtexthelper.hpp"

extern "C" {
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <core/mp_msg.h>
#define MSGSIZE_MAX 3072
int mp_msg_levels[MSGT_MAX]; // verbose level of this module. initialized to -2
int mp_msg_level_all = MSGL_STATUS;
int verbose = 0;
int mp_msg_color = 0;
int mp_msg_module = 0;
int mp_msg_test(int mod, int lev) {return lev <= (mp_msg_levels[mod] == -2 ? mp_msg_level_all + verbose : mp_msg_levels[mod]);}
const char* filename_recode(const char* filename) {return filename;}
char *mp_gtext(const char *string) {return (char *)string;}
void mp_msg_init(void){
	char *env = getenv("MPLAYER_VERBOSE");
	if (env)
		verbose = atoi(env);
	for(int i=0;i<MSGT_MAX;i++)
		mp_msg_levels[i] = -2;
	mp_msg_levels[MSGT_IDENTIFY] = -1; // no -identify output by default
}

void mp_msg_va(int mod, int lev, const char *format, va_list va) {
	if (!mp_msg_test(mod, lev))
		return; // do not display
	char tmp[MSGSIZE_MAX];
	vsnprintf(tmp, MSGSIZE_MAX, format, va);
	tmp[MSGSIZE_MAX-2] = '\n';
	tmp[MSGSIZE_MAX-1] = 0;
	fprintf(stdout, "%s", tmp);
	fflush(stdout);
}

void mp_msg(int mod, int lev, const char *format, ...) {
	va_list va;
	va_start(va, format);
	mp_msg_va2(mod, lev, format, va);
	va_end(va);
}

void mp_tmsg(int mod, int lev, const char *format, ...) {
	va_list va;
	va_start(va, format);
	mp_msg_va2(mod, lev, mp_gtext(format), va);
	va_end(va);
}

}

void mp_msg_va2(int mod, int lev, const char *format, va_list va) {
	if (!mp_msg_test(mod, lev))
		return; // do not display
	if (format[0] == '[')
		return;
	char tmp[MSGSIZE_MAX];
	vsnprintf(tmp, MSGSIZE_MAX, format, va);
	tmp[MSGSIZE_MAX-2] = '\n';
	tmp[MSGSIZE_MAX-1] = '\0';
//	qDebug() << tmp;
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

QList<MpMessage*> MpMessage::parsers;

MpMessage::MpMessage() {
	parsers.append(this);
}

MpMessage::~MpMessage() {
	parsers.removeAll(this);
}

void MpMessage::_parse(const QString &line) {
	auto id = MpMessage::id(line);
//	qDebug() << "mpv:" << qPrintable(line);
	if (id.name.isEmpty()) {
		for (auto p : parsers)
			if (p->parse(line))
				return;
	} else {
		for (auto p : parsers)
			if (p->parse(id))
				return;
	}
}

bool MpMessage::parse(const QString &) {return false;}
bool MpMessage::parse(const Id &) {return false;}

MpMessage::Id MpMessage::id(const QString &line) {
	static QRegExp rx("^ID_([^=]+)=([^=]+)$");
	return rx.indexIn(line) != -1 ? Id(rx.cap(1), rx.cap(2)) : Id();
}

bool MpMessage::getStream(const Id &id, const char *category, const char *idtext, StreamList &streams, const QString &trans) {
	static QRegExp rxCategory("^(AUDIO|VIDEO|SUBTITLE)_ID$");
	if (rxCategory.indexIn(id.name) != -1) {
		if (_Same(rxCategory.cap(1), category)) {
			auto &name = streams[id.value.toInt()].m_name;
			name = trans.arg(streams.size());
			return true;
		}
	} else {
		static QRegExp rxId("^(AID|SID|VID)_(\\d+)_(LANG|NAME)$");
		if (rxId.indexIn(id.name) != -1) {
			if (_Same(rxId.cap(1), idtext)) {
				const int streamId = rxId.cap(2).toInt();
				const auto attr = rxId.cap(3);
				const auto value = id.value;
				if (_Same(attr, "LANG"))
					streams[streamId].m_lang = value;
				else
					streams[streamId].m_title = value;
				return true;
			}
		}
	}
	return false;
}
