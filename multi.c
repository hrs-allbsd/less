/*
 * Copyright (c) 1994-2005  Kazushi (Jam) Marukawa
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Routines to manipulate a buffer to hold string of multi bytes character.
 * Detect a character set from input string and convert them to internal
 * codes.  And convert it to other codes to display them.
 */

#include "defines.h"
#include "less.h"

#include <stdio.h>
#include <assert.h>

#if STDC_HEADERS
#include <stdlib.h>
#include <string.h>
#endif

#define LESS 1

/* TODO: remove caller control_char(), change_control_char() and ecalloc() */
extern int control_char ();
extern void change_control_char ();
extern void* ecalloc ();


#if ISO

static void multi_reparse();


#if JAPANESE

int markwrongchar = 1;


/*
 * Kanji convetion
 */
#define ISJIS(c)		(0x21 <= (c) && (c) <= 0x7e)
#define ISUJIS(c)		(0xa1 <= (c) && (c) <= 0xfe)
#define ISUJISSS(c)		((c) == 0x8e || (c) == 0x8f)
#define ISUJISKANJI(c1,c2)	(ISUJIS(c1) && ISUJIS(c2))
#define ISUJISKANJI1(c)		(ISUJIS(c))
#define ISUJISKANA(c1,c2)	((c1) == 0x8e && ISUJIS(c2))
#define ISUJISKANA1(c)		((c) == 0x8e)
#define ISUJISKANJISUP(c1,c2,c3) ((c1) == 0x8f && ISUJIS(c2) && ISUJIS(c3))
#define ISUJISKANJISUP1(c)	((c) == 0x8f)
#define ISSJISKANJI(c1,c2)	(((0x81 <= (c1) && (c1) <= 0x9f) || \
				  (0xe0 <= (c1) && (c1) <= 0xfc)) && \
				 (0x40 <= (c2) && (c2) <= 0xfc && (c2) != 0x7f))
#define ISSJISKANJI1(c)		((0x81 <= (c) && (c) <= 0x9f) || \
				 (0xe0 <= (c) && (c) <= 0xfc))
#define ISSJISKANA(c)		(0xa1 <= (c) && (c) <= 0xdf)
#endif


/*
 * Definitions for understanding the escape sequence.
 * Following escape sequences which be understood by less:
 *  ESC 2/4 2/8,2/9,2/10,2/11,2/13,2/14,2/15 F
 *  ESC 2/4 4/0,4/1,4/2
 *  ESC 2/6 F
 *  ESC 2/8,2/9,2/10,2/11,2/13,2/14,2/15 F
 *  ESC 2/12 F		This is used in MULE.  Less support this as input.
 *  0/14,0/15
 *  ESC 4/14,4/15,6/14,6/15,7/12,7/13,7/14
 *  8/14,8/15
 */
enum escape_sequence {
    NOESC,		/* No */	ESC_,		/* ^[ */
    ESC_2_4,	/* ^[$ */	ESC_2_4_8,	/* ^[$( */
    ESC_2_4_9,	/* ^[$) */	ESC_2_4_10,	/* ^[$* */
    ESC_2_4_11,	/* ^[$+ */	ESC_2_4_13,	/* ^[$- */
    ESC_2_4_14,	/* ^[$. */	ESC_2_4_15,	/* ^[$/ */
    ESC_2_6,	/* ^[& */	ESC_2_8,	/* ^[( */
    ESC_2_9,	/* ^[) */	ESC_2_10,	/* ^[* */
    ESC_2_11,	/* ^[+ */	ESC_2_12,	/* ^[, */
    ESC_2_13,	/* ^[- */	ESC_2_14,	/* ^[. */
    ESC_2_15	/* ^[/ */
};


static CODESET def_left = iso7;		/* Default code set of left plane */
static CODESET def_right = iso8;	/* Default code set of right plane */
static int def_gs[4] = {
    ASCII,				/* Default g0 plane status */
    WRONGCS,				/* Default g1 plane status */
    WRONGCS,				/* Default g2 plane status */
    WRONGCS				/* Default g3 plane status */
};

static CODESET output = iso8;		/* Code set for output */
#if JAPANESE
static CODESET def_priority = ujis;	/* Which code was given priority. */
#endif

typedef POSITION m_position;
#define M_NULL_POS	((POSITION)(-1))

/*
 * Structure to represent character set information.
 *
 * This data set contains current character set and other information
 * to keep the status of ISO-2022 escape sequence.
 */
struct m_status {
    /* Graphi Sets */
    int gs[4];			/* Current g0..g3 plane sets. */
				/* gl, gr, and sg refer one of 4 planes. */
    int gl;			/* Current gl plane status */
    int gr;			/* Current gr plane status */
    int sg;			/* Current status of single-shifted plane */
#define WRONGPLANE		(-1)
#define ISVALIDPLANE(mp,plane)	((mp)->ms->plane != WRONGPLANE)
#define FINDCS(mp,c)	((mp)->ms->gs[(ISVALIDPLANE((mp), sg) ? (mp)->ms->sg : \
				 ((c) & 0x80) ? (mp)->ms->gr : (mp)->ms->gl)])
#define PLANE2CS(mp,plane)	((mp)->ms->gs[(mp)->ms->plane])

    int irr;			/* Identify revised registration number */
};

struct multibuf {
    struct {
	CODESET left;
	CODESET right;
    } io;

    CODESET orig_io_right;
    int rotation_io_right;

    enum escape_sequence eseq;
    /*
     * Variables to control of escape sequences as output.
     */
    int cs;			/* Current character set */
    struct m_status* ms;
#if JAPANESE
    CODESET priority;		/* Which code was given priority. */
    int sequence_counter;	/* Special counter for detect UJIS KANJI. */
#endif

    int icharset;		/* Last non ASCII character set of input */

    /*
     * Small buffers to hold all parsing bytes of multi-byte characters.
     *
     * multi_parse() function receive a sequence of byte and buffer it.
     * Each time multi_parse() recognize full data sequence to represent
     * one character, it converts the data into internal data and returns
     * converted data.
     *
     * Caller must buffer it somewhere and output it using outbuf() of
     * outchar().  Those output functions() converts internal data into
     * appropriate data stream for choosen output device.
     *
     * As internal data, we use char[] and CHARSET[] to keep byte and
     * additional information, respectively.  We choose ISO-2022 style
     * data format as our internal data format because it is most easy
     * to work with.  It has completely separated planes for each
     * character set.  This helps code conversion and others alot.
     * For example, we don't need to work to separate Chinese and
     * Japanese because they are separated from the beginning in ISO-2022
     * although UTF-8 uses only single plane with all CJK character sets.
     */
    /*
     * Buffer for input/parsing
     */
    m_position lastpos;		/* position of last byte */
    m_position startpos;	/* position of first byte buffered */
    unsigned char inbuf[20];
    m_position laststartpos;	/* position of first byte buffered last time */
    int lastsg;			/* last single-shifted plane (ms->sg) */
    /*
     * Buffer for internalized/converted data
     */
    unsigned char multiint[10];	/* Byte data */
    CHARSET multics[10];	/* Character set data (no UJIS/SJIS/UTF */
				/* because all of them are converted into */
				/* internal data format) */
    int intindex;		/* Index of multiint */
};

#define INBUF(mp)	((mp)->inbuf[(mp)->lastpos%sizeof((mp)->inbuf)])
#define INBUF0(mp)	((mp)->inbuf[(mp)->startpos%sizeof((mp)->inbuf)])
#define INBUF1(mp)	((mp)->inbuf[((mp)->startpos+1)%sizeof((mp)->inbuf)])
#define INBUF2(mp)	((mp)->inbuf[((mp)->startpos+2)%sizeof((mp)->inbuf)])
#define INBUFI(mp,i)	((mp)->inbuf[(i)%sizeof((mp)->inbuf)])

