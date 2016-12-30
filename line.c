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
 * Routines to manipulate the "line buffer".
 * The line buffer holds a line of output as it is being built
 * in preparation for output to the screen.
 */

#include "less.h"

#define IS_CONT(c)  (((c) & 0xC0) == 0x80)

public char *linebuf = NULL;	/* Buffer which holds the current output line */
static CHARSET *charset = NULL;	/* Extension of linebuf to hold character set */
static char *attr = NULL;	/* Extension of linebuf to hold attributes */
public int size_linebuf = 0;	/* Size of line buffer (and attr buffer) */

public int cshift;		/* Current left-shift of output line buffer */
public int hshift;		/* Desired left-shift of output line buffer */
public int tabstops[TABSTOP_MAX] = { 0 }; /* Custom tabstops */
public int ntabstops = 1;	/* Number of tabstops */
public int tabdefault = 8;	/* Default repeated tabstops */

static int curr;		/* Index into linebuf */
static int column;		/* Printable length, accounting for
				   backspaces, etc. */
static int overstrike;		/* Next char should overstrike previous char */
static int last_overstrike = AT_NORMAL;
static int is_null_line;	/* There is no current line */
static int lmargin;		/* Left margin */
static int hilites;		/* Number of hilites in this line */
static char pendc;
static POSITION pendpos;
static char *end_ansi_chars;

static int pwidth();
static int do_append();

extern int bs_mode;
extern int linenums;
extern int ctldisp;
extern int twiddle;
extern int binattr;
extern int status_col;
extern int auto_wrap, ignaw;
extern int bo_s_width, bo_e_width;
extern int ul_s_width, ul_e_width;
extern int bl_s_width, bl_e_width;
extern int so_s_width, so_e_width;
extern int sc_width, sc_height;
extern IFILE curr_ifile;

extern int utf_mode;
extern POSITION start_attnpos;
extern POSITION end_attnpos;

/*
 * Initialize from environment variables.
 */
	public void
init_line()
{
	end_ansi_chars = lgetenv("LESSANSIENDCHARS");
	if (end_ansi_chars == NULL || *end_ansi_chars == '\0')
		end_ansi_chars = "m";
	linebuf = (char *) ecalloc(LINEBUF_SIZE, sizeof(char));
	charset = (CHARSET *) ecalloc(LINEBUF_SIZE, sizeof(CHARSET));
	attr = (char *) ecalloc(LINEBUF_SIZE, sizeof(char));
	size_linebuf = LINEBUF_SIZE;
}

/*
 * Expand the line buffer.
 */
 	static int
expand_linebuf()
{
	int new_size = size_linebuf + LINEBUF_SIZE;
	char *new_buf = (char *) calloc(new_size, sizeof(char));
	CHARSET *new_charset = (CHARSET *) calloc(new_size, sizeof(CHARSET));
	char *new_attr = (char *) calloc(new_size, sizeof(char));
	if (new_buf == NULL || new_charset == NULL || new_attr == NULL)
	{
		if (new_attr != NULL)
			free(new_attr);
		if (new_charset != NULL)
			free(new_charset);
		if (new_buf != NULL)
			free(new_buf);
		return 1;
	}
	memcpy(new_buf, linebuf, size_linebuf * sizeof(char));
	memcpy(new_charset, charset, size_linebuf * sizeof(CHARSET));
	memcpy(new_attr, attr, size_linebuf * sizeof(char));
	free(attr);
	free(charset);
	free(linebuf);
	linebuf = new_buf;
	charset = new_charset;
	attr = new_attr;
	size_linebuf = new_size;
	return 0;
}

/*
 * Rewind the line buffer.
 */
	public void
prewind()
{
	curr = 0;
	column = 0;
	overstrike = 0;
	is_null_line = 0;
	pendc = '\0';
	lmargin = 0;
	if (status_col)
		lmargin += 1;
#if HILITE_SEARCH
	hilites = 0;
#endif
}

