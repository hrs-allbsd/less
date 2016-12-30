/*
 * Copyright (C) 1984-2002  Mark Nudelman
 *
 * You may distribute under the terms of either the GNU General Public
 * License or the Less License, as specified in the README file.
 *
 * For more information about less, or for information on how to 
 * contact the author, see the README file.
 */
/*
 * Copyright (c) 1997-2005  Kazushi (Jam) Marukawa
 * All rights of japanized routines are reserved.
 *
 * You may distribute under the terms of the Less License.
 */


/*
 * Functions to define the character set
 * and do things specific to the character set.
 */

#include "less.h"
#if HAVE_LOCALE
#include <locale.h>
#include <ctype.h>
#endif

public int utf_mode = 0;

/*
 * Predefined character sets,
 * selected by the JLESSCHARSET or LESSCHARSET environment variable.
 */
struct charset {
	char *name;
	int *p_flag;
	char *desc;
	SETCHARSET scs;
	ENCSET input;
	ENCSET inputr;
	ENCSET output;
} charsets[] = {
	{ "ascii",	NULL,       "8bcccbcc18b95.b",
		SCSASCII,	ESNOCONV,	ESNONE,		ESNOCONV },
	{ "dos",	NULL,       "8bcccbcc12bc5b223.b",
		SCSASCII,	ESNOCONV,	ESNOCONV,	ESNOCONV },
	{ "ebcdic",	NULL,       "5bc6bcc7bcc41b.9b7.9b5.b..8b6.10b6.b9.7b9.8b8.17b3.3b9.7b9.8b8.6b10.b.b.b.",
		SCSASCII,	ESNOCONV,	ESNOCONV,	ESNOCONV },
	{ "IBM-1047",	NULL,       "4cbcbc3b9cbccbccbb4c6bcc5b3cbbc4bc4bccbc191.b",
		SCSASCII,	ESNOCONV,	ESNOCONV,	ESNOCONV },
	{ "iso8859",	NULL,       "8bcccbcc18b95.33b.",
		SCSASCII,	ESNOCONV,	ESNOCONV,	ESNOCONV },
	{ "koi8-r",	NULL,       "8bcccbcc18b95.b128.",
		SCSASCII,	ESNOCONV,	ESNOCONV,	ESNOCONV },
	{ "next",	NULL,       "8bcccbcc18b95.bb125.bb",
		SCSASCII,	ESNOCONV,	ESNOCONV,	ESNOCONV },
#if ISO
	{ "iso7",	NULL,       "8bcccb4c11bc4b96.b",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESISO7,		ESNONE,		ESISO7 },
	{ "iso8",	NULL,       "8bcccb4c11bc4b95.15b2.16b.",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESISO7,		ESISO8,		ESISO8 },
	{ "utf8",	NULL,  	    "8bcccbcc18b95.b126.b",
		SCSASCII | SCSUTF8,
				ESNOCONV,	ESUTF8,		ESUTF8 },
# if JAPANESE
	/* read ISO-2022(7bit) */
	{ "iso7-iso8",	NULL,       "8bcccb4c11bc4b95.b",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESISO7,		ESNONE,		ESISO8 },
	{ "iso7-utf8",	NULL,       "8bcccb4c11bc4b95.b",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESISO7,		ESNONE,		ESUTF8 },

	/* read ISO-2022(8bit) */
	{ "iso8-iso7",	NULL,       "8bcccb4c11bc4b95.15b2.16b.",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESISO7,		ESISO8,		ESISO7 },
	{ "iso8-jis83",	NULL,       "8bcccb4c11bc4b95.15b2.16b.",
		SCSASCII | SCSALLJISTRAD,
				ESISO7,		ESISO8,		ESJIS83 },
	{ "iso8-utf8",	NULL,       "8bcccb4c11bc4b95.15b2.16b.",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESISO7,		ESISO8,		ESUTF8 },
	{ "iso8-utf8j",	NULL,       "8bcccb4c11bc4b95.15b2.16b.",
		SCSASCII | SCSALLJIS,
				ESISO7,		ESISO8,		ESUTF8 },
	{ "iso8-ujis",	NULL,       "8bcccb4c11bc4b95.15b2.16b.",
		SCSASCII | SCSALLJIS,
				ESISO7,		ESISO8,		ESUJIS },
	{ "iso8-sjis",	NULL,       "8bcccb4c11bc4b95.15b2.16b.",
		SCSASCII | SCSALLSJIS,
				ESISO7,		ESISO8,		ESSJIS },
	{ "iso8-cp932",	NULL,       "8bcccb4c11bc4b95.15b2.16b.",
		SCSASCII | SCSCP932,
				ESISO7,		ESISO8,		ESCP932 },

	/* read ISO-2022 (JIS only) */
	{ "jis-iso7",	NULL,       "8bcccb4c11bc4b95.b",
		SCSASCII | SCSALLJIS,
				ESISO7,		ESNONE,		ESISO7 },
	{ "jis-iso8",	NULL,       "8bcccb4c11bc4b95.b",
		SCSASCII | SCSALLJIS,
				ESISO7,		ESNONE,		ESISO8 },
	{ "jis-jis83",	NULL,       "8bcccb4c11bc4b95.b",
		SCSASCII | SCSALLJISTRAD,
				ESISO7,		ESNONE,		ESJIS83 },
	{ "jis-utf8j",	NULL,       "8bcccb4c11bc4b95.b",
		SCSASCII | SCSALLJIS,
				ESISO7,		ESNONE,		ESUTF8 },
	{ "jis-ujis",	NULL,       "8bcccb4c11bc4b95.b",
		SCSASCII | SCSALLJIS,
				ESISO7,		ESNONE,		ESUJIS },
	{ "jis-sjis",	NULL,       "8bcccb4c11bc4b95.b",
		SCSASCII | SCSALLSJIS,
				ESISO7,		ESNONE,		ESSJIS },
	{ "jis-cp932",	NULL,       "8bcccb4c11bc4b95.b",
	  	SCSASCII | SCSCP932,
				ESISO7,		ESNONE,		ESCP932 },

	/* read UTF8 */
	{ "utf8-iso7",	NULL,       "8bcccbcc18b95.b126.b",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESNOCONV,	ESUTF8,		ESISO7 },
	{ "utf8-iso8",	NULL,       "8bcccbcc18b95.b126.b",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESNOCONV,	ESUTF8,		ESISO8 },
	{ "utf8-jis83",	NULL,       "8bcccbcc18b95.b126.b",
		SCSASCII | SCSALLJISTRAD,
				ESNOCONV,	ESUTF8,		ESJIS83 },
	{ "utf8-utf8j",	NULL,       "8bcccbcc18b95.b126.b",
		SCSASCII | SCSALLJIS,
				ESNOCONV,	ESUTF8,		ESUTF8 },
	{ "utf8-ujis",	NULL,       "8bcccbcc18b95.b126.b",
		SCSASCII | SCSALLJIS,
				ESNOCONV,	ESUTF8,		ESUJIS },
	{ "utf8-sjis",	NULL,       "8bcccbcc18b95.b126.b",
		SCSASCII | SCSALLSJIS,
				ESNOCONV,	ESUTF8,		ESSJIS },
	{ "utf8-cp932",	NULL,       "8bcccbcc18b95.b126.b",
		SCSASCII | SCSCP932,
				ESNOCONV,	ESUTF8,		ESCP932 },

	/* read UJIS */
	{ "ujis-iso7",	NULL,       "8bcccbcc18b95.15b2.17b94.b",
		SCSASCII | SCSALLJIS,
				ESNOCONV,	ESUJIS,		ESISO7 },
	{ "ujis-iso8",	NULL,       "8bcccbcc18b95.15b2.17b94.b",
		SCSASCII | SCSALLJIS,
				ESNOCONV,	ESUJIS,		ESISO8 },
	{ "ujis-jis83",	NULL,       "8bcccbcc18b95.15b2.17b94.b",
		/* except plane 2 and supplement */
		SCSASCII | SCSALLJISTRAD,
				ESNOCONV,	ESUJIS,		ESJIS83 },
	{ "ujis-utf8j",	NULL,       "8bcccbcc18b95.15b2.17b94.b",
		SCSASCII | SCSALLJIS,
				ESNOCONV,	ESUJIS,		ESUTF8 },
	{ "ujis-ujis",	NULL,       "8bcccbcc18b95.15b2.17b94.b",
		SCSASCII | SCSALLJIS,
				ESNOCONV,	ESUJIS,		ESUJIS },
	{ "ujis-sjis",	NULL,       "8bcccbcc18b95.15b2.17b94.b",
		SCSASCII | SCSALLSJIS,
				ESNOCONV,	ESUJIS,		ESSJIS },
	{ "ujis-cp932",	NULL,       "8bcccbcc18b95.15b2.17b94.b",
		SCSASCII | SCSCP932,
				ESNOCONV,	ESUJIS,		ESCP932 },

	/* read SJIS */
	{ "sjis-iso7",	NULL,       "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSALLSJIS,
				ESNOCONV,	ESSJIS,		ESISO7 },
	{ "sjis-iso8",	NULL,       "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSALLSJIS,
				ESNOCONV,	ESSJIS,		ESISO8 },
	{ "sjis-jis83",	NULL,       "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSALLJISTRAD,
				ESNOCONV,	ESSJIS,		ESJIS83 },
	{ "sjis-utf8j",	NULL,        "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSALLSJIS,
				ESNOCONV,	ESSJIS,		ESUTF8 },
	{ "sjis-ujis",	NULL,        "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSALLSJIS,
				ESNOCONV,	ESSJIS,		ESUJIS },
	{ "sjis-sjis",	NULL,        "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSALLSJIS,
				ESNOCONV,	ESSJIS,		ESSJIS },
	{ "sjis-cp932",	NULL,        "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSCP932,
				ESNOCONV,	ESSJIS,		ESCP932 },

	/* read CP932 */
	{ "cp932-iso7",		NULL,       "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSALLJIS,
				ESNOCONV,	ESCP932,	ESISO7 },
	{ "cp932-iso8",		NULL,       "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSALLJIS,
				ESNOCONV,	ESCP932,	ESISO8 },
	{ "cp932-jis83",	NULL,       "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSALLJISTRAD,
				ESNOCONV,	ESCP932,	ESJIS83 },
	{ "cp932-utf8j",	NULL,        "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSCP932,
				ESNOCONV,	ESCP932,	ESUTF8 },
	{ "cp932-ujis",		NULL,        "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSALLJIS,
				ESNOCONV,	ESCP932,	ESUJIS },
	{ "cp932-sjis",		NULL,        "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSALLSJIS,
				ESNOCONV,	ESCP932,	ESSJIS },
	{ "cp932-cp932",	NULL,        "8bcccbcc18b95.b125.3b",
		SCSASCII | SCSCP932,
				ESNOCONV,	ESCP932,	ESCP932 },

	/* read all Japanese */
	{ "japanese-iso7",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESISO7,		ESALLJA,	ESISO7 },
	{ "japanese-iso8",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESISO7,		ESALLJA,	ESISO8 },
	{ "japanese-utf8",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJIS | SCSOTHERISO | SCSUTF8,
				ESISO7,		ESALLJA,	ESUTF8 },
	{ "japanese-utf8j",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJIS,
				ESISO7,		ESALLJA,	ESUTF8 },
	{ "japanese-ujis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJIS,
				ESISO7,		ESALLJA,	ESUJIS },
	{ "japanese-sjis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLSJIS,
				ESISO7,		ESALLJA,	ESSJIS },
	{ "japanese-cp932",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSCP932,
				ESISO7,		ESALLJA,	ESCP932 },

	/* read all Japanese before 1983 */
	{ "japanese83-iso7",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD,
				ESISO7,		ESALLJA,	ESISO7 },
	{ "japanese83-iso8",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD,
				ESISO7,		ESALLJA,	ESISO8 },
	{ "japanese83-jis83",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD,
				ESISO7,		ESALLJA,	ESJIS83 },
	{ "japanese83-utf8j",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD,
				ESISO7,		ESALLJA,	ESUTF8 },
	{ "japanese83-ujis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD,
				ESISO7,		ESALLJA,	ESUJIS },
	{ "japanese83-sjis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD,
				ESISO7,		ESALLJA,	ESSJIS },
	{ "japanese83-cp932",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD,
				ESISO7,		ESALLJA,	ESCP932 },

	/* read all Japanese before 1990 */
	{ "japanese90-iso7",	NULL,       "8bcccb4c11bc4b95.b127.b",
	  	SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 | SCSJISX0212_1990,
				ESISO7,		ESALLJA,	ESISO7 },
	{ "japanese90-iso8",	NULL,       "8bcccb4c11bc4b95.b127.b",
	  	SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 | SCSJISX0212_1990,
				ESISO7,		ESALLJA,	ESISO8 },
	{ "japanese90-utf8j",	NULL,       "8bcccb4c11bc4b95.b127.b",
	  	SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 | SCSJISX0212_1990,
				ESISO7,		ESALLJA,	ESUTF8 },
	{ "japanese90-ujis",	NULL,       "8bcccb4c11bc4b95.b127.b",
	  	SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 | SCSJISX0212_1990,
				ESISO7,		ESALLJA,	ESUJIS },
	{ "japanese90-sjis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990,
				ESISO7,		ESALLJA,	ESSJIS },

	/* read all KANJI before 2000 */
	{ "japanese90-iso7",	NULL,       "8bcccb4c11bc4b95.b127.b",
	  	SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 |
		SCSJISX0213_2000 | SCSJISX0213_2ND | SCSJISX0212_1990,
				ESISO7,		ESALLJA,	ESISO7 },
	{ "japanese00-iso8",	NULL,       "8bcccb4c11bc4b95.b127.b",
	  	SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 |
		SCSJISX0213_2000 | SCSJISX0213_2ND | SCSJISX0212_1990,
				ESISO7,		ESALLJA,	ESISO8 },
	{ "japanese00-utf8j",	NULL,       "8bcccb4c11bc4b95.b127.b",
	  	SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 |
		SCSJISX0213_2000 | SCSJISX0213_2ND | SCSJISX0212_1990,
				ESISO7,		ESALLJA,	ESUTF8 },
	{ "japanese00-ujis",	NULL,       "8bcccb4c11bc4b95.b127.b",
	  	SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 |
		SCSJISX0213_2000 | SCSJISX0213_2ND | SCSJISX0212_1990,
				ESISO7,		ESALLJA,	ESUJIS },
	{ "japanese00-sjis",	NULL,       "8bcccb4c11bc4b95.b127.b",
	  	SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 |
		SCSJISX0213_2000 | SCSJISX0213_2ND,
				ESISO7,		ESALLJA,	ESSJIS },

	/* read all Japanese page */
	{ "japanesep1-iso7",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 |
		SCSJISX0213_2000 | SCSJISX0213_2004,
				ESISO7,		ESALLJA,	ESISO7 },
	{ "japanesep1-iso8",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 |
		SCSJISX0213_2000 | SCSJISX0213_2004,
				ESISO7,		ESALLJA,	ESISO8 },
	{ "japanesep1-jis83",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 |
		SCSJISX0213_2000 | SCSJISX0213_2004,
				ESISO7,		ESALLJA,	ESJIS83 },
	{ "japanesep1-utf8j",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 |
		SCSJISX0213_2000 | SCSJISX0213_2004,
				ESISO7,		ESALLJA,	ESUTF8 },
	{ "japanesep1-ujis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJISTRAD | SCSJISX0208_1990 |
		SCSJISX0213_2000 | SCSJISX0213_2004,
				ESISO7,		ESALLJA,	ESUJIS },
	{ "japanesep1-sjis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLSJIS,
				ESISO7,		ESALLJA,	ESSJIS },

	/* read all Japanese (use CP932 insted of SJIS) */
	{ "japanesecp932-iso7",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESISO7,		ESALLJACP932,	ESISO7 },
	{ "japanesecp932-iso8",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJIS | SCSOTHERISO,
				ESISO7,		ESALLJACP932,	ESISO8 },
	{ "japanesecp932-utf8",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJIS | SCSOTHERISO | SCSCP932 | SCSUTF8,
				ESISO7,		ESALLJACP932,	ESUTF8 },
	{ "japanesecp932-utf8j",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJIS,
				ESISO7,		ESALLJACP932,	ESUTF8 },
	{ "japanesecp932-ujis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLJIS,
				ESISO7,		ESALLJACP932,	ESUJIS },
	{ "japanesecp932-sjis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		SCSASCII | SCSALLSJIS,
				ESISO7,		ESALLJACP932,	ESSJIS },
	{ "japanesecp932-cp932",NULL,       "8bcccb4c11bc4b95.b127.b",
	  	SCSASCII | SCSCP932,
				ESISO7,		ESALLJACP932,	ESCP932 },
# endif
#endif
	{ NULL, NULL, NULL,
		SCSASCII,	ESNOCONV,	ESNOCONV,	ESNOCONV },
};

