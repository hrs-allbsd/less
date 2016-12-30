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
	CODESET left;
	CODESET right;
	CODESET output;
} charsets[] = {
	{ "ascii",	NULL,       "8bcccbcc18b95.b",
		noconv,		none,		noconv		},
	{ "dos",	NULL,       "8bcccbcc12bc5b223.b",
		noconv,		noconv,		noconv		},
	{ "ebcdic",	NULL,       "5bc6bcc7bcc41b.9b7.9b5.b..8b6.10b6.b9.7b9.8b8.17b3.3b9.7b9.8b8.6b10.b.b.b.",
		noconv,		noconv,		noconv		},
	{ "IBM-1047",	NULL,       "4cbcbc3b9cbccbccbb4c6bcc5b3cbbc4bc4bccbc191.b",
		noconv,		noconv,		noconv		},
	{ "iso8859",	NULL,       "8bcccbcc18b95.33b.",
		noconv,		noconv,		noconv		},
	{ "koi8-r",	NULL,       "8bcccbcc18b95.b128.",
		noconv,		noconv,		noconv		},
	{ "next",	NULL,       "8bcccbcc18b95.bb125.bb",
		noconv,		noconv,		noconv		},
	{ "utf-8",	&utf_mode,  "8bcccbcc18b.",
		noconv,		noconv,		noconv		},
#if ISO
	{ "iso7",	NULL,       "8bcccb4c11bc4b96.b",
		iso7,		noconv,		iso7		},
	{ "iso8",	NULL,       "8bcccb4c11bc4b95.15b2.16b.",
		iso7,		iso8,		iso8		},
# if JAPANESE
	/* read all KANJI code sets */
	{ "japanese",		NULL,       "8bcccb4c11bc4b95.b127.b",
		jis,		japanese,	jis		},
	{ "japanese-iso7",	NULL,       "8bcccb4c11bc4b95.b127.b",
		iso7,		japanese,	iso7		},
	{ "japanese-jis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		jis,		japanese,	jis		},
	{ "japanese-ujis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		jis,		japanese,	ujis		},
	{ "japanese-euc", 	NULL,       "8bcccb4c11bc4b95.b127.b",
		jis,		japanese,	ujis		},
	{ "japanese-sjis",	NULL,       "8bcccb4c11bc4b95.b127.b",
		jis,		japanese,	sjis		},
	/* read JIS */
	{ "jis",	NULL,       "8bcccb4c11bc4b95.b",
		jis,		none,		jis		},
	{ "jis-ujis",	NULL,       "8bcccb4c11bc4b95.15b2.17b94.b",
		jis,		ujis,		ujis		},
	{ "jis-euc",	NULL,       "8bcccb4c11bc4b95.15b2.17b94.b",
		jis,		ujis,		ujis		},
	{ "jis-sjis",	NULL,       "8bcccb4c11bc4b95.b125.3b",
		jis,		sjis,		sjis		},
	/* read UJIS */
	{ "ujis",	NULL,       "8bcccbcc18b95.15b2.17b94.b",
		noconv,		ujis,		ujis		},
	{ "euc",	NULL,       "8bcccbcc18b95.15b2.17b94.b",
		noconv,		ujis,		ujis		},
	{ "ujis-iso7",	NULL,       "8bcccb4c11bc4b96.14b2.17b94.b",
		iso7,		ujis,		iso7		},
	{ "euc-iso7",	NULL,       "8bcccb4c11bc4b96.14b2.17b94.b",
		iso7,		ujis,		iso7		},
	{ "ujis-jis",	NULL,       "8bcccb4c11bc4b95.15b2.17b94.b",
		jis,		ujis,		jis		},
	{ "euc-jis",	NULL,       "8bcccb4c11bc4b95.15b2.17b94.b",
		jis,		ujis,		jis		},
	/* disallow UJIS's katakana to improve the encoding detection */
	{ "ujis-sjis",	NULL,       "8bcccbcc18b95.15b.18b94.b",
		noconv,		ujis,		sjis		},
	{ "euc-sjis",	NULL,       "8bcccbcc18b95.15b.18b94.b",
		noconv,		ujis,		sjis		},
	/* read SJIS */
	{ "sjis",	NULL,       "8bcccbcc18b95.b125.3b",
		noconv,		sjis,		sjis		},
	{ "sjis-iso7",	NULL,       "8bcccb4c11bc4b221.b",
		iso7,		sjis,		iso7		},
	{ "sjis-jis",	NULL,       "8bcccb4c11bc4b95.b125.3b",
		jis,		sjis,		jis		},
	{ "sjis-ujis",	NULL,       "8bcccbcc18b95.b125.3b",
		noconv,		sjis,		ujis		},
	{ "sjis-euc",	NULL,       "8bcccbcc18b95.b125.3b",
		noconv,		sjis,		ujis		},
# endif
#endif
	{ NULL, NULL, NULL, noconv, noconv, noconv }
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
	{ "ja_JP.jis8",		"iso8"		},
# if JAPANESE
	{ "ja_JP.JIS",		"japanese-jis"	},
	{ "ja_JP.jis7",		"japanese-jis"	},
	{ "ja_JP.EUC",		"japanese-ujis"	},
	{ "ja_JP.ujis",		"japanese-ujis"	},
	{ "ja_JP.SJIS",		"japanese-sjis"	},
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
	{ "ja",			"japanese-ujis" },
	{ "ja_JP",		"japanese-ujis" },
	{ "japan",		"japanese-ujis"	},
	{ "Japan",		"japanese-ujis"	},
	{ "japanese",		"japanese-ujis"	},
	{ "Japanese",		"japanese-ujis"	},
	/* DEC OSF/1's */
	{ "ja_JP.eucJP",	"japanese-ujis"	},
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
	{ NULL, NULL }
};

#define	IS_BINARY_CHAR	01
#define	IS_CONTROL_CHAR	02

static char chardef[256];
static char *binfmt = NULL;
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
		if (strncmp(name, p->name, namelen) == 0) {
			score += namelen;
			if ((int) strlen(p->name) == namelen)
				score++; /* add bonus point for exactly match */
		}
		if (name2 && n2 && strncmp(name2, n2, name2len) == 0) {
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
 * Return the CODESET of left plane of named charset.
 */
	public CODESET
left_codeset_of_charset(name)
	register char *name;
{
	struct charset *p = search_charset(name);

	if (p)
		return (p->left);
	return (noconv);
}

/*
 * Return the CODESET of right plane of named charset.
 */
	public CODESET
right_codeset_of_charset(name)
	register char *name;
{
	struct charset *p = search_charset(name);

	if (p)
		return (p->right);
	return (none);
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
		if (strcmp(name, a->name) == 0)
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
		init_def_codesets(p->left, p->right, p->output);
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
		if (strcmp(name, p->name) == 0)
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
	init_def_codesets(noconv, noconv, noconv);
#endif
}
#endif

/*
 * Define the printing format for control chars.
 */
   	public void
setbinfmt(s)
	char *s;
{
	if (s == NULL || *s == '\0')
		s = "*s<%X>";
	/*
	 * Select the attributes if it starts with "*".
	 */
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
	binfmt = s;
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
 * Initialize charset data structures.
 */
	public void
init_charset()
{
	register char *s;

	s = lgetenv("LESSBINFMT");
	setbinfmt(s);
	
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

#if HAVE_STRSTR
	/*
	 * Check whether LC_ALL, LC_CTYPE or LANG look like UTF-8 is used.
	 */
	if ((s = lgetenv("LC_ALL")) != NULL ||
	    (s = lgetenv("LC_CTYPE")) != NULL ||
	    (s = lgetenv("LANG")) != NULL)
	{
		if (strstr(s, "UTF-8") != NULL || strstr(s, "utf-8") != NULL)
			if (icharset("utf-8"))
				return;
	}
#endif

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
	static char buf[8];

	c &= 0377;
	if (CSISWRONG(cs) && c > 127)
		sprintf(buf, binfmt, c);
	else if (!control_char(c))
		sprintf(buf, "%c", c);
	else if (c == ESC)
		sprintf(buf, "ESC");
#if IS_EBCDIC_HOST
	else if (!binary_char(c) && c < 64)
		sprintf(buf, "^%c",
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
  		sprintf(buf, "^%c", c ^ 0100);
#endif
	else
		sprintf(buf, binfmt, c);
	return (buf);
}