/*
 * Insert the line number (of the given position) into the line buffer.
 */
	public void
plinenum(pos)
	POSITION pos;
{
	register LINENUM linenum = 0;
	register int i;

	if (linenums == OPT_ONPLUS)
	{
		/*
		 * Get the line number and put it in the current line.
		 * {{ Note: since find_linenum calls forw_raw_line,
		 *    it may seek in the input file, requiring the caller 
		 *    of plinenum to re-seek if necessary. }}
		 * {{ Since forw_raw_line modifies linebuf, we must
		 *    do this first, before storing anything in linebuf. }}
		 */
		linenum = find_linenum(pos);
	}

	/*
	 * Display a status column if the -J option is set.
	 */
	if (status_col)
	{
		linebuf[curr] = ' ';
		charset[curr] = ASCII;
		if (start_attnpos != NULL_POSITION &&
		    pos >= start_attnpos && pos < end_attnpos)
			attr[curr] = AT_STANDOUT;
		else
			attr[curr] = 0;
		curr++;
		column++;
	}
	/*
	 * Display the line number at the start of each line
	 * if the -N option is set.
	 */
	if (linenums == OPT_ONPLUS)
	{
		char buf[INT_STRLEN_BOUND(pos) + 2];
		int n;

		linenumtoa(linenum, buf);
		n = strlen(buf);
		if (n < MIN_LINENUM_WIDTH)
			n = MIN_LINENUM_WIDTH;
		sprintf(linebuf+curr, "%*s ", n, buf);
		n++;  /* One space after the line number. */
		for (i = 0; i < n; i++) {
			charset[curr+i] = ASCII;
			attr[curr+i] = AT_NORMAL;
		}
		curr += n;
		column += n;
		lmargin += n;
	}

	/*
	 * Append enough spaces to bring us to the lmargin.
	 */
	while (column < lmargin)
	{
		linebuf[curr] = ' ';
		charset[curr] = ASCII;
		attr[curr++] = AT_NORMAL;
		column++;
	}
}

/*
 * Determine how many characters are required to shift N columns.
 */
	static int
shift_chars(s, len)
	char *s;
	int len;
{
	char *p = s;

	/*
	 * Each char counts for one column, except ANSI color escape
	 * sequences use no columns since they don't move the cursor.
	 */
	while (*p != '\0' && len > 0)
	{
		if (*p++ != ESC)
		{
			len--;
		} else
		{
			while (*p != '\0')
			{
				if (is_ansi_end(*p++))
					break;
			}
		}
	}
	return (p - s);
}

/*
 * Determine how many characters are required to shift N columns (UTF version).
 * {{ FIXME: what about color escape sequences in UTF mode? }}
 */
	static int
utf_shift_chars(s, len)
	char *s;
	int len;
{
	int ulen = 0;

	while (*s != '\0' && len > 0)
	{
		if (!IS_CONT(*s))
			len--;
		s++;
		ulen++;
	}
	while (IS_CONT(*s))
	{
		s++;
		ulen++;
	}
	return (ulen);
}

/*
 * Shift the input line left.
 * This means discarding N printable chars at the start of the buffer.
 */
	static void