#if HAVE_LOCALE && ISO
/*
 * Predefined local languages,
 * selected by the setlocale().
 */
struct charlocale {
	char *name;
	char *charset;
} charlocales[] = {
	{ "C",			"ascii"	},
	{ "wr_WR.ct",		"iso8"	},
	{ "ja_JP.jis8",		"iso8"	},
# if JAPANESE
	{ "ja_JP.JIS",		"japanese-jis83" },
	{ "ja_JP.jis7",		"japanese-jis83" },
	{ "ja_JP.EUC",		"japanese-ujis"	},
	{ "ja_JP.ujis",		"japanese-ujis"	},
	{ "ja_JP.SJIS",		"japanese-sjis"	},
	{ "ja_JP.Shift_JIS",	"japanese-sjis"	},
	{ "ja_JP.mscode",	"japanese-sjis"	},
/* Other local locales */
#  ifdef _AIX
	/* AIX's */
	{ "Ja_JP",		"japanese-sjis" },
	{ "ja_JP.IBM-eucJP",	"japanese-ujis" },
	{ "Ja_JP.IBM-932",	"japanese-sjis" },
#  endif
#  ifdef __hpux
	/* HPUX */
	{ "japanese",		"japanese-sjis" },
	{ "japanese.euc",	"japanese-ujis" },
#  endif
	{ "ja",			"japanese-utf8" },
	{ "ja_JP",		"japanese-utf8" },
	{ "japan",		"japanese-ujis"	},
	{ "Japan",		"japanese-ujis"	},
	{ "japanese",		"japanese-ujis"	},
	{ "ja_JP.utf-8",	"japanese-utf8"	},
	{ "ja_JP.utf8",		"japanese-utf8"	},
	{ "ja_JP.eucJP",	"japanese-ujis"	},
	/* DEC OSF/1's */
	{ "ja_JP.deckanji",	"japanese-ujis"	},
	{ "ja_JP.sdeckanji",	"japanese-ujis"	},
	/* BSDI's */
	{ "Japanese-EUC",	"japanese-ujis"	},
	/* Win32 */
	{ "Japanese_Japan.932",	"japanese-sjis"	},
# endif
	{ NULL,                 "" }
};
#endif