static int code_length(mp, cs)
MULBUF* mp;
CHARSET cs;
{
#if JAPANESE
    unsigned char c;
#endif

    if (CSISWRONG(cs))
	return 1;

#if JAPANESE
    switch (CS2CHARSET(cs)) {
    case UJIS:
	c = INBUF0(mp);
	if (ISUJISKANJI1(c)) return 2;
	if (ISUJISKANA1(c)) return 2;
	if (ISUJISKANJISUP1(c)) return 3;
	return 1;
    case SJIS:
	c = INBUF0(mp);
	if (ISSJISKANJI1(c)) return 2;
	if (ISSJISKANA(c)) return 1;
	return 1;
    }
#endif

    switch (CS2TYPE(cs))
    {
    case TYPE_94_CHARSET:
    case TYPE_96_CHARSET:
	return 1;
    case TYPE_94N_CHARSET:
    case TYPE_96N_CHARSET:
	switch (CS2FT(cs) & 0x70)
	{
	case 0x30: return 2;	/* for private use */
	case 0x40:
	case 0x50: return 2;
	case 0x60: return 3;
	case 0x70: return 4;	/* or more bytes */
	}
    }
    assert(0);
    return (0);
}

/*
 * Convert first byte of buffered data as one byte ASCII data
 * without any conversion.
 */
static void noconv1(mp)
MULBUF *mp;
{
    mp->multiint[mp->intindex] = INBUF0(mp);
    mp->multics[mp->intindex] = ASCII;
    mp->intindex++;
    mp->startpos++;
}

/*
 * Convert first byte of buffered data as one byte WRONGCS data
 * without any conversion.
 */
static void wrongcs1(mp)
MULBUF *mp;
{
    mp->multiint[mp->intindex] = INBUF0(mp);
    mp->multics[mp->intindex] = WRONGCS;
    mp->intindex++;
    mp->startpos++;
}

/*
 * Write a wrongmark on out buffer.
 */
static void put_wrongmark(mp)
MULBUF *mp;
{
    mp->multiint[mp->intindex + 0] = '"';
    mp->multiint[mp->intindex + 1] = '.';
    mp->multics[mp->intindex + 0] = JISX0208KANJI;
    mp->multics[mp->intindex + 1] = REST_MASK | JISX0208KANJI;
    mp->intindex += 2;
}

/*
 * Convert first several bytes of buffered data.
 *
 *  If less is in marking mode, it erase several bytes of data (depend on
 * the current character set) and write "?" mark on output buffer.
 *  If less is not in marking mode, it calls wrongcs1().
 */
static void wrongchar(mp)
MULBUF *mp;
{
    if (markwrongchar) {
	switch (CS2CHARSET(mp->multics[mp->intindex])) {
	case JISX0201KANA:
	case JISX0201ROMAN:
	case LATIN1:
	case LATIN2:
	case LATIN3:
	case LATIN4:
	case GREEK:
	case ARABIC:
	case HEBREW:
	case CYRILLIC:
	case LATIN5:
	    /* Should I use one byte character, like '?' or '_'? */
	    put_wrongmark(mp);
	    break;
	case JISX0208_78KANJI:
	case JISX0208KANJI:
	case JISX0208_90KANJI:
	case JISX0212KANJISUP:
	case JISX0213KANJI1:
	case JISX0213KANJI2:
	case UJIS:
	case SJIS:
	    put_wrongmark(mp);
	    break;
	case GB2312:
	case KSC5601:
	default:
	    break;
	}
    } else {
	int i;

	i = code_length(mp, mp->multics[mp->intindex]);
	while (--i >= 0) {
	    wrongcs1(mp);
	}
    }
}

/*
 * Internalize input stream.
 * We recognized input data as using ISO coding set.
 */
static void internalize_iso(mp)
MULBUF *mp;
{
    register int i;
    m_position pos;
    m_position to;
    int intindex;

    /*
     * If character set points empty character set, reject buffered data.
     */
    if (CSISWRONG(mp->cs)) {
	wrongcs1(mp);
	return;
    }

    /*
     * If character set points 94 or 94x94 character set, reject
     * DEL and SPACE codes in buffered data.
     */
    if (CS2TYPE(mp->cs) == TYPE_94_CHARSET ||
	CS2TYPE(mp->cs) == TYPE_94N_CHARSET) {
	unsigned char c = INBUF(mp);
	if ((c & 0x7f) == 0x7f) {
	    if (mp->lastpos - mp->startpos + 1 == 1) {
		wrongcs1(mp);
	    } else {
		wrongcs1(mp);
		multi_reparse(mp);
	    }
	    return;
	} else if ((c & 0x7f) == 0x20) {
	    /*
	     * A 0x20 (SPACE) code is wrong, but I treat it as
	     * a SPACE.
	     */
	    if (mp->lastpos - mp->startpos + 1 == 1) {
		noconv1(mp);
	    } else {
		wrongcs1(mp);
		multi_reparse(mp);
	    }
	    return;
	}
    }

    /*
     * Otherwise, keep buffering.
     */
    pos = mp->startpos;
    to = pos + code_length(mp, mp->cs) - 1;
    if (mp->lastpos < to) {
	return;		/* Not enough, so go back to fetch next data. */
    }

    /*
     * We buffered enough data for one character of multi byte characters.
     * Therefore, start to convert this buffered data into a first character.
     */
    intindex = mp->intindex;
    mp->multiint[intindex] = INBUFI(mp, pos) & 0x7f;
    mp->multics[intindex] = mp->cs;
    intindex++;
    for (pos++; pos <= to; pos++) {
	mp->multiint[intindex] = INBUFI(mp, pos) & 0x7f;
	mp->multics[intindex] = REST_MASK | mp->cs;
	intindex++;
    }
    /*
     * Check newly converted code.  If it is not valid code,
     * less may mark it as not valid code.
     */
    if (chisvalid_cs(&mp->multiint[mp->intindex], &mp->multics[mp->intindex])) {
	mp->intindex = intindex;
	mp->startpos = pos;
    } else {
	    /*
	     * less ignore the undefined codes
	     */
	wrongchar(mp);
	mp->startpos = pos;
	multi_reparse(mp);
    }
}

#if JAPANESE
/*
 * Internalize input stream.
 * We recognized input data as using UJIS coding set.
 */
static void internalize_ujis(mp)
MULBUF *mp;
{
    if (mp->lastpos - mp->startpos + 1 == 1) {
	/* do nothing */
    } else if (mp->lastpos - mp->startpos + 1 == 2) {
	if (ISUJISKANA(INBUF0(mp), INBUF1(mp))) {
	    mp->multiint[mp->intindex] = INBUF1(mp) & 0x7f;
	    mp->multics[mp->intindex] = mp->cs;
	    mp->intindex += 1;
	    mp->startpos = mp->lastpos + 1;
	} else if (ISUJISKANJI(INBUF0(mp), INBUF1(mp))) {
	    mp->multiint[mp->intindex] = INBUF0(mp);
	    mp->multics[mp->intindex] = UJIS;
	    mp->multiint[mp->intindex + 1] = INBUF1(mp);
	    mp->multics[mp->intindex + 1] = REST_MASK | UJIS;

	    /*
	     * Eliminate some wrong codes
	     */
	    if (chisvalid_cs(&mp->multiint[mp->intindex],
			     &mp->multics[mp->intindex])) {
		/* JIS X 0208:1997 */
		mp->multiint[mp->intindex] &= 0x7f;
		mp->multics[mp->intindex] = mp->cs;
		mp->multiint[mp->intindex + 1] &= 0x7f;
		mp->multics[mp->intindex + 1] = REST_MASK | mp->cs;
		mp->intindex += 2;
		mp->startpos = mp->lastpos + 1;
	    } else {
		/*
		 * less ignore the undefined codes
		 */
		wrongchar(mp);
		mp->startpos = mp->lastpos + 1;
		multi_reparse(mp);
	    }
	}
    } else if (mp->lastpos - mp->startpos + 1 == 3 &&
	       ISUJISKANJISUP(INBUF0(mp), INBUF1(mp), INBUF2(mp))) {
	mp->multiint[mp->intindex] = INBUF0(mp);
	mp->multics[mp->intindex] = UJIS;
	mp->multiint[mp->intindex + 1] = INBUF1(mp);
	mp->multics[mp->intindex + 1] = REST_MASK | UJIS;
	mp->multiint[mp->intindex + 2] = INBUF2(mp);
	mp->multics[mp->intindex + 2] = REST_MASK | UJIS;

	/*
	 * Eliminate some wrong codes
	 */
	if (chisvalid_cs(&mp->multiint[mp->intindex],
			 &mp->multics[mp->intindex])) {
	    register int c1;
	    static unsigned char table[] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#if UJIS0213
		   0, 0x21,    0, 0x23, 0x24, 0x25,    0,    0,
		0x28,    0,    0,    0, 0x2C, 0x2D, 0x2E, 0x2F,
#else
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#if UJIS0213
		   0,    0,    0,    0,    0,    0,    0,    0,
		   0,    0,    0,    0,    0,    0, 0x6E, 0x6F,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
		0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E,    0
#else
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
#endif
	    };
	    c1 = mp->multiint[mp->intindex + 1] & 0x7f;
	    if (table[c1] != 0) {
		/* JIS X 0213:2000 plane 2 */
		if (output == jis) {
		    /* JIS cannot output JIS X 0213:2000 plane 2 */
		    wrongcs1(mp);
		    multi_reparse(mp);
		} else {
		    mp->multiint[mp->intindex] = c1;
		    mp->multics[mp->intindex] =
			    JISX0213KANJI2;
		    mp->multiint[mp->intindex + 1] =
			    mp->multiint[mp->intindex + 2] & 0x7f;
		    mp->multics[mp->intindex + 1] =
			    REST_MASK | JISX0213KANJI2;
		    mp->intindex += 2;
		    mp->startpos = mp->lastpos + 1;
		}
	    } else {
		/* JIS X 0212:1990 */
		if (output == sjis || output == jis) {
		    /* SJIS cannot output JIS X 0212:1990 */
		    wrongcs1(mp);
		    multi_reparse(mp);
		} else {
		    mp->multiint[mp->intindex] = c1;
		    mp->multics[mp->intindex] = mp->cs;
		    mp->multiint[mp->intindex + 1] =
			    mp->multiint[mp->intindex + 2] & 0x7f;
		    mp->multics[mp->intindex + 1] =
			    REST_MASK | mp->cs;
		    mp->intindex += 2;
		    mp->startpos = mp->lastpos + 1;
		}
	    }
	} else {
	    wrongchar(mp);
	    mp->startpos = mp->lastpos + 1;
	    multi_reparse(mp);
	}
    } else {
	wrongcs1(mp);
	multi_reparse(mp);
    }
}