pshift(shift)
	int shift;
{
	int i;
	int nchars;
	int j;
	int real_shift;			/* exact columns to shift */
	int exact_length;		/* exact bytes to shift */
	int padding;			/* columns for padding */

	if (shift > column - lmargin)
		shift = column - lmargin;
	if (shift > curr - lmargin)
		shift = curr - lmargin;

#if ISO
	/*
	 * Calculate exact bytes to shift.
	 *
	 * We would like to shift linebuf, charset, and attr by "shift"
	 * characters.  The problem is we don't know how many bytes we
	 * need to shift.  So, calculate it first.
	 */
	padding = 0;
	real_shift = 0;
	exact_length = 0;
	/*
	 * Skip rest of multi bytes character.
	 */
	for (j = lmargin; j < curr && pwidth(linebuf[j], charset[j], attr[j]) == 0; j++)
	{
		padding++;
		exact_length++;
	}
	/*
	 * Calculate how many bytes we need to shift.
	 */
	for (; j < curr && real_shift < shift; j++)
	{
		real_shift += pwidth(linebuf[j], charset[j], attr[j]);
		exact_length++;
	}
	/*
	 * Skip following rest bytes of a last multi bytes character.
	 */
	for (; j < curr && pwidth(linebuf[j], charset[j], attr[j]) == 0; j++)
	{
		exact_length++;
	}

	/*
	 * Put characters.
	 */
	for (i = 0;  i < padding;  i++)
	{
		linebuf[lmargin + i] = ' ';
		charset[lmargin + i] = ASCII;
		attr[lmargin + i] = AT_NORMAL;;
	}
	for (i = 0;  i < curr - exact_length;  i++)
	{
		linebuf[lmargin + i + padding] = linebuf[lmargin + i + exact_length];
		charset[lmargin + i + padding] = charset[lmargin + i + exact_length];
		attr[lmargin + i + padding] = attr[lmargin + i + exact_length];
	}

	curr -= exact_length;
#else
	if (utf_mode)
		nchars = utf_shift_chars(linebuf + lmargin, shift);
	else
		nchars = shift_chars(linebuf + lmargin, shift);
	if (nchars > curr)
		nchars = curr;
	for (i = 0;  i < curr - nchars;  i++)
	{
		linebuf[lmargin + i] = linebuf[lmargin + i + nchars];
		charset[lmargin + i] = charset[lmargin + i + nchars];
		attr[lmargin + i] = attr[lmargin + i + nchars];
	}

	curr -= nchars;
#endif
	column -= shift;
	cshift += shift;
}

/*
 * Return the printing width of the start (enter) sequence
 * for a given character attribute.
 */
	static int
attr_swidth(a)
	int a;
{
	switch (a)
	{
	case AT_BOLD:		return (bo_s_width);
	case AT_UNDERLINE:	return (ul_s_width);
	case AT_BLINK:		return (bl_s_width);
	case AT_STANDOUT:	return (so_s_width);
	}
	return (0);
}

/*
 * Return the printing width of the end (exit) sequence
 * for a given character attribute.
 */
	static int
attr_ewidth(a)
	int a;
{
	switch (a)
	{
	case AT_BOLD:		return (bo_e_width);
	case AT_UNDERLINE:	return (ul_e_width);
	case AT_BLINK:		return (bl_e_width);
	case AT_STANDOUT:	return (so_e_width);
	}
	return (0);
}

/*
 * Return the printing width of a given character and attribute,
 * if the character were added to the current position in the line buffer.
 * Adding a character with a given attribute may cause an enter or exit
 * attribute sequence to be inserted, so this must be taken into account.
 */
	static int
pwidth(c, cs, a)
	int c;
	CHARSET cs;
	int a;
{
	register int w;

	if (utf_mode && IS_CONT(c))
		return (0);

	if (c == '\b')
		/*
		 * Backspace moves backwards one position.
		 */
		return (-1);

	if (control_char(c))
		/*
		 * Control characters do unpredicatable things,
		 * so we don't even try to guess; say it doesn't move.
		 * This can only happen if the -r flag is in effect.
		 */
		return (0);

	/*
	 * Other characters take one space,
	 * plus the width of any attribute enter/exit sequence.
	 */
#if ISO
	w = mwidth(c, cs);
#else
	w = 1;
#endif
	if (curr > 0 && attr[curr-1] != a)
		w += attr_ewidth(attr[curr-1]);
	if (a && (curr == 0 || attr[curr-1] != a))
		w += attr_swidth(a);
	return (w);
}

/*
 * Delete the previous character in the line buffer.
 */
	static void