struct cs_alias {
	char *name;
	char *oname;
} cs_aliases[] = {
	{ "latin1",	"iso8859" },
	{ "latin9",	"iso8859" },
#if JAPANESE
	{ "iso7-iso7",		"iso7" },
	{ "iso8-iso8",		"iso8" },
	{ "iso7-jis83",		"jis-jis83" },
	{ "jis",		"jis-iso7" },
	{ "jis83",		"jis-jis83" },
	{ "iso2022jp",		"jis-jis83" },
	{ "iso-2022-jp",	"jis-jis83" },
	{ "iso7-utf8j",		"jis-utf8j" },
	{ "jis-utf8",		"jis-utf8j" },
	{ "iso7-ujis",		"jis-ujis" },
	{ "iso7-sjis",		"jis-sjis" },
	{ "iso7-cp932",		"jis-cp932" },
	{ "utf8-utf8",		"utf8" },
	{ "utf8j",		"utf8-utf8j" },	
	{ "utf-8",		"utf8" },
	{ "ujis",		"ujis-ujis" },	
	{ "euc",		"ujis-ujis" },
	{ "euc-jp",		"ujis-ujis" },
	{ "eucjp",		"ujis-ujis" },
	{ "sjis",		"sjis-sjis" },	
	{ "shift_jis",		"sjis-sjis" },
	{ "shift-jis",		"sjis-sjis" },
	{ "cp932",		"cp932-cp932" },
	{ "windows-31j",	"cp932-cp932" },	
	{ "japaneseiso7",	"japanese-iso7" },
	{ "japaneseiso7-iso7",	"japanese-iso7" },
	{ "japanese",		"japanese-iso7" },
	{ "japanese-jis83",	"japanese83-jis83" },
	{ "japanese90-jis83",	"japanese83-jis83" },
	{ "japanese00-jis83",	"japanese83-jis83" },
	{ "japanese83-utf8",	"japanese83-utf8j" },
	{ "japanese90-utf8",	"japanese90-utf8j" },
	{ "japanese00-utf8",	"japanese00-utf8j" },
	{ "japanesep1-utf8",	"japanesep1-utf8j" },
	{ "japanese-euc",	"japanese-ujis" },
	{ "japanese83-euc",	"japanese83-ujis" },
	{ "japanese90-euc",	"japanese90-ujis" },
	{ "japanese90-euc",	"japanese00-ujis" },
	{ "japanese-jis",	"japanese-iso7" },
	{ "japanese83-jis",	"japanese83-iso7" },
	{ "japanese90-jis",	"japanese90-iso7" },
	{ "japanese00-jis",	"japanese00-iso7" },
	{ "japanese90-cp932",	"japanese-cp932" },
	{ "japanese00-cp932",	"japanese-cp932" },
#endif
	{ NULL, NULL }
};