/*
 * Check and normalize all SJIS codes
 */
static void internalize_sjis(mp)
MULBUF *mp;
{
    if (mp->lastpos - mp->startpos + 1 == 1) {
	if (!ISSJISKANA(INBUF(mp))) {
	    wrongcs1(mp);
	} else {
	    mp->multiint[mp->intindex] = INBUF(mp) & 0x7f;
	    mp->multics[mp->intindex] = mp->cs;
	    mp->intindex += 1;
	    mp->startpos = mp->lastpos + 1;
	}
    } else if (mp->lastpos - mp->startpos + 1 == 2 &&
	       ISSJISKANJI(INBUF0(mp), INBUF1(mp))) {
	mp->multiint[mp->intindex] = INBUF0(mp);
	mp->multics[mp->intindex] = SJIS;
	mp->multiint[mp->intindex + 1] = INBUF1(mp);
	mp->multics[mp->intindex + 1] = REST_MASK | SJIS;

	/*
	 * Check the correctness of SJIS encoded characters and
	 * convert them into internal representation.
	 */
	if (chisvalid_cs(&mp->multiint[mp->intindex],
			 &mp->multics[mp->intindex])) {
	    register int c1, c2, c3;
	    static unsigned char table[] = {
		   0, 0x21, 0x23, 0x25, 0x27, 0x29, 0x2B, 0x2D,
		0x2F, 0x31, 0x33, 0x35, 0x37, 0x39, 0x3B, 0x3D,
		0x3F, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4B, 0x4D,
		0x4F, 0x51, 0x53, 0x55, 0x57, 0x59, 0x5B, 0x5D,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		0x5F, 0x61, 0x63, 0x65, 0x67, 0x69, 0x6B, 0x6D,
#if SJIS0213
		0x6F, 0x71, 0x73, 0x75, 0x77, 0x79, 0x7B, 0x7D,
		0x80, 0xA3, 0x81, 0xAD, 0x82, 0xEF, 0xF1, 0xF3,
		0xF5, 0xF7, 0xF9, 0xFB, 0xFD,    0,    0,    0
#else
		0x6F, 0x71, 0x73,    0,    0,    0,    0,    0,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
#endif
	    };

	    c1 = table[INBUF0(mp) & 0x7f];
	    c2 = INBUF(mp) - ((unsigned char)INBUF(mp) >= 0x80 ? 1 : 0);
	    c3 = c2 >= 0x9e;
	    if (c1 < 0x80) {
		/* JIS X 0213:2000 plane 1 or JIS X 0208:1997 */
		mp->multiint[mp->intindex] =
			(c1 + (c3 ? 1 : 0));
		mp->multics[mp->intindex] = mp->cs;
		mp->multiint[mp->intindex + 1] =
			(c2 - (c3 ? 0x9e - 0x21 : 0x40 - 0x21));
		mp->multics[mp->intindex + 1] =
			REST_MASK | mp->cs;
		mp->intindex += 2;
		mp->startpos = mp->lastpos + 1;
	    } else {
		/* JIS X 0213:2000 plane 2 */
		if (output == jis) {
		    /* JIS cannot output JIS X 0213:2000 plane 2 */
		    wrongcs1(mp);
		    multi_reparse(mp);
		} else {
		    if (c1 > 0xA0) {
			/* row 3-4, 13-14, and 79-94 */
			mp->multiint[mp->intindex] =
				((c1 & 0x7f) + (c3 ? 1 : 0));
		    } else if (c1 == 0x80) {
			/* row 1 or 8 */
			mp->multiint[mp->intindex] =
				c3 ? 0x28 : 0x21;
		    } else if (c1 == 0x81) {
			/* row 5 or 12 */
			mp->multiint[mp->intindex] =
				c3 ? 0x2C : 0x25;
		    } else {
			/* row 15 or 78 */
			mp->multiint[mp->intindex] =
				c3 ? 0x6E : 0x2F;
		    }
		    mp->multics[mp->intindex] = JISX0213KANJI2;
		    mp->multiint[mp->intindex + 1] =
			    (c2 - (c3 ? 0x9e - 0x21 : 0x40 - 0x21));
		    mp->multics[mp->intindex + 1] =
			    REST_MASK | JISX0213KANJI2;
		    mp->intindex += 2;
		    mp->startpos = mp->lastpos + 1;
		}
	    }
	} else {
	    /*
	     * Less ignores undefined characters after marking
	     * them as wrong characters.
	     */
	    wrongchar(mp);
	    mp->startpos = mp->lastpos + 1;
	    multi_reparse(mp);
	}
    } else {
	wrongcs1(mp);
	multi_reparse(mp);
    }
}
#endif

