/*------------------------------------------------------------------------*/
/* Character set conversion of FatFs for sd2iec: Latin-1 as the code page */
/*------------------------------------------------------------------------*/
/* Replaces ffunicode.c. The names written by sd2iec are ASCII, bytes above
/  0x7F of an 8.3 name or a long name are passed on as Latin-1, characters of
/  a long name outside Latin-1 are not representable (the 8.3 name is used). */

#include "ff.h"

#if FF_USE_LFN != 0

WCHAR ff_uni2oem (	/* Returns OEM code character, zero on error */
	DWORD	uni,	/* UTF-16 encoded character to be converted */
	WORD	cp		/* Code page for the conversion */
)
{
	(void)cp;
	return uni < 0x100 ? (WCHAR)uni : 0;
}


WCHAR ff_oem2uni (	/* Returns Unicode character in UTF-16, zero on error */
	WCHAR	oem,	/* OEM code to be converted */
	WORD	cp		/* Code page for the conversion */
)
{
	(void)cp;
	return oem < 0x100 ? oem : 0;
}


DWORD ff_wtoupper (	/* Returns up-converted code point */
	DWORD uni		/* Unicode code point to be up-converted */
)
{
	if (uni >= 'a' && uni <= 'z') return uni - 0x20;
	if (uni >= 0xE0 && uni <= 0xFE && uni != 0xF7) return uni - 0x20;
	return uni;
}

#endif /* FF_USE_LFN != 0 */