#define	IS_BINARY_CHAR	01
#define	IS_CONTROL_CHAR	02

static char chardef[256];
static char *binfmt = NULL;
static char *utfbinfmt = NULL;
public int binattr = AT_STANDOUT;
public char* opt_charset = NULL;


/*
 * Look for an appropriate charset and return it.
 */
	static struct charset *
search_charset(name)
	char *name;
{
	struct charset *p;
	char *name2, *n2;
	int namelen, name2len;
	int maxscore, score;
	struct charset *result;

	if (!name)
		name = "";
	namelen = strlen(name);
	name2 = strchr(name, '-');
	if (name2)
	{
		name2len = namelen;
		namelen = (name2 - name);
		name2len -= namelen;
	} else
	{
		name2len = 0;
	}
	maxscore = 0;
	result = NULL;
	for (p = charsets;  p->name != NULL;  p++)
	{
		score = 0;
		n2 = strchr(p->name, '-');
		if (strncasecmp(name, p->name, namelen) == 0) {
			score += namelen;
			if ((int) strlen(p->name) == namelen)
				score++; /* add bonus point for exactly match */
		}
		if (name2 && n2 && strncasecmp(name2, n2, name2len) == 0) {
			score += name2len - 1;	/* decrease score of '-' */
			if ((int) strlen(n2) == name2len)
				score++; /* add bonus point for exactly match */
		}
		if (score > maxscore)
		{
			maxscore = score;
			result = p;
		}
	}
	return (result);
}