static void internalize(mp)
MULBUF *mp;
{
    int c = INBUF(mp);

    if (mp->lastpos - mp->startpos + 1 == 1) {
	if ((c <= 0x7f && mp->io.left == noconv) ||
	    (c >= 0x80 && mp->io.right == noconv)) {
#if JAPANESE
	    mp->sequence_counter = 0;
#endif
	    if (control_char(c)) {
		    wrongcs1(mp);
	    } else {
		    noconv1(mp);
	    }
	    return;
	} else if (c >= 0x80 && mp->io.right == none) {
#if JAPANESE
	    mp->sequence_counter = 0;
#endif
	    wrongcs1(mp);
	    return;
	}

	mp->cs = ASCII;
	if (c < 0x20) {
#if JAPANESE
	    mp->sequence_counter = 0;
#endif
	    wrongcs1(mp);
	    return;
	} else if (c <= 0x7f ||
		   (mp->io.right == iso8 && (0xa0 <= c && c <= 0xff))) {
#if JAPANESE
	    mp->sequence_counter = 0;
#endif
	    /*
	     * Decide current character set.
	     */
	    mp->cs = FINDCS(mp, c);
	    /*
	     * Check cs that fit for output code set.
	     */
	    /* JIS cannot output JISX0212, JISX0213_2, or ISO2022 */
	    if (output == jis && mp->cs != ASCII &&
		mp->cs != JISX0201KANA &&
		mp->cs != JISX0201ROMAN &&
		mp->cs != JISX0208_78KANJI &&
		mp->cs != JISX0208KANJI &&
		mp->cs != JISX0208_90KANJI &&
		mp->cs != JISX0213KANJI1) {
		wrongcs1(mp);
		multi_reparse(mp);
		return;
	    }
	    /* UJIS cannot output regular ISO2022 except JIS */
	    if (output == ujis && mp->cs != ASCII &&
		mp->cs != JISX0201KANA &&
		mp->cs != JISX0201ROMAN &&
		mp->cs != JISX0208_78KANJI &&
		mp->cs != JISX0208KANJI &&
		mp->cs != JISX0208_90KANJI &&
		mp->cs != JISX0212KANJISUP &&
		mp->cs != JISX0213KANJI1 &&
		mp->cs != JISX0213KANJI2) {
		wrongcs1(mp);
		multi_reparse(mp);
		return;
	    }
	    /* SJIS cannot output JISX0212 or ISO2022 */
	    if (output == sjis && mp->cs != ASCII &&
		mp->cs != JISX0201KANA &&
		mp->cs != JISX0201ROMAN &&
		mp->cs != JISX0208_78KANJI &&
		mp->cs != JISX0208KANJI &&
		mp->cs != JISX0208_90KANJI &&
		mp->cs != JISX0213KANJI1 &&
		mp->cs != JISX0213KANJI2) {
		wrongcs1(mp);
		multi_reparse(mp);
		return;
	    }

	    if (mp->cs != ASCII)
		mp->icharset = mp->cs;
	    internalize_iso(mp);
	    return;
	} else if (control_char(c)) {
#if JAPANESE
	    mp->sequence_counter = 0;
#endif
	    wrongcs1(mp);
	    return;
	}
#if JAPANESE
	if (mp->priority == sjis && ISSJISKANA(c)) {
	    if (mp->io.right == japanese) {
		mp->sequence_counter++;
		if (mp->sequence_counter % 2 == 1 &&
		    INBUF0(mp) != 0xa4) /* ???? */
		{
		    mp->sequence_counter = 0;
		}
		if (mp->sequence_counter >= 6)
		    /*
		     * It looks like a sequence of UJIS
		     * hiragana.  Thus we give priority
		     * to not sjis.
		     */
		    mp->priority = ujis;
	    }
	    mp->cs = JISX0201KANA;
	    mp->icharset = SJIS;
	    internalize_sjis(mp);
	    return;
	} else if (mp->io.right == ujis || mp->io.right == sjis ||
		   mp->io.right == japanese) {
	    mp->sequence_counter = 0;
	    return;
	}
	mp->sequence_counter = 0;
#endif
	wrongcs1(mp);
	return;
    }

#if JAPANESE
    assert(mp->sequence_counter == 0);
#endif
    if (c < 0x20) {
	wrongcs1(mp);
	multi_reparse(mp);
	return;
    } else if (mp->cs != ASCII &&
	       (c <= 0x7f ||
		(mp->io.right == iso8 && 0xa0 <= c && c <= 0xff))) {
	if (mp->cs != FINDCS(mp, c)) {
	    wrongcs1(mp);
	    multi_reparse(mp);
	} else {
	    internalize_iso(mp);
	}
	return;
    } else if (control_char(c)) {
	wrongcs1(mp);
	multi_reparse(mp);
	return;
    }
#if JAPANESE
    if (mp->lastpos - mp->startpos + 1 == 2) {
	int c0 = INBUF0(mp);
	if (mp->priority == sjis && ISSJISKANJI(c0, c)) {
#if UJIS0213
	    mp->cs = JISX0213KANJI1;
#else
	    mp->cs = JISX0208KANJI;
#endif
	    mp->icharset = SJIS;
	    internalize_sjis(mp);
	    return;
	} else if (mp->priority == ujis) {
	    if (ISUJISKANA(c0, c)) {
		mp->cs = JISX0201KANA;
		mp->icharset = UJIS;
		internalize_ujis(mp);
		return;
	    } else if (ISUJISKANJI(c0, c)) {
#if UJIS0213
		mp->cs = JISX0213KANJI1;
#else
		mp->cs = JISX0208KANJI;
#endif
		mp->icharset = UJIS;
		internalize_ujis(mp);
		return;
	    } else if (ISUJISKANJISUP(c0, c, 0xa1)) {
		return;
	    }
	}

	if ((mp->io.right == sjis || mp->io.right == japanese) &&
	    ISSJISKANJI(c0, c)) {
#if UJIS0213
	    mp->cs = JISX0213KANJI1;
#else
	    mp->cs = JISX0208KANJI;
#endif
	    mp->priority = sjis;
	    mp->icharset = SJIS;
	    internalize_sjis(mp);
	    return;
	} else if ((mp->io.right == ujis || mp->io.right == japanese)) {
	    if (ISUJISKANA(c0, c)) {
		mp->cs = JISX0201KANA;
		mp->priority = ujis;
		mp->icharset = UJIS;
		internalize_ujis(mp);
		return;
	    } else if (ISUJISKANJI(c0, c)) {
#if UJIS0213
		    mp->cs = JISX0213KANJI1;
#else
		    mp->cs = JISX0208KANJI;
#endif
		    mp->priority = ujis;
		    mp->icharset = UJIS;
		    internalize_ujis(mp);
		    return;
	    } else if (ISUJISKANJISUP(c0, c, 0xa1))
	    {
		    return;
	    }
	}
    } else if (mp->lastpos - mp->startpos + 1 == 3 &&
	       (mp->priority == ujis ||
		mp->io.right == ujis || mp->io.right == japanese) &&
	       ISUJISKANJISUP(INBUF0(mp), INBUF1(mp), c)) {
	    mp->cs = JISX0212KANJISUP;
	    mp->priority = ujis;
	    mp->icharset = UJIS;
	    internalize_ujis(mp);
	    return;
    }
#endif
    wrongcs1(mp);
    multi_reparse(mp);
}

/*
 * Check routines
 */
static int check_ft(mp, c, type, plane)
MULBUF *mp;
register int c;
int type;
int *plane;
{
    if (mp->io.left == jis) {
	/*
	 * If the target code system is traditional jis,
	 * allow only JIS C6226-1978, JIS X0208-1983, JIS X0208-1990,
	 * JIS X0213-2000, JIS X0212-1990, ASCII,
	 * JIS X0201 right, and JIS X0201 left.
	 */
	if ((type == TYPE_94N_CHARSET &&
	     (c == '@' || c == 'B' || c == 'D' ||
	      c == 'O' || c == 'P')) ||
	    (type == TYPE_94_CHARSET &&
	     (c == 'B' || c == 'I' || c == 'J'))) {
	    *plane = (mp->ms->irr ? IRR2CS(mp->ms->irr) : 0) | TYPE2CS(type) | FT2CS(c);
	    mp->ms->irr = 0;
	    mp->eseq = NOESC;
	    return (0);
	}
    } else if (0x30 <= c && c <= 0x7e) {
	/*
	 * Otherwise, accept all.
	 */
	*plane = (mp->ms->irr ? IRR2CS(mp->ms->irr) : 0) | TYPE2CS(type) | FT2CS(c);
	mp->ms->irr = 0;
	mp->eseq = NOESC;
	return (0);
    }
    return (-1);
}

static int check_irr(mp, c)
MULBUF *mp;
register int c;
{
    if (0x40 <= c && c <= 0x7e) {
	mp->ms->irr = CODE2IRR(c);
	mp->eseq = NOESC;
	return (0);
    }
    return (-1);
}

static void fix_status_for_escape_sequence(mp)
MULBUF *mp;
{
    if (mp->eseq == NOESC) {
	switch (CS2TYPE(ISVALIDPLANE(mp, sg) ? PLANE2CS(mp, sg) :
					       PLANE2CS(mp, gl))) {
	case TYPE_96_CHARSET:
	case TYPE_96N_CHARSET:
	    change_control_char(0177, 0);
	    break;
	case TYPE_94_CHARSET:
	case TYPE_94N_CHARSET:
	    change_control_char(0177, 1);
	    break;
	}
	switch (CS2TYPE(ISVALIDPLANE(mp, sg) ? PLANE2CS(mp, sg) :
					       PLANE2CS(mp, gr))) {
	case TYPE_96_CHARSET:
	case TYPE_96N_CHARSET:
	    change_control_char(0377, 0);
	    break;
	case TYPE_94_CHARSET:
	case TYPE_94N_CHARSET:
	    change_control_char(0377, 1);
	    break;
	}
    }
}