backc()
{
	/* remove garbage in the buffer. */
	if (CSISREST(charset[curr]))
		charset[curr] = 0;
	/* delete the previous character. */
	do
	{
		curr--;
		column -= pwidth(linebuf[curr], charset[curr], attr[curr]);
	} while (curr > 0 && CSISREST(charset[curr]));
}

/*
 * Are we currently within a recognized ANSI escape sequence?
 */
	static int
in_ansi_esc_seq()
{
	int i;

	/*
	 * Search backwards for either an ESC (which means we ARE in a seq);
	 * or an end char (which means we're NOT in a seq).
	 */
	for (i = curr-1;  i >= 0;  i--)
	{
		if (linebuf[i] == ESC)
			return (1);
		if (is_ansi_end(linebuf[i]))
			return (0);
	}
	return (0);
}

/*
 * Is a character the end of an ANSI escape sequence?
 */
	public int
is_ansi_end(c)
	char c;
{
	return (strchr(end_ansi_chars, c) != NULL);
}

/*
 * Append a character and attribute to the line buffer.
 */
#define	STORE_CHAR(c,cs,n,a,pos) \
	do { if (store_char((c),(cs),(n),(a),(pos))) return (1); else curr++; } while (0)

	static int
store_char(c, cs, n, a, pos)
	int c;
	CHARSET cs;
	int n;
	int a;
	POSITION pos;
{
	register int w;

	if (a != AT_NORMAL)
		last_overstrike = a;
#if HILITE_SEARCH
	if (is_hilited(pos, pos+n, 0))
	{
		/*
		 * This character should be highlighted.
		 * Override the attribute passed in.
		 */
		a = AT_STANDOUT;
		hilites++;
	}
#endif
	if (ctldisp == OPT_ONPLUS && in_ansi_esc_seq())
		w = 0;
	else
		w = pwidth(c, cs, a);
	if (ctldisp != OPT_ON && column + w + attr_ewidth(a) > sc_width)
		/*
		 * Won't fit on screen.
		 */
		return (1);

	if (curr >= size_linebuf-2)
	{
		/*
		 * Won't fit in line buffer.
		 * Try to expand it.
		 */
		if (expand_linebuf())
			return (1);
	}

	/*
	 * Special handling for "magic cookie" terminals.
	 * If an attribute enter/exit sequence has a printing width > 0,
	 * and the sequence is adjacent to a space, delete the space.
	 * We just mark the space as invisible, to avoid having too
	 * many spaces deleted.
	 * {{ Note that even if the attribute width is > 1, we
	 *    delete only one space.  It's not worth trying to do more.
	 *    It's hardly worth doing this much. }}
	 */
	if (curr > 0 && a != AT_NORMAL && 
		linebuf[curr-1] == ' ' && charset[curr-1] == ASCII && attr[curr-1] == AT_NORMAL &&
		attr_swidth(a) > 0)
	{
		/*
		 * We are about to append an enter-attribute sequence
		 * just after a space.  Delete the space.
		 */
		attr[curr-1] = AT_INVIS;
		column--;
	} else if (curr > 0 && attr[curr-1] != AT_NORMAL && 
		attr[curr-1] != AT_INVIS && c == ' ' && a == AT_NORMAL &&
		attr_ewidth(attr[curr-1]) > 0)
	{
		/*
		 * We are about to append a space just after an 
		 * exit-attribute sequence.  Delete the space.
		 */
		a = AT_INVIS;
		column--;
	}
	/* End of magic cookie handling. */

	linebuf[curr] = c;
	charset[curr] = cs;
	attr[curr] = a;
	column += w;
	return (0);
}

/*
 * Append a tab to the line buffer.
 * Store spaces to represent the tab.
 */
#define	STORE_TAB(a,pos) \
	do { if (store_tab((a),(pos))) return (1); } while (0)

	static int