/*
 * Return the ENCSET of left plane of named charset.
 */
	public ENCSET
left_codeset_of_charset(name)
	register char *name;
{
	struct charset *p = search_charset(name);

	if (p)
		return (p->input);
	return (ESNOCONV);
}

/*
 * Return the ENCSET of right plane of named charset.
 */
	public ENCSET
right_codeset_of_charset(name)
	register char *name;
{
	struct charset *p = search_charset(name);

	if (p)
		return (p->inputr);
	return (ESNOCONV);
}

/*
 * Define a charset, given a description string.
 * The string consists of 256 letters,
 * one for each character in the charset.
 * If the string is shorter than 256 letters, missing letters
 * are taken to be identical to the last one.
 * A decimal number followed by a letter is taken to be a 
 * repetition of the letter.
 *
 * Each letter is one of:
 *	. normal character
 *	b binary character
 *	c control character
 */
	static void
ichardef(s)
	char *s;
{
	register char *cp;
	register int n;
	register char v;

	n = 0;
	v = 0;
	cp = chardef;
	while (*s != '\0')
	{
		switch (*s++)
		{
		case '.':
			v = 0;
			break;
		case 'c':
			v = IS_CONTROL_CHAR;
			break;
		case 'b':
			v = IS_BINARY_CHAR|IS_CONTROL_CHAR;
			break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			n = (10 * n) + (s[-1] - '0');
			continue;

		default:
			error("invalid chardef", NULL_PARG);
			quit(QUIT_ERROR);
			/*NOTREACHED*/
		}

		do
		{
			if (cp >= chardef + sizeof(chardef))
			{
				error("chardef longer than 256", NULL_PARG);
				quit(QUIT_ERROR);
				/*NOTREACHED*/
			}
			*cp++ = v;
		} while (--n > 0);
		n = 0;
	}

	while (cp < chardef + sizeof(chardef))
		*cp++ = v;
}