static int check_escape_sequence(mp)
MULBUF *mp;
{
    int c = INBUF(mp);

    switch (mp->eseq) {
    case ESC_:
	switch (c) {
	case '$': mp->eseq = ESC_2_4; break;
	case '&': mp->eseq = ESC_2_6; break;
	case '(': mp->eseq = ESC_2_8; break;
	case ')': mp->eseq = ESC_2_9; break;
	case '*': mp->eseq = ESC_2_10; break;
	case '+': mp->eseq = ESC_2_11; break;
	case ',': mp->eseq = ESC_2_12; break;
	case '-': mp->eseq = ESC_2_13; break;
	case '.': mp->eseq = ESC_2_14; break;
	case '/': mp->eseq = ESC_2_15; break;
	case 'N': mp->ms->sg = 2; mp->eseq = NOESC; /*SS2*/break;
	case 'O': mp->ms->sg = 3; mp->eseq = NOESC; /*SS3*/break;
	case 'n': mp->ms->gl = 2; mp->eseq = NOESC; break;
	case 'o': mp->ms->gl = 3; mp->eseq = NOESC; break;
	case '|': if (mp->io.right != iso8) goto wrong;
		  mp->ms->gr = 3; mp->eseq = NOESC; break;
	case '}': if (mp->io.right != iso8) goto wrong;
		  mp->ms->gr = 2; mp->eseq = NOESC; break;
	case '~': if (mp->io.right != iso8) goto wrong;
		  mp->ms->gr = 1; mp->eseq = NOESC; break;
	default:  goto wrong;
	}
	break;
    case ESC_2_4:
	switch (c) {
	case '(': mp->eseq = ESC_2_4_8; break;
	case ')': mp->eseq = ESC_2_4_9; break;
	case '*': mp->eseq = ESC_2_4_10; break;
	case '+': mp->eseq = ESC_2_4_11; break;
	case '-': mp->eseq = ESC_2_4_13; break;
	case '.': mp->eseq = ESC_2_4_14; break;
	case '/': mp->eseq = ESC_2_4_15; break;
	case '@':
	case 'A':
	case 'B': if (check_ft(mp, c, TYPE_94N_CHARSET, &(mp->ms->gs[0])) == 0)
			break;
	default:  goto wrong;
	}
	break;
    case ESC_2_6:
	if (check_irr(mp, c) == 0) break;
	goto wrong;
    case ESC_2_8:
	if (check_ft(mp, c, TYPE_94_CHARSET, &(mp->ms->gs[0])) == 0) break;
	goto wrong;
    case ESC_2_9:
	if (check_ft(mp, c, TYPE_94_CHARSET, &(mp->ms->gs[1])) == 0) break;
	goto wrong;
    case ESC_2_10:
	if (check_ft(mp, c, TYPE_94_CHARSET, &(mp->ms->gs[2])) == 0) break;
	goto wrong;
    case ESC_2_11:
	if (check_ft(mp, c, TYPE_94_CHARSET, &(mp->ms->gs[3])) == 0) break;
	goto wrong;
    case ESC_2_12:
	if (check_ft(mp, c, TYPE_96_CHARSET, &(mp->ms->gs[0])) == 0) break;
	goto wrong;
    case ESC_2_13:
	if (check_ft(mp, c, TYPE_96_CHARSET, &(mp->ms->gs[1])) == 0) break;
	goto wrong;
    case ESC_2_14:
	if (check_ft(mp, c, TYPE_96_CHARSET, &(mp->ms->gs[2])) == 0) break;
	goto wrong;
    case ESC_2_15:
	if (check_ft(mp, c, TYPE_96_CHARSET, &(mp->ms->gs[3])) == 0) break;
	goto wrong;
    case ESC_2_4_8:
	if (check_ft(mp, c, TYPE_94N_CHARSET, &(mp->ms->gs[0])) == 0) break;
	goto wrong;
    case ESC_2_4_9:
	if (check_ft(mp, c, TYPE_94N_CHARSET, &(mp->ms->gs[1])) == 0) break;
	goto wrong;
    case ESC_2_4_10:
	if (check_ft(mp, c, TYPE_94N_CHARSET, &(mp->ms->gs[2])) == 0) break;
	goto wrong;
    case ESC_2_4_11:
	if (check_ft(mp, c, TYPE_94N_CHARSET, &(mp->ms->gs[3])) == 0) break;
	goto wrong;
    case ESC_2_4_13:
	if (check_ft(mp, c, TYPE_96N_CHARSET, &(mp->ms->gs[1])) == 0) break;
	goto wrong;
    case ESC_2_4_14:
	if (check_ft(mp, c, TYPE_96N_CHARSET, &(mp->ms->gs[2])) == 0) break;
	goto wrong;
    case ESC_2_4_15:
	if (check_ft(mp, c, TYPE_96N_CHARSET, &(mp->ms->gs[3])) == 0) break;
	goto wrong;
    case NOESC:
	/*
	 * This sequence is wrong if we buffered some data.
	 */
	if (mp->lastpos != mp->startpos) {
	    switch (c) {
	    case 0033:
	    case 0016:
	    case 0017:
	    case 0031: goto wrong;
	    case 0216:
	    case 0217: if (mp->io.right == iso8) goto wrong;
	    default:   goto wrongone;
	    }
	}
	/*
	 * Nothing is buffered.  So, check this sequence.
	 */
	switch (c) {
	case 0033: mp->eseq = ESC_; break;
	case 0016: mp->ms->gl = 1; mp->eseq = NOESC; break;
	case 0017: mp->ms->gl = 0; mp->eseq = NOESC; break;
	case 0031: mp->ms->sg = 2; mp->eseq = NOESC; /*SS2*/ break;
	case 0216: if (mp->io.right != iso8) goto wrongone;
		   mp->ms->sg = 2; mp->eseq = NOESC; /*SS2*/ break;
	case 0217: if (mp->io.right != iso8) goto wrongone;
		   mp->ms->sg = 3; mp->eseq = NOESC; /*SS3*/ break;
	default:   goto wrongone;
	}
	break;
    default:
	assert(0);
    }
    if (mp->eseq == NOESC) {
	fix_status_for_escape_sequence(mp);
	mp->startpos = mp->lastpos + 1;
	return (0);
    }
    return (0);
wrong:
    if (mp->eseq != NOESC) {
	mp->eseq = NOESC;
	fix_status_for_escape_sequence(mp);
    }
    wrongcs1(mp);
    multi_reparse(mp);
    return (0);
wrongone:
    assert(mp->eseq == NOESC);
    return (-1);
}

struct planeset {
    char *name;
    char *planeset;
} planesets[] = {
    { "ascii",		""	},
    { "ctext",		"\\e-A"	},
    { "latin1",		"\\e-A"	},
    { "latin2",		"\\e-B"	},
    { "latin3",		"\\e-C"	},
    { "latin4",		"\\e-D"	},
    { "greek",		"\\e-F"	},
    { "arabic",		"\\e-G"	},
    { "hebrew",		"\\e-H"	},
    { "cyrillic",	"\\e-L"	},
    { "latin5",		"\\e-M"	},
    { "japanese",	"\\e$)B\\e*I\\e$+D" },
    { "ujis",		"\\e$)B\\e*I\\e$+D" },
    { "euc",		"\\e$)B\\e*I\\e$+D" },
    { NULL,		"" }
};

int set_planeset(name)
register char *name;
{
    register struct planeset *p;
    MULBUF *mp;
    int ret;
    int i;

    if (name == NULL) {
	return -1;
    }
    for (p = planesets; p->name != NULL; p++) {
	if (strcmp(name, p->name) == 0) {
	    name = p->planeset;
	    break;
	}
    }
    mp = new_multibuf();
    init_priority(mp);
    while (*name) {
	if (*name == '\\' &&
	    (*(name + 1) == 'e' || *(name + 1) == 'E')) {
	    ++mp->lastpos;
	    INBUF(mp) = '\033';
	    ret = check_escape_sequence(mp);
	    name += 2;
	} else {
	    ++mp->lastpos;
	    INBUF(mp) = *name++;
	    ret = check_escape_sequence(mp);
	}
	if (ret < 0 || mp->intindex > 0) {
	    free(mp);
	    return -1;
	}
    }
    def_gs[0] = mp->ms->gs[0];
    def_gs[1] = mp->ms->gs[1];
    def_gs[2] = mp->ms->gs[2];
    def_gs[3] = mp->ms->gs[3];
    free(mp);
    return 0;
}

void init_def_codesets(left, right, out)
CODESET left;
CODESET right;
CODESET out;
{
    def_left = left;
    def_right = right;
    output = out;
}

void init_def_priority(pri)
CODESET pri;
{
#if JAPANESE
    assert(pri == sjis || pri == ujis);
    def_priority = pri;
#endif
}