store_tab(attr, pos)
	int attr;
	POSITION pos;
{
	int to_tab = column + cshift - lmargin;
	int i;
	CHARSET tmpcs = ASCII;

	if (ntabstops < 2 || to_tab >= tabstops[ntabstops-1])
		to_tab = tabdefault -
		     ((to_tab - tabstops[ntabstops-1]) % tabdefault);
	else
	{
		for (i = ntabstops - 2;  i >= 0;  i--)
			if (to_tab >= tabstops[i])
				break;
		to_tab = tabstops[i+1] - to_tab;
	}

	do {
		STORE_CHAR(' ', tmpcs, 1, attr, pos);
	} while (--to_tab > 0);
	return 0;
}

/*
 * Append multiple characters to the line buffer.
 * Expand tabs into spaces, handle underlining, boldfacing, etc.
 * Returns 0 if ok, 1 if couldn't fit in buffer.
 */
	public int
pappend_multi(mbd, mpos)
	M_BUFDATA *mbd;
	POSITION *mpos;
{
	char *cbuf = mbd->cbuf;
	CHARSET *csbuf = mbd->csbuf;
	int byte = mbd->byte;
	POSITION pos = *mpos;
	int r;
	int saved_curr;
	int saved_column;
	int saved_overstrike;
	int saved_last_overstrike;
	int saved_hilites;
	int saved_cshift;
	int i;

	if (byte == 0)
		return (0);
	if (pendc)
	{
		if (do_append(pendc, control_char(pendc) ? WRONGCS :
			ASCII, 1, pendpos))
			/*
			 * Oops.  We've probably lost the char which
			 * was in pendc, since caller won't back up.
			 */
			return (1);
		pendc = '\0';
	}

	if (*cbuf == '\r' && byte == 1 && bs_mode == BS_SPECIAL)
	{
		/*
		 * Don't put the CR into the buffer until we see 
		 * the next char.  If the next char is a newline,
		 * discard the CR.
		 */
		pendc = *cbuf;
		pendpos = pos;
		return (0);
	}

	/*
	 * Save several variables.
	 */
	saved_curr = curr;
	saved_column = column;
	saved_overstrike = overstrike;
	saved_last_overstrike = last_overstrike;
	saved_hilites = hilites;
	saved_cshift = cshift;
	r = 0;
	for (i = 0; i < byte && r == 0; i++)
	{
		r = do_append(cbuf[i], csbuf[i], byte - i, pos);
	}
	curr = saved_curr;
	column = saved_column;
	overstrike = saved_overstrike;
	last_overstrike = saved_last_overstrike;
	hilites = saved_hilites;
	cshift = saved_cshift;
	if (r != 0)
	{
		return (r);
	}

	for (i = 0; i < byte && r == 0; i++)
	{
		r = do_append(cbuf[i], csbuf[i], byte - i, pos);
	}

	/*
	 * If we need to shift the line, do it.
	 * But wait until we get to at least the middle of the screen,
	 * so shifting it doesn't affect the chars we're currently
	 * pappending.  (Bold & underline can get messed up otherwise.)
	 */
	if (cshift < hshift && column > sc_width / 2)
	{
		linebuf[curr] = '\0';
		pshift(hshift - cshift);
	}
	return (r);
}

/*
 * Append a character to the line buffer.
 * Expand tabs into spaces, handle underlining, boldfacing, etc.
 * Returns 0 if ok, 1 if couldn't fit in buffer.
 */
	public int
pappend(c, cs, i, pos)
	register int c;
	CHARSET cs;
	int i;
	POSITION pos;
{
	int r;

	if (pendc)
	{
		if (do_append(pendc, control_char(pendc) ? WRONGCS :
			ASCII, 1, pendpos))
			/*
			 * Oops.  We've probably lost the char which
			 * was in pendc, since caller won't back up.
			 */
			return (1);
		pendc = '\0';
	}

	if (c == '\r' && bs_mode == BS_SPECIAL)
	{
		/*
		 * Don't put the CR into the buffer until we see 
		 * the next char.  If the next char is a newline,
		 * discard the CR.
		 */
		pendc = c;
		pendpos = pos;
		return (0);
	}

	r = do_append(c, cs, i, pos);
	/*
	 * If we need to shift the line, do it.
	 * But wait until we get to at least the middle of the screen,
	 * so shifting it doesn't affect the chars we're currently
	 * pappending.  (Bold & underline can get messed up otherwise.)
	 */
	if (cshift < hshift && column > sc_width / 2)
	{
		linebuf[curr] = '\0';
		pshift(hshift - cshift);
	}
	return (r);
}

