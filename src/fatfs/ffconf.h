/*---------------------------------------------------------------------------/
/  Configuration of FatFs R0.16 for sd2iec
/  (option descriptions: see the ffconf.h of the FatFs distribution)
/---------------------------------------------------------------------------*/

#define FFCONF_DEF	80386	/* Revision ID */

/* Enables the sd2iec changes in ff.c and ff.h (sd2iec API, FILINFO, shared window) */
#define FF_SD2IEC		1

#ifdef FF_HOST_TEST
#include "fftest.h"
#else
#include "config.h"
#endif

/* Longest long name that is created or returned */
#ifdef CONFIG_XE5
#define FF_MAX_LFN_CHARS	45
#elif defined(CONFIG_LCD_DISPLAY)
#define FF_MAX_LFN_CHARS	32
#else
#define FF_MAX_LFN_CHARS	20
#endif

#define FF_FS_READONLY	0
#define FF_FS_MINIMIZE	0
#define FF_USE_FIND		0
#define FF_USE_MKFS		0
#define FF_USE_FASTSEEK	0
#define FF_USE_EXPAND	0
#define FF_USE_CHMOD	1
#define FF_USE_LABEL	0
#define FF_USE_FORWARD	0
#define FF_USE_STRFUNC	0
#define FF_PRINT_LLI	0
#define FF_PRINT_FLOAT	0
#define FF_STRF_ENCODE	0

#define FF_CODE_PAGE	437
#define FF_USE_LFN		1
#define FF_MAX_LFN		FF_MAX_LFN_CHARS
#define FF_LFN_UNICODE	0
#define FF_LFN_BUF		12
#define FF_SFN_BUF		12

#define FF_FS_RPATH		1
#define FF_PATH_DEPTH	10

#define FF_VOLUMES		1
#define FF_STR_VOLUME_ID	0
#define FF_MULTI_PARTITION	0
#define FF_MIN_SS		512
#define FF_MAX_SS		512
#define FF_LBA64		0
#define FF_MIN_GPT		0x10000000
#define FF_USE_TRIM		0

#define FF_FS_TINY		1
#define FF_FS_EXFAT		0

#ifdef HAVE_RTC
#define FF_FS_NORTC		0
#else
/* Fixed time 1982-08-31 0:00:00, same month as the introduction of the C64 */
#define FF_FS_NORTC		1
#endif
#define FF_NORTC_MON	8
#define FF_NORTC_MDAY	31
#define FF_NORTC_YEAR	1982

#define FF_FS_CRTIME	0
#define FF_FS_NOFSINFO	0
#define FF_FS_LOCK		0
#define FF_FS_REENTRANT	0
#define FF_FS_TIMEOUT	1000