/*
 * Define a charset, given a charset name.
 * The valid charset names are listed in the "charsets" array.
 */
	static int
icharset(name)
	register char *name;
{
	register struct charset *p;
	register struct cs_alias *a;

	if (name == NULL || *name == '\0')
		return (0);

	/* First see if the name is an alias. */
	for (a = cs_aliases;  a->name != NULL;  a++)
	{
		if (strcasecmp(name, a->name) == 0)
		{
			name = a->oname;
			break;
		}
	}

	p = search_charset(name);
	if (p)
	{
		ichardef(p->desc);
		if (p->p_flag != NULL)
			*(p->p_flag) = 1;
#if ISO
		init_def_scs_es(p->scs, p->input, p->inputr, p->output);
#endif
		return (1);
	}

	error("invalid charset name", NULL_PARG);
	quit(QUIT_ERROR);
	/*NOTREACHED*/
	return (0);
}

#if HAVE_LOCALE
/*
 * Define a charset, given a locale name.
 */
	static void
ilocale()
{
	register int c;
#if ISO
	/*
	 * We cannot trust in a system's ctype because it
	 * cannot treat any coding system are not like EUC.
	 */
	register char *name;
	register struct charlocale *p;

#if MSB_ENABLE
	/* HP-UX is used LC_COLLATE to specify codes in the regexp library. */
	(void) setlocale(LC_COLLATE, "");
#endif
	name = setlocale(LC_CTYPE, "");
#ifdef __hpux
	if (name != NULL)
		name = getlocale(LOCALE_STATUS)->LC_CTYPE_D;
#endif
	/*
	 * Search some environment variable like a setlocale()
	 * because some poor system's setlocale treat only
	 * system's local locale.
	 */
	if (name == NULL)
		name = getenv("LC_CTYPE");
	if (name == NULL)
		name = getenv("LANG");
	for (p = charlocales; name && p->name != NULL; p++)
	{
		if (strcasecmp(name, p->name) == 0)
		{
			(void) icharset(p->charset);
			return;
		}
	}
#endif

	setlocale(LC_ALL, "");
	for (c = 0;  c < (int) sizeof(chardef);  c++)
	{
		if (isprint(c))
			chardef[c] = 0;
		else if (iscntrl(c))
			chardef[c] = IS_CONTROL_CHAR;
		else
			chardef[c] = IS_BINARY_CHAR|IS_CONTROL_CHAR;
	}
#if ISO
	init_def_scs_es(SCSASCII, ESNOCONV, ESNOCONV, ESNOCONV);
#endif
}
#endif

