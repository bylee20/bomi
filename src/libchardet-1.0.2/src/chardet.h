/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * Mozilla's universal charset detector C/C++ Wrapping API
 *      Writer(s) :
 *          Detect class by John Gardiner Myers <jgmyers@proofpoint.com>
 *          C wrapping API by JoungKyun.Kim <http://oops.org>
 *
 * $Id: chardet.h,v 1.5 2010/07/05 12:17:50 oops Exp $
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#if defined _WIN32 || defined __CYGWIN__
	#define CHARDET_API
#else
	#if defined(__GNUC__) && __GNUC__ >= 4
		#define CHARDET_API __attribute__ ((visibility("default")))
	#else
		#define CHARDET_API
	#endif
#endif

#ifdef HAVE_CONFIG_H
#include <chardet-config.h>
#endif

#include <version.h>

#include <stdio.h>
#include <string.h>

#define LIBCHARDET_MAJOR_VER 1
#define LIBCHARDET_MINOR_VER 0
#define LIBCHARDET_PATCH_VER 0
#define LIBCHARDET_VER "1.0.0"

#define CHARDET_OUT_OF_MEMORY -128
#define CHARDET_MEM_ALLOCATED_FAIL -127

#define CHARDET_SUCCESS     0
#define CHARDET_NO_RESULT   1
#define CHARDET_NULL_OBJECT 2


#ifdef __cplusplus
extern "C" {
#endif
	typedef struct Detect_t Detect;

	typedef struct DetectObject {
		char * encoding;
		float confidence;
	} DetectObj;

	CHARDET_API char * detect_version (void);
	CHARDET_API char * detect_uversion (void);

	CHARDET_API DetectObj * detect_obj_init (void);
	CHARDET_API void detect_obj_free (DetectObj **);

	CHARDET_API Detect * detect_init (void);
	CHARDET_API void detect_reset (Detect **);
	CHARDET_API void detect_dataend (Detect **);
	CHARDET_API short detect_handledata (Detect **, const char *, DetectObj **);
	CHARDET_API void detect_destroy (Detect **);
	CHARDET_API short detect (const char *, DetectObj **);
#ifdef __cplusplus
};
#endif

/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 * vim600: noet sw=4 ts=4 fdm=marker
 * vim<600: noet sw=4 ts=4
 */