void init_priority(mp)
MULBUF *mp;
{
#if JAPANESE
    if (mp->io.right == sjis)
	mp->priority = sjis;
    else if (mp->io.right == ujis)
	mp->priority = ujis;
    else if (mp->io.right == japanese)
	mp->priority = def_priority;
    else
	mp->priority = noconv;
    mp->sequence_counter = 0;
#endif
}

CODESET get_priority(mp)
MULBUF *mp;
{
#if JAPANESE
    return (mp->priority);
#else
    return (noconv);
#endif
}

void set_priority(mp, pri)
MULBUF *mp;
CODESET pri;
{
#if JAPANESE
    assert(pri == sjis || pri == ujis || pri == noconv);
    mp->priority = pri;
#endif
}

MULBUF *new_multibuf()
{
    MULBUF *mp = (MULBUF*) ecalloc(1, sizeof(MULBUF));
    mp->io.left = def_left;
    mp->io.right = def_right;
    mp->orig_io_right = def_right;
    mp->rotation_io_right = 0;
    mp->eseq = NOESC;
    mp->ms = (struct m_status*) ecalloc(1, sizeof(struct m_status));
    init_multibuf(mp);
    return (mp);
}

void clear_multibuf(mp)
MULBUF *mp;
{
    mp->lastpos = M_NULL_POS;
    mp->startpos = 0;
    mp->laststartpos = 0;
    mp->lastsg = WRONGPLANE;
    mp->intindex = 0;
}

static void init_ms(ms)
struct m_status *ms;
{
    ms->gs[0] = def_gs[0];
    ms->gs[1] = def_gs[1];
    ms->gs[2] = def_gs[2];
    ms->gs[3] = def_gs[3];
    ms->gl = 0;
    ms->gr = 1;
    ms->sg = WRONGPLANE;
    ms->irr = 0;
}

void init_multibuf(mp)
MULBUF *mp;
{
    mp->cs = ASCII;
    init_ms(mp->ms);
    if (mp->eseq != NOESC) {
	mp->eseq = NOESC;
    }
    fix_status_for_escape_sequence(mp);
#if JAPANESE
    mp->sequence_counter = 0;
#endif
    mp->icharset = ASCII;
    clear_multibuf(mp);
}

/*
 * Buffering characters untile get a guarantee that it is right sequence.
 */
static void check_new_buffered_byte(mp)
MULBUF *mp;
{
    m_position last_startpos = mp->startpos;

    if (mp->io.left == jis || mp->io.left == iso7 || mp->io.right == iso8) {
	if (check_escape_sequence(mp) == 0) {
	    return;		/* going process well */
	}
    }

    /* it is not a escape sequence, try to use it as character */
    internalize(mp);

    /*
     * If a character was detected in internalize(),
     * clean sg since single shift affect only one character.
     */
    if (last_startpos != mp->startpos) {
	mp->lastsg = mp->ms->sg;
	if (mp->ms->sg != WRONGPLANE) {
	    mp->ms->sg = WRONGPLANE;
	    fix_status_for_escape_sequence(mp);
	}
    }
}

/*
 * Re-parse all buffered data.
 *
 * This routine is called when we find a problem in buffered data.
 * We firstly take out the first byte of buffered data before we call
 * this function.  This routine parse all rest of buffered data again.
 */
static void multi_reparse(mp)
MULBUF *mp;
{
    m_position to;

    /*
     * We found something wrong and going to move first byte.
     * So, we clear single-shifted character set because it will
     * shift only this one byte being makred wrong.
     */
    if (mp->ms->sg != WRONGPLANE) {
	mp->ms->sg = WRONGPLANE;
	fix_status_for_escape_sequence(mp);
    }

#if JAPANESE
    /*
     * Quick japanese code hack.
     * Check whether character is SJIS KANA or no.
     * If it is SJIS KANA, it means our prediction was failed.
     * Now going to fall back to SJIS KANA mode.
     */
    if ((mp->priority == sjis ||
	 mp->io.right == sjis || mp->io.right == japanese) &&
	ISSJISKANA(mp->multiint[mp->intindex - 1])) {
	mp->cs = JISX0201KANA;
	mp->priority = sjis;
	mp->icharset = SJIS;
	mp->multiint[mp->intindex - 1] &= 0x7f;
	mp->multics[mp->intindex - 1] = mp->cs;
    }
#endif

    /*
     * Retry to parse rest of buffered data.
     */
    to = mp->lastpos;
    for (mp->lastpos = mp->startpos; mp->lastpos <= to; mp->lastpos++) {
	check_new_buffered_byte(mp);
    }
    mp->lastpos = to;
}

#if LESS
void multi_find_cs(mp, pos)
MULBUF* mp;
m_position pos;
{
    int c;
    m_position lpos = pos;

    if (ch_seek(pos) == 0) {
	/*
	 * Back up to the beginning of the line.
	 */
	while ((c = ch_back_get()) != '\n' && c != EOI) ;
	if (c == '\n') {
	    (void)ch_forw_get();
	}

	lpos = ch_tell();

	if (lpos != pos) {
	    while (lpos < pos) {
		c = ch_forw_get();
		assert(c != EOI && c != '\n');
		multi_parse(mp, c, NULL_POSITION, NULL);
		lpos++;
	    }
	    ch_seek(pos);
	}
    }
}
#endif

#define DEBUG 0
#if DEBUG
int debug = 1;
#endif

/*
 * Manage m_status data structure to maintain ISO-2022 status of input stream.
 */
void multi_start_buffering(mp, pos)
MULBUF *mp;
m_position pos;
{
    /* buffer must be empty */
    assert(mp->lastpos < mp->startpos);

    /* initialize m_status if it is necessary */
    if (pos == mp->lastpos + 2 || pos == mp->laststartpos) {
	/*
	 * pos == mp->lastpos+2 if this line is started after \n.
	 * pos == mp->laststartpos if this line is started by a non-fit
	 * character.
	 */
	/* restore backed up sg */
	if (mp->ms->sg != mp->lastsg) {
	    mp->ms->sg = mp->lastsg;
	    fix_status_for_escape_sequence(mp);
	}
	/* adjust pointers */
	mp->startpos = pos;
	mp->lastpos = pos - 1;
    } else {
	/*
	 * pos == somewhere else if this function is called after jump_loc().
	 */
#if DEBUG
	if (debug) {
	    fprintf(stderr, "%qd, %qd, %qd, %qd\n", pos, mp->lastpos,
		mp->startpos, mp->laststartpos);
	    fprintf(stderr, "oct %qo, %qo, %qo, %qo\n", pos, mp->lastpos,
		mp->startpos, mp->laststartpos);
	}
#endif
	init_multibuf(mp);
#if LESS
	multi_find_cs(mp, pos);
	clear_multibuf(mp);
#endif

	/* adjust pointers */
	mp->startpos = pos;
	mp->lastpos = pos - 1;
	mp->laststartpos = pos;
    }
}

/*
 * Buffering characters untile get a guarantee that it is right sequence.
 */
void multi_parse(mp, c, pos, mbd)
MULBUF* mp;
int c;
m_position pos;
M_BUFDATA* mbd;
{
    if (c < 0) {
	if (mbd != NULL) {
	    mbd->pos = mp->startpos;
	}
	/*
	 * Force to flush all buffering characters.
	 */
	if (mp->eseq != NOESC) {
	    mp->eseq = NOESC;
	    fix_status_for_escape_sequence(mp);
	}
	while (mp->startpos <= mp->lastpos) {
	    wrongcs1(mp);
	    multi_reparse(mp);
	}

	if (mbd != NULL) {
	    mbd->cbuf = mp->multiint;
	    mbd->csbuf = mp->multics;
	    mbd->byte = mp->intindex;
	}
	mp->intindex = 0;
    } else {
	if (pos != NULL_POSITION) {
	    assert(pos == mp->lastpos + 1);
	    mp->lastpos = pos;
	} else {
	    mp->lastpos++;
	}
	INBUF(mp) = c;

	mp->laststartpos = mp->startpos;
	if (mbd != NULL) {
	    mbd->pos = mp->startpos;
	}

	/*
	 * Put it into buffer and parse it.
	 */
	check_new_buffered_byte(mp);

	if (mbd != NULL) {
	    mbd->cbuf = mp->multiint;
	    mbd->csbuf = mp->multics;
	    mbd->byte = mp->intindex;
	}
	mp->intindex = 0;
    }
}