/*
 * Define the printing format for control chars.
 */
   	public void
setbinfmt(s, fmtvarptr, default_fmt)
	char *s;
	char **fmtvarptr;
	char *default_fmt;
{
	if (s && utf_mode)
	{
		char *t = s;
		while (*t)
		{
			if (*t < ' ' || *t > '~')
			{
				s = default_fmt;
				goto attr;
			}
			t++;
		}
	}

	/* %n is evil */
	if (s == NULL 
	    || *s == '\0'
	    || (*s == '*' 
		&& (s[1] == '\0' || s[2] == '\0' || strchr(s + 2, 'n')))
	    || (*s != '*' && strchr(s, 'n')))
		s = default_fmt;

	/*
	 * Select the attributes if it starts with "*".
	 */
 attr:
	if (*s == '*')
	{
		switch (s[1])
		{
		case 'd':  binattr = AT_BOLD;      break;
		case 'k':  binattr = AT_BLINK;     break;
		case 's':  binattr = AT_STANDOUT;  break;
		case 'u':  binattr = AT_UNDERLINE; break;
		default:   binattr = AT_NORMAL;    break;
		}
		s += 2;
	}
	*fmtvarptr = s;
}

/*
 * Initialize planeset data structures.
 */
	public void
init_planeset()
{
	char *s;

#if ISO
	s = lgetenv("JLESSPLANESET");
	if (s == NULL)
		s = DEFPLANESET;
	if (set_planeset(s) < 0)
	{
		error("invalid plane set", NULL_PARG);
		quit(1);
		/*NOTREACHED*/
	}
#endif
}

/*
 * Initialize UCS width
 */
	public void
init_utfwidth(s)
char *s;
{
	static struct st_ucsw {
		char *name;
		UWidth uw;
	} ucsw[] = {
		"none",     UWIDTH_NONE,
		"normal",   UWIDTH_NORMAL,
		"default",  UWIDTH_NORMAL,
		"latin",    UWIDTH_NORMAL,
		"cjk",      UWIDTH_CJK,
		"ja",       UWIDTH_JA,
		"japan",    UWIDTH_JA,
		"japanese", UWIDTH_JA,
		"almost",   UWIDTH_ALMOST,
		"all",      UWIDTH_ALL,
		NULL,       0,
	}, *p;

	if (!s)
		return;
	for (p = ucsw; p->name; ++ p) {
		if (strcasecmp(p->name, s) == 0) {
			set_utfwidth(p->uw);
			return;
		}
	}

	return;
}

/*
 * Initialize charset data structures.
 */
	public void