#define IS_UTF8_4BYTE(c) ( ((c) & 0xf8) == 0xf0 )
#define IS_UTF8_3BYTE(c) ( ((c) & 0xf0) == 0xe0 )
#define IS_UTF8_2BYTE(c) ( ((c) & 0xe0) == 0xc0 )
#define IS_UTF8_TRAIL(c) ( ((c) & 0xc0) == 0x80 )

	static int
do_append(c, cs, n, pos)
	int c;
	CHARSET cs;
	int n;
	POSITION pos;
{
	register char *s;
	register int a;

#define STOREC(c,cs,n,a) \
	if ((c) == '\t') STORE_TAB((a),pos); else STORE_CHAR((c),(cs),(n),(a),pos)

#if ISO
	if (overstrike)
	{
		/*
		 * Check about multi '\b' for multi bytes character.
		 */
		if (c == '\b')
		{
			if (linebuf[curr] == '_' && CSISASCII(charset[curr]))
				goto do_bs_char; /* do backc on underline */
			else
				return (0);		/* ignore it */
		}
	}
#endif
	if (CSISWRONG(cs) && c != '\b' && c != '\t')
	{
		goto do_control_char;
	} else if (c == '\b' && CSISWRONG(cs))
	{
	do_bs_char:
		switch (bs_mode)
		{
		case BS_NORMAL:
			STORE_CHAR(c, cs, n, AT_NORMAL, pos);
			break;
		case BS_CONTROL:
			goto do_control_char;
		case BS_SPECIAL:
			if (curr == 0)
				goto do_control_char;
			if (CSISWRONG(charset[curr - 1]))
				goto do_control_char;
			backc();
			overstrike = 1;
			break;
		}
	} else if (overstrike)
	{
		/*
		 * Overstrike the character at the current position
		 * in the line buffer.  This will cause either 
		 * underline (if a "_" is overstruck), 
		 * bold (if an identical character is overstruck),
		 * or just deletion of the character in the buffer.
		 */
		overstrike--;
		if (utf_mode && IS_UTF8_4BYTE(c) && curr > 2 && (char)c == linebuf[curr-3])
		{
			backc();
			backc();
			backc();
			STORE_CHAR(linebuf[curr], cs, n, AT_BOLD, pos);
			overstrike = 3;
		} else if (utf_mode && (IS_UTF8_3BYTE(c) || (overstrike==2 && IS_UTF8_TRAIL(c))) && curr > 1 && (char)c == linebuf[curr-2])
		{
			backc();
			backc();
			STORE_CHAR(linebuf[curr], cs, n, AT_BOLD, pos);
			overstrike = 2;
		} else if (utf_mode && curr > 0 && (IS_UTF8_2BYTE(c) || (overstrike==1 && IS_UTF8_TRAIL(c))) && (char)c == linebuf[curr-1])
		{
			backc();
			STORE_CHAR(linebuf[curr], cs, n, AT_BOLD, pos);
			overstrike = 1;
		} else if (utf_mode && curr > 0 && IS_UTF8_TRAIL(c) && attr[curr-1] == AT_UNDERLINE)
		{
			STOREC(c, cs, n, AT_UNDERLINE);
		} else if ((char)c == linebuf[curr] && charset[curr] == cs)
		{
			/*
			 * Overstriking a char with itself means make it bold.
			 * But overstriking an underscore with itself is
			 * ambiguous.  It could mean make it bold, or
			 * it could mean make it underlined.
			 * Use the previous overstrike to resolve it.
			 */
			if (c == '_' && last_overstrike != AT_NORMAL)
				STOREC(c, cs, n, last_overstrike);
			else
				STOREC(c, cs, n, AT_BOLD);
		} else if (c == '_' && CSISASCII(cs))
		{
			if (utf_mode)
			{
				int i;
				for (i = 0;  i < 5;  i++)
				{
					if (curr <= i || !IS_CONT(linebuf[curr-i]))
						break;
					attr[curr-i-1] = AT_UNDERLINE;
				}
			}
			STOREC(linebuf[curr], charset[curr], n, AT_UNDERLINE);
#if ISO
			while (CSISREST(charset[curr]))
				STOREC(linebuf[curr], charset[curr], --n,
				       AT_UNDERLINE);
#endif
		} else if (linebuf[curr] == '_' && CSISASCII(charset[curr]))
		{
			if (utf_mode)
			{
				if (IS_UTF8_2BYTE(c))
					overstrike = 1;
				else if (IS_UTF8_3BYTE(c))
					overstrike = 2;
				else if (IS_UTF8_4BYTE(c))
					overstrike = 3;
			}
			STOREC(c, cs, n, AT_UNDERLINE);
		} else if (CSISWRONG(cs) && control_char(c))
			goto do_control_char;
		else
			STOREC(c, cs, n, AT_NORMAL);
	} else if (c == '\t' && CSISWRONG(cs)) 
	{
		/*
		 * Expand a tab into spaces.
		 */
		switch (bs_mode)
		{
		case BS_CONTROL:
			goto do_control_char;
		case BS_NORMAL:
		case BS_SPECIAL:
			STORE_TAB(AT_NORMAL, pos);
			break;
		}
	} else if (CSISWRONG(cs) && control_char(c))
	{
	do_control_char:
		if (ctldisp == OPT_ON || (ctldisp == OPT_ONPLUS && c == ESC))
		{
			/*
			 * Output as a normal character.
			 */
			STORE_CHAR(c, cs, n, AT_NORMAL, pos);
		} else 
		{
			/*
			 * Convert to printable representation.
			 */
			s = prchar(c, cs);  
			a = binattr;

			/*
			 * Make sure we can get the entire representation
			 * of the character on this line.
			 */
			if (column + (int) strlen(s) + 
			    attr_swidth(a) + attr_ewidth(a) > sc_width)
				return (1);

			for ( ;  *s != 0;  s++)
				STORE_CHAR(*s, WRONGCS, 1, a, pos);
		}
#if ISO
	} else if (CSISREST(cs))
	{
		STOREC(c, cs, n, attr[curr - 1]);
#endif
	} else
	{
		STOREC(c, cs, n, AT_NORMAL);
	}

	return (0);
}