/*
 * Flush buffered data.
 */
void multi_flush(mp, mbd)
MULBUF* mp;
M_BUFDATA* mbd;
{
    multi_parse(mp, -1, NULL_POSITION, mbd);
}

/*
 * Discard buffered data.
 */
void multi_discard(mp)
MULBUF* mp;
{
    multi_parse(mp, -1, NULL_POSITION, NULL);
}

void set_codesets(mp, left, right)
MULBUF *mp;
CODESET left;
CODESET right;
{
    mp->io.left = left;
    mp->io.right = right;
}

/*
 * Return string representation about multi bytes character
 * which was buffered.
 */
char *get_icharset_string(mp)
MULBUF *mp;
{
	static char buf[10];

	switch (mp->icharset)
	{
#if JAPANESE
	/*
	 * Code set
	 */
	case SJIS:		return ("SJIS");
	case UJIS:		return ("UJIS");
#endif
	/*
	 * Character set
	 */
	case ASCII:		return ("ASCII");
	case JISX0201KANA:	return ("JIS-KANA");
	case JISX0201ROMAN:	return ("JIS-ROMAN");
	case LATIN1:		return ("LATIN1");
	case LATIN2:		return ("LATIN2");
	case LATIN3:		return ("LATIN3");
	case LATIN4:		return ("LATIN4");
	case GREEK:		return ("GREEK");
	case ARABIC:		return ("ARABIC");
	case HEBREW:		return ("HEBREW");
	case CYRILLIC:		return ("CYRILLIC");
	case LATIN5:		return ("LATIN5");
	case JISX0208_78KANJI:	return ("JIS-78KANJI");
	case GB2312:		return ("GB2312");
	case JISX0208KANJI:	return ("JIS-83KANJI");
	case JISX0208_90KANJI:	return ("JIS-90KANJI");
	case KSC5601:		return ("KSC5601");
	case JISX0212KANJISUP:	return ("JIS-KANJISUP");
	case JISX0213KANJI1:	return ("JISX0213KANJI1");
	case JISX0213KANJI2:	return ("JISX0213KANJI2");
	}
	switch (CS2TYPE(mp->icharset))
	{
	case TYPE_94_CHARSET:
		strcpy(buf, "94( )");
		buf[3] = CS2FT(mp->icharset);
		break;
	case TYPE_96_CHARSET:
		strcpy(buf, "96( )");
		buf[3] = CS2FT(mp->icharset);
		break;
	case TYPE_94N_CHARSET:
		strcpy(buf, "94N( )");
		buf[4] = CS2FT(mp->icharset);
		break;
	case TYPE_96N_CHARSET:
		strcpy(buf, "96N( )");
		buf[4] = CS2FT(mp->icharset);
		break;
	default:
		assert(0);
	}
	if (CS2IRR(mp->icharset) > 0)
	{
		char num[3];
		sprintf(num, "%d", CS2IRR(mp->icharset));
		strcat(buf, num);
	}
	return (buf);
}

static int old_output_charset = ASCII;	/* Last displayed character set */

static unsigned char *make_escape_sequence(charset)
int charset;
{
	static unsigned char p[9];
	int len;

	if (CSISWRONG(charset))
	{
		charset = ASCII;
	}

	p[0] = '\033';
	len = 1;
	if ((output == iso7 || output == iso8) && CS2IRR(charset) > 0)
	{
		p[len] = '&';
		p[len + 1] = IRR2CODE(CS2IRR(charset));
		p[len + 2] = '\033';
		len += 3;
	}
	switch (CS2TYPE(charset))
	{
	case TYPE_94_CHARSET:
		p[len] = '(';
		p[len + 1] = CS2FT(charset);
		len += 2;
		break;
	case TYPE_94N_CHARSET:
		switch (CS2FT(charset))
		{
		case '@':
		case 'A':
		case 'B':
			p[len] = '$';
			p[len + 1] = CS2FT(charset);
			len += 2;
			break;
		default:
			p[len] = '$';
			p[len + 1] = '(';
			p[len + 2] = CS2FT(charset);
			len += 3;
			break;
		}
		break;
	case TYPE_96_CHARSET:
		p[len] = '-';
		p[len + 1] = CS2FT(charset);
		len += 2;
		break;
	case TYPE_96N_CHARSET:
		p[len] = '$';
		p[len + 1] = '-';
		p[len + 2] = CS2FT(charset);
		len += 3;
		break;
	}
	if (output != iso8)
	{
		switch (CS2TYPE(charset))
		{
		case TYPE_94_CHARSET:
		case TYPE_94N_CHARSET:
			switch (CS2TYPE(old_output_charset))
			{
			case TYPE_96_CHARSET:
			case TYPE_96N_CHARSET:
				p[len] = '\017';
				len++;
			}
			break;
		case TYPE_96_CHARSET:
		case TYPE_96N_CHARSET:
			switch (CS2TYPE(old_output_charset))
			{
			case TYPE_94_CHARSET:
			case TYPE_94N_CHARSET:
				p[len] = '\016';
				len++;
			}
			break;
		}
	}
	p[len] = '\0';
	return (p);
}

static char cvbuffer[32];
static int cvindex = 0;
static char *nullcvbuffer = "";


static char *convert_to_iso(c, cs)
int c;
int cs;
{
	register unsigned char *p;
	static char buffer2[2];

	if (output == iso8 && c != 0 &&
	    (CS2TYPE(cs) == TYPE_96_CHARSET ||
	     CS2TYPE(cs) == TYPE_96N_CHARSET))
		c |= 0x80;

	buffer2[0] = c;
	buffer2[1] = '\0';

	if (CSISREST(cs))
	{
		return (buffer2);
	}
	if (CSISWRONG(cs))
	{
		cs = ASCII;
	}

	cs = CS2CHARSET(cs);

	if (cs == old_output_charset)
	{
		return (buffer2);
	}
	else
	{
		p = make_escape_sequence(cs);
		old_output_charset = cs;
		strcpy(cvbuffer, p);
		strcat(cvbuffer, buffer2);
		return (cvbuffer);
	}
}

static char *convert_to_jis(c, cs)
int c;
int cs;
{
	register unsigned char *p;
	static char buffer2[3];

	if (c == 0)
	{
		cvindex = 0;
		return (nullcvbuffer);
	}

	buffer2[cvindex++] = c;
	buffer2[cvindex] = '\0';

	if (CSISWRONG(cs))
	{
		cs = ASCII;
	}

	cs = CS2CHARSET(cs);

	if (cs == ASCII || cs == JISX0201ROMAN)
	{
		assert(cvindex == 1);
		cvindex = 0;
	} else if (cs == JISX0201KANA)
	{
		assert(cvindex == 1);
		cvindex = 0;
	} else if (cs == JISX0208_78KANJI)
	{
		if (cvindex == 1)
			return (nullcvbuffer);
		assert(cvindex == 2);
		jis78to90(buffer2);
		cs = JISX0208_90KANJI;
		cvindex = 0;
	} else if (cs == JISX0208KANJI || cs == JISX0208_90KANJI)
	{
		if (cvindex == 1)
			return (nullcvbuffer);
		assert(cvindex == 2);
		cvindex = 0;
	} else if (cs == JISX0213KANJI1)
	{
		if (cvindex == 1)
			return (nullcvbuffer);
		assert(cvindex == 2);
		cvindex = 0;
		cs = JISX0208KANJI;
	} else
	{
		assert(0);
		cvindex = 0;
	}

	if (cs == old_output_charset)
	{
		return (buffer2);
	}
	else
	{
		p = make_escape_sequence(cs);
		old_output_charset = cs;
		strcpy(cvbuffer, p);
		strcat(cvbuffer, buffer2);
		return (cvbuffer);
	}
}

