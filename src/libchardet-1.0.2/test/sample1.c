/*
 * sample code with libchardet
 * author: JoungKyun.Kim <http://oops.org>
 * $Id: sample1.c,v 1.1.1.1 2009/02/21 20:08:02 oops Exp $
 */
#include <chardet.h>
//#include "../src/chardet.h"

int main (void) {
	DetectObj *obj;
	int i;

	char *str[] = {
		"안녕",
		"안녕하세요",
		"안녕하세요 정말?",
		"그래 이정도면 판단 될까?",
		"좀더 길게 적어 볼까 얼마나 길게 해야!",
		"그래 그래 좀 더 길게 적어 보자 더 길게 적어 보야야 겠지...",
		"12345 abcde"
	};

	short arrayNum;
	arrayNum = sizeof (str) / sizeof (str[0]);

	for ( i=0; i<arrayNum; i++ ) {
		obj = detect_obj_init ();
		if ( obj == NULL ) {
			fprintf (stderr, "On attemped \"%s\", memory allocation failed\n", str[i]);
			continue;
		}

		if ( detect (str[i], &obj) == CHARDET_OUT_OF_MEMORY ) {
			fprintf (stderr, "On handle processing, occured out of memory\n");
			return CHARDET_OUT_OF_MEMORY;
		}
		printf ("## %s : %s : %f\n", str[i], obj->encoding, obj->confidence);
		detect_obj_free (&obj);
	}

	return 0;
}