/*
 * Terminate the line in the line buffer.
 */
	public void
pdone(endline)
	int endline;
{
	if (pendc && (pendc != '\r' || !endline))
		/*
		 * If we had a pending character, put it in the buffer.
		 * But discard a pending CR if we are at end of line
		 * (that is, discard the CR in a CR/LF sequence).
		 */
		(void) do_append(pendc, control_char(pendc) ? WRONGCS :
			ASCII, 1, pendpos);

	/*
	 * Make sure we've shifted the line, if we need to.
	 */
	if (cshift < hshift)
		pshift(hshift - cshift);

	/*
	 * Add a newline if necessary,
	 * and append a '\0' to the end of the line.
	 */
	if (column < sc_width || !auto_wrap || ignaw || ctldisp == OPT_ON)
	{
		linebuf[curr] = '\n';
		charset[curr] = ASCII;
		attr[curr] = AT_NORMAL;
		curr++;
	}
	linebuf[curr] = '\0';
	charset[curr] = ASCII;
	attr[curr] = AT_NORMAL;

#if HILITE_SEARCH
	if (status_col && hilites > 0)
	{
		linebuf[0] = '*';
		charset[0] = ASCII;
		attr[0] = AT_STANDOUT;
	}
#endif
	/*
	 * If we are done with this line, reset the current shift.
	 */
	if (endline)
		cshift = 0;
}