#if JAPANESE
static char *convert_to_ujis(c, cs)
int c;
int cs;
{
	if (c == 0)
	{
		cvindex = 0;
		return (nullcvbuffer);
	}

	cvbuffer[cvindex++] = c;
	cvbuffer[cvindex] = '\0';

	if (CSISWRONG(cs))
	{
		cs = ASCII;
	}

	cs = CS2CHARSET(cs);

	if (cs == ASCII || cs == JISX0201ROMAN)
	{
		assert(cvindex == 1);
		cvindex = 0;
		return (cvbuffer);
	} else if (cs == JISX0201KANA)
	{
		assert(cvindex == 1);
		cvbuffer[2] = '\0';
		cvbuffer[1] = cvbuffer[0] | 0x80;
		cvbuffer[0] = 0x8e;
		cvindex = 0;
		return (cvbuffer);
	} else if (cs == JISX0208_78KANJI || cs == JISX0208KANJI ||
		   cs == JISX0208_90KANJI || cs == JISX0213KANJI1)
	{
		if (cvindex == 1)
			return (nullcvbuffer);
		assert(cvindex == 2);
		if (cs == JISX0208_78KANJI)
			jis78to90(cvbuffer);
		cvbuffer[0] |= 0x80;
		cvbuffer[1] |= 0x80;
		cvindex = 0;
		return (cvbuffer);
	} else if (cs == JISX0212KANJISUP)
	{
		if (cvindex == 1)
			return (nullcvbuffer);
		assert(cvindex == 2);
		cvbuffer[2] = cvbuffer[1] | 0x80;
		cvbuffer[1] = cvbuffer[0] | 0x80;
		cvbuffer[0] = 0x8f;
		cvbuffer[3] = '\0';
		cvindex = 0;
		return (cvbuffer);
	} else if (cs == JISX0213KANJI2)
	{
		if (cvindex == 1)
			return (nullcvbuffer);
		assert(cvindex == 2);
		cvbuffer[2] = cvbuffer[1] | 0x80;
		cvbuffer[1] = cvbuffer[0] | 0x80;
		cvbuffer[0] = 0x8f;
		cvbuffer[3] = '\0';
		cvindex = 0;
		return (cvbuffer);
	}
	assert(0);
	cvindex = 0;
	return (cvbuffer);
}

static char *convert_to_sjis(c, cs)
int c;
int cs;
{
	if (c == 0)
	{
		cvindex = 0;
		return (nullcvbuffer);
	}

	cvbuffer[cvindex++] = c;
	cvbuffer[cvindex] = '\0';

	if (CSISWRONG(cs))
	{
		cs = ASCII;
	}

	cs = CS2CHARSET(cs);

	if (cs == ASCII || cs == JISX0201ROMAN)
	{
		assert(cvindex == 1);
		cvindex = 0;
		return (cvbuffer);
	} else if (cs == JISX0201KANA)
	{
		assert(cvindex == 1);
		cvbuffer[0] |= 0x80;
		cvindex = 0;
		return (cvbuffer);
	} else if (cs == JISX0208_78KANJI || cs == JISX0208KANJI ||
		   cs == JISX0208_90KANJI || cs == JISX0213KANJI1)
	{
		register int c1, c2, c3;
		static unsigned char table[] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			   0, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
			0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
			0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
#if SJIS0213
			0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
#else
			0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E,    0,
#endif
			0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
#if SJIS0213
			0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
#else
			0xE8, 0xE9, 0xEA,    0,    0,    0,    0,    0,
#endif
		};

		if (cvindex == 1)
			return (nullcvbuffer);
		assert(cvindex == 2);
		if (cs == JISX0208_78KANJI)
			jis78to90(cvbuffer);
		c3 = cvbuffer[0] & 0x7f;
		c1 = c3 & 1;
		c2 = (cvbuffer[1] & 0x7f) + (c1 ? 0x40 - 0x21 : 0x9e - 0x21);
		c1 = table[c3 / 2 + c1];
		cvbuffer[0] = c1;
		cvbuffer[1] = c2 + (c2 >= 0x7f ? 1 : 0);
		cvindex = 0;
		return (cvbuffer);
	} else if (cs == JISX0213KANJI2)
	{
		register int c1, c2, c3;
		if (cvindex == 1)
			return (nullcvbuffer);
		assert(cvindex == 2);
		c3 = cvbuffer[0] & 0x7f;
		c1 = c3 & 1;
		c2 = (cvbuffer[1] & 0x7f) +
		     (c1 ? 0x40 - 0x21 : 0x9e - 0x21);
		if (c3 <= 0x25) {
			/* Map 1, 3, 4, and 5-KU */
			/* Note: 2-KU is rejected already. */
			c1 = (c3 - 0x21) / 2 + 0xf0;
		} else if (c3 == 0x28) {
			/* Map 8-KU */
			c1 = 0xf0;
		} else if (c3 <= 0x2f) {
			/* Map 12, 13, 14, and 15-KU */
			c1 = (c3 - 0x2b) / 2 + 0xf2;
		} else {
			/* Map 78-94 KU. */
			/* Note: 16-77 KU is rejected already. */
			c1 = (c3 - 0x6d) / 2 + 0xf4;
		}
		cvbuffer[0] = c1;
		cvbuffer[1] = c2 + (c2 >= 0x7f ? 1 : 0);
		cvindex = 0;
		return (cvbuffer);
	}
	assert(0);
	cvindex = 0;
	return (cvbuffer);
}
#endif

char *outchar(c, cs)
int c;
CHARSET cs;
{
	if (c < 0)
	{
		c = 0;
		cs = ASCII;
	}

	if (output == iso7 || output == iso8)
		return (convert_to_iso(c, cs));
	if (output == jis)
		return (convert_to_jis(c, cs));
#if JAPANESE
	if (output == ujis)
		return (convert_to_ujis(c, cs));
	if (output == sjis)
		return (convert_to_sjis(c, cs));
#endif
	cvbuffer[0] = c;
	cvbuffer[1] = '\0';
	return (cvbuffer);
}

char *outbuf(p, cs)
unsigned char *p;
CHARSET cs;
{
	static char buffer[1024];
	char *s;
	int i = 0;

	while (*p != '\0')
	{
		s = outchar(*p++, cs);
		while (*s != '\0')
			buffer[i++] = *s++;
		assert(i < (int)sizeof(buffer));
	}
	buffer[i] = '\0';
	return (buffer);
}

int mwidth(c, cs)
int c;
CHARSET cs;
{
	if (CSISREST(cs))
		return (0);
	switch (CS2TYPE(cs))
	{
	case TYPE_94_CHARSET:
	case TYPE_96_CHARSET:
		return (1);
	case TYPE_94N_CHARSET:
	case TYPE_96N_CHARSET:
		return (2);
	default:
		assert(0);
		return (0);
	}
}

char *rotate_right_codeset(mp)
MULBUF *mp;
{
	char *p = NULL;

	mp->rotation_io_right++;
	mp->rotation_io_right %= 7;
	switch (mp->rotation_io_right) {
	case 0: p = "original"; mp->io.right = mp->orig_io_right; break;
	case 1: p = "japanese"; mp->io.right = japanese; break;
	case 2: p = "ujis"; mp->io.right = ujis; break;
	case 3: p = "sjis"; mp->io.right = sjis; break;
	case 4: p = "iso8"; mp->io.right = iso8; break;
	case 5: p = "noconv"; mp->io.right = noconv; break;
	case 6: p = "none"; mp->io.right = none; break;
	default: assert(0); break;
	}
	init_priority(mp);
	return (p);
}

#endif

int strlen_cs(str, cs)
char* str;
CHARSET* cs;
{
	int i = 0;
	if (cs == NULL)
		return strlen(str);
	while (*str != NULCH || !CSISNULLCS(*cs)) {
		str++;
		cs++;
		i++;
	}
	return i;
}

int chlen_cs(chstr, cs)
char* chstr;
CHARSET* cs;
{
	int i;
	if (cs == NULL)
	{
		if (chstr == NULL || *chstr == NULCH)
			return 0;
		else
			return 1;
	}
	if (*chstr == NULCH && CSISNULLCS(*cs))
		return 0;
	i = 0;
	do {
		i++;
		cs++;
	} while (CSISREST(*cs));
	return i;
}

char* strdup_cs(str, cs, csout)
char* str;
CHARSET* cs;
CHARSET** csout;
{
	int len = strlen_cs(str, cs);
	char* save_str = (char *)ecalloc(len + 1, 1);
	CHARSET* save_cs = (CHARSET *)ecalloc(len + 1, sizeof(CHARSET));
	memcpy(save_str, str, sizeof(char) * (len + 1));
	if (cs)
		memcpy(save_cs, cs, sizeof(CHARSET) * (len + 1));
	else {
		cs = save_cs;
		while (--len >= 0)
			*cs++ = ASCII;
		*cs = NULLCS;
	}
	*csout = save_cs;
	return save_str;
}