init_charset()
{
	register char *s;

	s = lgetenv("LESSBINFMT");
	setbinfmt(s, &binfmt, "*s<%02X>");

	s = lgetenv("LESSUTFBINFMT");
	setbinfmt(s, &utfbinfmt, "<U+%04lX>");

	s = lgetenv("JLESSUTFWIDTH");
	init_utfwidth(s);

#if JAPANESE
	/*
	 * See if option -K is defined.
	 */
	s = opt_charset;
	if (icharset(s))
		return;
#endif
#if ISO
	/*
	 * See if environment variable JLESSCHARSET is defined.
	 */
	s = lgetenv("JLESSCHARSET");
	if (icharset(s))
		return;
#endif
	/*
	 * See if environment variable LESSCHARSET is defined.
	 */
	s = lgetenv("LESSCHARSET");
	if (icharset(s))
		return;
	/*
	 * JLESSCHARSET and LESSCHARSET are not defined: try LESSCHARDEF.
	 */
	s = lgetenv("LESSCHARDEF");
	if (s != NULL && *s != '\0')
	{
		ichardef(s);
		return;
	}

#if HAVE_LOCALE
	/*
	 * Use setlocale.
	 */
	ilocale();
#else
#if MSDOS_COMPILER
	/*
	 * Default to "dos".
	 */
	(void) icharset("dos");
#else
#if HAVE_STRSTR
	/*
	 * Check whether LC_ALL, LC_CTYPE or LANG look like UTF-8 is used.
	 */
	if ((s = lgetenv("LC_ALL")) != NULL ||
	    (s = lgetenv("LC_CTYPE")) != NULL ||
	    (s = lgetenv("LANG")) != NULL)
	{
		if (strstr(s, "UTF-8") != NULL 
		    || strstr(s, "utf-8") != NULL
		    || strstr(s, "UTF8") != NULL
		    || strstr(s, "utf8") != NULL)
			if (icharset("utf8"))
				return;
	}
#endif
	/*
	 * All variables are not defined either, default to DEFCHARSET.
	 * DEFCHARSET is defined in defines.h.
	 */
	(void) icharset(DEFCHARSET);
#endif
#endif
}

/*
 * Is a given character a "binary" character?
 */
	public int
binary_char(c)
	unsigned char c;
{
	c &= 0377;
	return (chardef[c] & IS_BINARY_CHAR);
}

/*
 * Is a given character a "control" character?
 */
	public int
control_char(c)
	int c;
{
	c &= 0377;
	return (chardef[c] & IS_CONTROL_CHAR);
}

#if ISO
/*
 * Change a database to check "control" character.
 *  This function is called by multi.c only to support iso2022 charset.
 */
	public void
change_control_char(c, flag)
	int c, flag;
{
	c &= 0377;
	if (flag)
		chardef[c] |= IS_CONTROL_CHAR;
	else
		chardef[c] &= ~IS_CONTROL_CHAR;
}
#endif

/*
 * Return the printable form of a character.
 * For example, in the "ascii" charset '\3' is printed as "^C".
 */
	public char *
prchar(c, cs)
	int c;
	CHARSET cs;
{
	static char buf[32];
	static unsigned long ucs = 0;
	*buf = 0;

	c &= 0377;
	if (CS2CHARSET(cs) == WRONGUCS_H)
		ucs = c & 0x1f;
	else if (CS2CHARSET(cs) == WRONGUCS_M)
		ucs = ucs << 6 | (c & 0x3f);
	else if (CS2CHARSET(cs) == WRONGUCS_T)
		snprintf(buf, sizeof(buf), utfbinfmt, ucs << 6 | (c & 0x3f));
	else if (CSISWRONG(cs) && c > 127)
		snprintf(buf, sizeof(buf), binfmt, c);
	else if (!control_char(c))
		snprintf(buf, sizeof(buf), "%c", c);
	else if (c == ESC)
		snprintf(buf, sizeof(buf), "ESC");
#if IS_EBCDIC_HOST
	else if (!binary_char(c) && c < 64)
		snprintf(buf,  sizeof(buf), "^%c",
		/*
		 * This array roughly inverts CONTROL() #defined in less.h,
	 	 * and should be kept in sync with CONTROL() and IBM-1047.
 	 	 */
		"@ABC.I.?...KLMNO"
		"PQRS.JH.XY.."
		"\\]^_"
		"......W[.....EFG"
		"..V....D....TU.Z"[c]);
#else
  	else if (c < 128 && !control_char(c ^ 0100))
  		snprintf(buf, sizeof(buf), "^%c", c ^ 0100);
#endif
	else
		snprintf(buf, sizeof(buf), binfmt, c);
	return (buf);
}