/*
 * Get a character from the current line.
 * Return the character as the function return value,
 * and the character attribute in *ap.
 */
	public int
gline(i, csp, ap)
	register int i;
	register int *csp;
	register int *ap;
{
	char *s;
	
	if (is_null_line)
	{
		/*
		 * If there is no current line, we pretend the line is
		 * either "~" or "", depending on the "twiddle" flag.
		 */
		*csp = ASCII;
		*ap = AT_BOLD;
		s = (twiddle) ? "~\n" : "\n";
		return (s[i]);
	}

	*csp = charset[i];
	*ap = attr[i];
	return (linebuf[i] & 0377);
}

/*
 * Indicate that there is no current line.
 */
	public void
null_line()
{
	is_null_line = 1;
	cshift = 0;
}

/*
 * Analogous to forw_line(), but deals with "raw lines":
 * lines which are not split for screen width.
 * {{ This is supposed to be more efficient than forw_line(). }}
 */
	public POSITION
forw_raw_line(curr_pos, linep)
	POSITION curr_pos;
	char **linep;
{
	register int n;
	register int c;
	POSITION new_pos;

	if (curr_pos == NULL_POSITION || ch_seek(curr_pos) ||
		(c = ch_forw_get()) == EOI)
		return (NULL_POSITION);

	n = 0;
	for (;;)
	{
		if (c == '\n' || c == EOI)
		{
			new_pos = ch_tell();
			break;
		}
		if (n >= size_linebuf-1)
		{
			if (expand_linebuf())
			{
				/*
				 * Overflowed the input buffer.
				 * Pretend the line ended here.
				 */
				new_pos = ch_tell() - 1;
				break;
			}
		}
		linebuf[n++] = c;
		c = ch_forw_get();
	}
	linebuf[n] = '\0';
	if (linep != NULL)
		*linep = linebuf;
	return (new_pos);
}

/*
 * Analogous to back_line(), but deals with "raw lines".
 * {{ This is supposed to be more efficient than back_line(). }}
 */
	public POSITION
back_raw_line(curr_pos, linep)
	POSITION curr_pos;
	char **linep;
{
	register int n;
	register int c;
	POSITION new_pos;

	if (curr_pos == NULL_POSITION || curr_pos <= ch_zero() ||
		ch_seek(curr_pos-1))
		return (NULL_POSITION);

	n = size_linebuf;
	linebuf[--n] = '\0';
	for (;;)
	{
		c = ch_back_get();
		if (c == '\n')
		{
			/*
			 * This is the newline ending the previous line.
			 * We have hit the beginning of the line.
			 */
			new_pos = ch_tell() + 1;
			break;
		}
		if (c == EOI)
		{
			/*
			 * We have hit the beginning of the file.
			 * This must be the first line in the file.
			 * This must, of course, be the beginning of the line.
			 */
			new_pos = ch_zero();
			break;
		}
		if (n <= 0)
		{
			int old_size_linebuf = size_linebuf;
			char *fm;
			char *to;
			if (expand_linebuf())
			{
				/*
				 * Overflowed the input buffer.
				 * Pretend the line ended here.
				 */
				new_pos = ch_tell() + 1;
				break;
			}
			/*
			 * Shift the data to the end of the new linebuf.
			 */
			for (fm = linebuf + old_size_linebuf,
			      to = linebuf + size_linebuf;
			     fm >= linebuf;  fm--, to--)
				*to = *fm;
			n = size_linebuf - old_size_linebuf;
		}
		linebuf[--n] = c;
	}
	if (linep != NULL)
		*linep = &linebuf[n];
	return (new_pos);
}
