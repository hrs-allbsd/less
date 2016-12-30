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
 * Macro for character detection
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
#define ISUTF8_HEAD(c)		(0xc0 <= (c) && (c) < 0xfe)
#define ISUTF8_REST(c)		(((c) & 0xc0) == 0x80)
#define ISUTF8_1(c)		((c) <= 0x7f)
#define ISUTF8_2(c1,c2)		(((c1) & 0xe0) == 0xc0 && ISUTF8_REST(c2))
#define ISUTF8_3(c1,c2,c3)	(((c1) & 0xf0) == 0xe0 && ISUTF8_REST(c2) && \
				 ISUTF8_REST(c3))
#define ISUTF8_4(c1,c2,c3,c4)	(((c1) & 0xf8) == 0xf0 && ISUTF8_REST(c2) && \
				 ISUTF8_REST(c3) && ISUTF8_REST(c4))
#define ISUTF8_5(c1,c2,c3,c4,c5) \
	(((c1) & 0xfc) == 0xf8 && ISUTF8_REST(c2) && ISUTF8_REST(c3) && \
	 ISUTF8_REST(c4) && ISUTF8_REST(c5))
#define ISUTF8_6(c1,c2,c3,c4,c5,c6) \
	(((c1) & 0xfe) == 0xfc && ISUTF8_REST(c2) && ISUTF8_REST(c3) && \
	 ISUTF8_REST(c4) && ISUTF8_REST(c5) && ISUTF8_REST(c6))
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


static SETCHARSET def_scs = SCSASCII | SCSOTHERISO;
static ENCSET def_input = ESISO7;	/* Default character set of left plane */
static ENCSET def_inputr = ESISO8;	/* Default character set of right plane */
static int def_gs[4] = {
    ASCII,				/* Default g0 plane status */
    WRONGCS,				/* Default g1 plane status */
    WRONGCS,				/* Default g2 plane status */
    WRONGCS				/* Default g3 plane status */
};

static ENCSET output = ESISO8;		/* Character set for output */
#if JAPANESE
static J_PRIORITY def_priority = PUJIS;	/* Which code was given priority. */
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
	SETCHARSET scs;
	ENCSET input;
	ENCSET inputr;
    } io;

    ENCSET orig_io_right;
    int rotation_io_right;

    enum escape_sequence eseq;
    /*
     * Variables to control of escape sequences as output.
     */
    int cs;			/* Current character set */
    struct m_status* ms;
#if JAPANESE
    J_PRIORITY priority;	/* Which code was given priority. */
    int sequence_counter;	/* Special counter for detect UJIS KANJI. */
#endif

    CHARSET icharset;		/* Last non ASCII character set of input */

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
#define INBUF3(mp)	((mp)->inbuf[((mp)->startpos+3)%sizeof((mp)->inbuf)])
#define INBUF4(mp)	((mp)->inbuf[((mp)->startpos+4)%sizeof((mp)->inbuf)])
#define INBUF5(mp)	((mp)->inbuf[((mp)->startpos+5)%sizeof((mp)->inbuf)])
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
    case UJIS2000:
    case UJIS2004:
	c = INBUF0(mp);
	if (ISUJISKANJI1(c)) return 2;
	if (ISUJISKANA1(c)) return 2;
	if (ISUJISKANJISUP1(c)) return 3;
	return 1;
    case SJIS:
    case SJIS2000:
    case SJIS2004:
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
    /* flush buffer */
    mp->startpos = mp->lastpos + 1;
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
	case JISX02132004KANJI1:
	case UJIS:
	case UJIS2000:
	case UJIS2004:
	case SJIS:
	case SJIS2000:
	case SJIS2004:
	    put_wrongmark(mp);
	    break;
	case GB2312:
	case KSC5601:
	default:
	    put_wrongmark(mp);
	    break;
	}
    } else {
	while (mp->startpos <= mp->lastpos) {
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
    }
}

#if JAPANESE
/*
 * Internalize input stream encoded by UJIS encoding scheme.
 *
 * Return 1 if input is recognized well.
 * Return 0 if input is rejected.
 */
static int internalize_ujis(mp)
MULBUF *mp;
{
    if (mp->lastpos - mp->startpos + 1 == 1) {
	/* do nothing.  return 1 to get next byte */
	return 1;
    } else if (mp->lastpos - mp->startpos + 1 == 2) {
	int c0 = INBUF0(mp);
	int c1 = INBUF1(mp);
	if (ISUJISKANA(c0, c1)) {
	    mp->cs = JISX0201KANA;
	    mp->icharset = UJIS;
	    mp->multiint[mp->intindex] = c1 & 0x7f;
	    mp->multics[mp->intindex] = mp->cs;
	    mp->intindex += 1;
	    mp->startpos = mp->lastpos + 1;
	    return 1;
	} else if (ISUJISKANJI(c0, c1)) {
	    if (mp->io.scs & SCSJISX0213_2004) {
		mp->icharset = UJIS2004;
		mp->cs = JISX02132004KANJI1;
	    } else if (mp->io.scs & SCSJISX0213_2000) {
		mp->icharset = UJIS2000;
		mp->cs = JISX0213KANJI1;
	    } else {
		mp->icharset = UJIS;
		mp->cs = JISX0208KANJI;
	    }
	    mp->multiint[mp->intindex] = c0;
	    mp->multics[mp->intindex] = mp->icharset;
	    mp->multiint[mp->intindex + 1] = c1;
	    mp->multics[mp->intindex + 1] = REST_MASK | mp->icharset;

	    /* Check character whether it has defined glyph or not */
	    if (chisvalid_cs(&mp->multiint[mp->intindex],
			     &mp->multics[mp->intindex])) {
		/* defined */
		mp->multiint[mp->intindex] = c0 & 0x7f;
		mp->multics[mp->intindex] = mp->cs;
		mp->multiint[mp->intindex + 1] = c1 & 0x7f;
		mp->multics[mp->intindex + 1] = REST_MASK | mp->cs;
		mp->intindex += 2;
		mp->startpos = mp->lastpos + 1;
	    } else {
		/* undefined.  less ignore them */
		wrongchar(mp);
	    }
	    /* data are recognized as kanji or wrong data, so return 1 */
	    return 1;
	} else if (ISUJISKANJISUP(c0, c1, 0xa1)) {
	    /* do nothing.  return 1 to get next byte */
	    return 1;
	}
    } else if (mp->lastpos - mp->startpos + 1 == 3) {
	int c0 = INBUF0(mp);
	int c1 = INBUF1(mp);
	int c2 = INBUF2(mp);
	if (ISUJISKANJISUP(c0, c1, c2)) {
	    mp->cs = JISX0212KANJISUP;
	    mp->icharset = UJIS;
	    mp->multiint[mp->intindex] = c0;
	    mp->multics[mp->intindex] = UJIS;
	    mp->multiint[mp->intindex + 1] = c1;
	    mp->multics[mp->intindex + 1] = REST_MASK | UJIS;
	    mp->multiint[mp->intindex + 2] = c2;
	    mp->multics[mp->intindex + 2] = REST_MASK | UJIS;

	    /* Check character whether it has defined glyph or not */
	    if (chisvalid_cs(&mp->multiint[mp->intindex],
			     &mp->multics[mp->intindex])) {
		/* defined */
		static unsigned char table_ujis[] = {
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		       0, 0x21,    0, 0x23, 0x24, 0x25,    0,    0,
		    0x28,    0,    0,    0, 0x2C, 0x2D, 0x2E, 0x2F,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		       0,    0,    0,    0,    0,    0,    0,    0,
		       0,    0,    0,    0,    0,    0, 0x6E, 0x6F,
		    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77,
		    0x78, 0x79, 0x7A, 0x7B, 0x7C, 0x7D, 0x7E,    0
		};
		c1 &= 0x7f;
		if (table_ujis[c1] != 0) {
		    /* JIS X 0213:2000 plane 2 */
		    if (output & ESJIS83) {
			/* JIS cannot output JIS X 0213:2000 plane 2 */
			wrongchar(mp);
		    } else {
			mp->cs = JISX0213KANJI2;
			mp->multiint[mp->intindex] = c1;
			mp->multics[mp->intindex] = mp->cs;
			mp->multiint[mp->intindex + 1] = c2 & 0x7f;
			mp->multics[mp->intindex + 1] = REST_MASK | mp->cs;
			mp->intindex += 2;
			mp->startpos = mp->lastpos + 1;
		    }
		} else {
		    /* JIS X 0212:1990 */
		    if (output & (ESSJIS | ESJIS83)) {
			/* SJIS cannot output JIS X 0212:1990 */
			wrongchar(mp);
		    } else {
			mp->multiint[mp->intindex] = c1;
			mp->multics[mp->intindex] = mp->cs;
			mp->multiint[mp->intindex + 1] = c2 & 0x7f;
			mp->multics[mp->intindex + 1] = REST_MASK | mp->cs;
			mp->intindex += 2;
			mp->startpos = mp->lastpos + 1;
		    }
		}
	    } else {
		/* undefined.  less ignore them */
		wrongchar(mp);
	    }
	    /* data are recognized as kanji or wrong data, so return 1 */
	    return 1;
	}
    }
    /* return 0 because this data sequence is not matched to UJIS */
    return 0;
}

/*
 * Internalize input stream encoded by SJIS encoding scheme.
 *
 * Return 1 if input is recognized well.
 * Return 0 if input is rejected.
 */
static int internalize_sjis(mp)
MULBUF *mp;
{
    if (mp->lastpos - mp->startpos + 1 == 1) {
	int c0 = INBUF(mp);
	if (ISSJISKANA(c0)) {
	    mp->cs = JISX0201KANA;
	    mp->icharset = SJIS;
	    mp->multiint[mp->intindex] = c0 & 0x7f;
	    mp->multics[mp->intindex] = mp->cs;
	    mp->intindex += 1;
	    mp->startpos = mp->lastpos + 1;
	    return 1;
	} else {
	    /* do nothing.  return 1 to get next byte */
	    return 1;
	}
    } else if (mp->lastpos - mp->startpos + 1 == 2) {
	int c0 = INBUF0(mp);
	int c1 = INBUF1(mp);
	if (ISSJISKANJI(c0, c1)) {
	    if (mp->io.scs & SCSJISX0213_2004) {
		mp->icharset = SJIS2004;
		mp->cs = JISX02132004KANJI1;
	    } else if (mp->io.scs & SCSJISX0213_2000) {
		mp->icharset = SJIS2000;
		mp->cs = JISX0213KANJI1;
	    } else {
		mp->icharset = SJIS;
		mp->cs = JISX0208KANJI;
	    }

	    mp->multiint[mp->intindex] = c0;
	    mp->multics[mp->intindex] = mp->icharset;
	    mp->multiint[mp->intindex + 1] = c1;
	    mp->multics[mp->intindex + 1] = REST_MASK | mp->icharset;

	    /*
	     * Check the correctness of SJIS encoded characters and
	     * convert them into internal representation.
	     */
	    if (chisvalid_cs(&mp->multiint[mp->intindex],
			     &mp->multics[mp->intindex])) {
		int c2, c3;
		static unsigned char table_sjis[] = {
		       0, 0x21, 0x23, 0x25, 0x27, 0x29, 0x2B, 0x2D,
		    0x2F, 0x31, 0x33, 0x35, 0x37, 0x39, 0x3B, 0x3D,
		    0x3F, 0x41, 0x43, 0x45, 0x47, 0x49, 0x4B, 0x4D,
		    0x4F, 0x51, 0x53, 0x55, 0x57, 0x59, 0x5B, 0x5D,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
		    0x5F, 0x61, 0x63, 0x65, 0x67, 0x69, 0x6B, 0x6D,
		    0x6F, 0x71, 0x73, 0x75, 0x77, 0x79, 0x7B, 0x7D,
		    0x80, 0xA3, 0x81, 0xAD, 0x82, 0xEF, 0xF1, 0xF3,
		    0xF5, 0xF7, 0xF9, 0xFB, 0xFD,    0,    0,    0
		};

		c0 = table_sjis[c0 & 0x7f];
		c2 = c1 - ((unsigned char)c1 >= 0x80 ? 1 : 0);
		c1 = c0;
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
		    if (output & ESJIS83) {
			/* JIS cannot output JIS X 0213:2000 plane 2 */
			wrongchar(mp);
		    } else {
			mp->cs = JISX0213KANJI2;
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
		/* undefined.  less ignore them */
		wrongchar(mp);
	    }
	    /* data are recognized as kanji or wrong data, so return 1 */
	    return 1;
	}
    }
    /* return 0 because this data sequence is not matched to UJIS */
    return 0;
}

/*
 * Internalize input stream encoded by UTF8 encoding scheme.
 *
 * Return 1 if input is recognized well.
 * Return 0 if input is rejected.
 */
static int internalize_utf8(mp)
MULBUF *mp;
{
    if (mp->lastpos - mp->startpos + 1 == 1) {
	/* do nothing.  return 1 to get next byte */
	return 1;
    } else if (mp->lastpos - mp->startpos + 1 == 2) {
	int c0 = INBUF0(mp);
	int c1 = INBUF1(mp);
	if (ISUTF8_2(c0, c1)) {
	    mp->cs = UTF8;
	    mp->icharset = UTF8;
	    mp->multiint[mp->intindex] = c0;
	    mp->multics[mp->intindex] = mp->cs;
	    mp->multiint[mp->intindex + 1] = c1;
	    mp->multics[mp->intindex + 1] = REST_MASK | mp->cs;
	    if (output & ESUTF8) {
		mp->intindex += 2;
		mp->startpos = mp->lastpos + 1;
		return 1;
	    } else {
		mp->intindex += 2;
		mp->startpos = mp->lastpos + 1;
		return 1;
	    }
	} else if (ISUJISKANJI(c0, c1)) {
	    if (mp->io.scs & SCSJISX0213_2004) {
		mp->icharset = UJIS2004;
		mp->cs = JISX02132004KANJI1;
	    } else if (mp->io.scs & SCSJISX0213_2000) {
		mp->icharset = UJIS2000;
		mp->cs = JISX0213KANJI1;
	    } else {
		mp->icharset = UJIS;
		mp->cs = JISX0208KANJI;
	    }
	    mp->multiint[mp->intindex] = c0;
	    mp->multics[mp->intindex] = mp->icharset;
	    mp->multiint[mp->intindex + 1] = c1;
	    mp->multics[mp->intindex + 1] = REST_MASK | mp->icharset;

	    /* Check character whether it has defined glyph or not */
	    if (chisvalid_cs(&mp->multiint[mp->intindex],
			     &mp->multics[mp->intindex])) {
		/* defined */
		mp->multiint[mp->intindex] = c0 & 0x7f;
		mp->multics[mp->intindex] = mp->cs;
		mp->multiint[mp->intindex + 1] = c1 & 0x7f;
		mp->multics[mp->intindex + 1] = REST_MASK | mp->cs;
		mp->intindex += 2;
		mp->startpos = mp->lastpos + 1;
	    } else {
		/* undefined.  less ignore them */
		wrongchar(mp);
	    }
	    /* data are recognized as kanji or wrong data, so return 1 */
	    return 1;
	} else if (ISUTF8_HEAD(c0) && ISUTF8_REST(c1)) {
	    /* do nothing.  return 1 to get next byte */
	    return 1;
	}
    } else if (mp->lastpos - mp->startpos + 1 == 3) {
	int c0 = INBUF0(mp);
	int c1 = INBUF1(mp);
	int c2 = INBUF2(mp);
	if (ISUTF8_3(c0, c1, c2)) {
	    mp->cs = UTF8;
	    mp->icharset = UTF8;
	    mp->multiint[mp->intindex] = c0;
	    mp->multics[mp->intindex] = mp->cs;
	    mp->multiint[mp->intindex + 1] = c1;
	    mp->multics[mp->intindex + 1] = REST_MASK | mp->cs;
	    mp->multiint[mp->intindex + 2] = c2;
	    mp->multics[mp->intindex + 2] = REST_MASK | mp->cs;
	    mp->intindex += 3;
	    mp->startpos = mp->lastpos + 1;
	    /* data are recognized as kanji or wrong data, so return 1 */
	    return 1;
	} else if (ISUTF8_HEAD(c0) && ISUTF8_REST(c1) && ISUTF8_REST(c2)) {
	    /* do nothing.  return 1 to get next byte */
	    return 1;
	}
    } else if (mp->lastpos - mp->startpos + 1 == 4) {
	int c0 = INBUF0(mp);
	int c1 = INBUF1(mp);
	int c2 = INBUF2(mp);
	int c3 = INBUF3(mp);
	if (ISUTF8_4(c0, c1, c2, c3)) {
	    mp->cs = UTF8;
	    mp->icharset = UTF8;
	    mp->multiint[mp->intindex] = c0;
	    mp->multics[mp->intindex] = mp->cs;
	    mp->multiint[mp->intindex + 1] = c1;
	    mp->multics[mp->intindex + 1] = REST_MASK | mp->cs;
	    mp->multiint[mp->intindex + 2] = c2;
	    mp->multics[mp->intindex + 2] = REST_MASK | mp->cs;
	    mp->multiint[mp->intindex + 3] = c3;
	    mp->multics[mp->intindex + 3] = REST_MASK | mp->cs;
	    mp->intindex += 4;
	    mp->startpos = mp->lastpos + 1;
	    /* data are recognized as kanji or wrong data, so return 1 */
	    return 1;
	} else if (ISUTF8_HEAD(c0) && ISUTF8_REST(c1) && ISUTF8_REST(c2) &&
		   ISUTF8_REST(c3)) {
	    /* do nothing.  return 1 to get next byte */
	    return 1;
	}
    } else if (mp->lastpos - mp->startpos + 1 == 5) {
	int c0 = INBUF0(mp);
	int c1 = INBUF1(mp);
	int c2 = INBUF2(mp);
	int c3 = INBUF3(mp);
	int c4 = INBUF4(mp);
	if (ISUTF8_5(c0, c1, c2, c3, c4)) {
	    mp->cs = UTF8;
	    mp->icharset = UTF8;
	    mp->multiint[mp->intindex] = c0;
	    mp->multics[mp->intindex] = mp->cs;
	    mp->multiint[mp->intindex + 1] = c1;
	    mp->multics[mp->intindex + 1] = REST_MASK | mp->cs;
	    mp->multiint[mp->intindex + 2] = c2;
	    mp->multics[mp->intindex + 2] = REST_MASK | mp->cs;
	    mp->multiint[mp->intindex + 3] = c3;
	    mp->multics[mp->intindex + 3] = REST_MASK | mp->cs;
	    mp->multiint[mp->intindex + 4] = c4;
	    mp->multics[mp->intindex + 4] = REST_MASK | mp->cs;
	    mp->intindex += 5;
	    mp->startpos = mp->lastpos + 1;
	    /* data are recognized as kanji or wrong data, so return 1 */
	    return 1;
	} else if (ISUTF8_HEAD(c0) && ISUTF8_REST(c1) && ISUTF8_REST(c2) &&
		   ISUTF8_REST(c3) && ISUTF8_REST(c4)) {
	    /* do nothing.  return 1 to get next byte */
	    return 1;
	}
    } else if (mp->lastpos - mp->startpos + 1 == 6) {
	int c0 = INBUF0(mp);
	int c1 = INBUF1(mp);
	int c2 = INBUF2(mp);
	int c3 = INBUF3(mp);
	int c4 = INBUF4(mp);
	int c5 = INBUF5(mp);
	if (ISUTF8_6(c0, c1, c2, c3, c4, c5)) {
	    mp->cs = UTF8;
	    mp->icharset = UTF8;
	    mp->multiint[mp->intindex] = c0;
	    mp->multics[mp->intindex] = mp->cs;
	    mp->multiint[mp->intindex + 1] = c1;
	    mp->multics[mp->intindex + 1] = REST_MASK | mp->cs;
	    mp->multiint[mp->intindex + 2] = c2;
	    mp->multics[mp->intindex + 2] = REST_MASK | mp->cs;
	    mp->multiint[mp->intindex + 3] = c3;
	    mp->multics[mp->intindex + 3] = REST_MASK | mp->cs;
	    mp->multiint[mp->intindex + 4] = c4;
	    mp->multics[mp->intindex + 4] = REST_MASK | mp->cs;
	    mp->multiint[mp->intindex + 5] = c5;
	    mp->multics[mp->intindex + 5] = REST_MASK | mp->cs;
	    mp->intindex += 6;
	    mp->startpos = mp->lastpos + 1;
	    /* data are recognized as kanji or wrong data, so return 1 */
	    return 1;
	}
    }
    /* return 0 because this data sequence is not matched to UTF8 */
    return 0;
}

#endif

static void internalize(mp)
MULBUF *mp;
{
    int c = INBUF(mp);

    if (mp->lastpos - mp->startpos + 1 == 1) {
	if ((c <= 0x7f && mp->io.input == ESNOCONV) ||
	    (c >= 0x80 && mp->io.inputr == ESNOCONV)) {
#if JAPANESE
	    mp->sequence_counter = 0;
#endif
	    if (control_char(c)) {
		    wrongcs1(mp);
	    } else {
		    noconv1(mp);
	    }
	    return;
	} else if (c >= 0x80 && mp->io.inputr == ESNONE) {
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
		   ((mp->io.inputr & ESISO8) && (0xa0 <= c && c <= 0xff))) {
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
	    if ((output & ESJIS83) && mp->cs != ASCII &&
		mp->cs != JISX0201KANA &&
		mp->cs != JISX0201ROMAN &&
		mp->cs != JISX0208_78KANJI &&
		mp->cs != JISX0208KANJI &&
		mp->cs != JISX0208_90KANJI &&
		mp->cs != JISX0213KANJI1 &&
		mp->cs != JISX02132004KANJI1) {
		wrongcs1(mp);
		multi_reparse(mp);
		return;
	    }
	    /* UJIS cannot output regular ISO2022 except JIS */
	    if ((output & ESUJIS) && mp->cs != ASCII &&
		mp->cs != JISX0201KANA &&
		mp->cs != JISX0201ROMAN &&
		mp->cs != JISX0208_78KANJI &&
		mp->cs != JISX0208KANJI &&
		mp->cs != JISX0208_90KANJI &&
		mp->cs != JISX0212KANJISUP &&
		mp->cs != JISX0213KANJI1 &&
		mp->cs != JISX0213KANJI2 &&
		mp->cs != JISX02132004KANJI1) {
		wrongcs1(mp);
		multi_reparse(mp);
		return;
	    }
	    /* SJIS cannot output JISX0212 or ISO2022 */
	    if ((output & ESSJIS) && mp->cs != ASCII &&
		mp->cs != JISX0201KANA &&
		mp->cs != JISX0201ROMAN &&
		mp->cs != JISX0208_78KANJI &&
		mp->cs != JISX0208KANJI &&
		mp->cs != JISX0208_90KANJI &&
		mp->cs != JISX0213KANJI1 &&
		mp->cs != JISX0213KANJI2 &&
		mp->cs != JISX02132004KANJI1) {
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
	if (mp->priority == PSJIS && ISSJISKANA(c)) {
	    if (mp->io.inputr & ESUJIS) {
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
		     * to not PSJIS.
		     */
		    mp->priority = PUJIS;
	    }
	    internalize_sjis(mp);
	    return;
	} else if (mp->io.inputr & (ESUJIS | ESSJIS)) {
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
		((mp->io.inputr & ESISO8) && 0xa0 <= c && c <= 0xff))) {
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
	if (mp->priority == PSJIS) {
	    if (internalize_sjis(mp)) {
		return;
	    }
	} else if (mp->priority == PUJIS) {
	    if (internalize_ujis(mp)) {
		return;
	    }
	} else if (mp->priority == PUTF8) {
	    if (internalize_utf8(mp)) {
		return;
	    }
	}

	if (mp->io.inputr & ESUJIS) {
	    if (internalize_ujis(mp)) {
		mp->priority = PUJIS;
		return;
	    }
	}
	if (mp->io.inputr & ESUTF8) {
	    if (internalize_utf8(mp)) {
		mp->priority = PUTF8;
		return;
	    }
	}
	if (mp->io.inputr & ESSJIS) {
	    if (internalize_sjis(mp)) {
		mp->priority = PSJIS;
		return;
	    }
	}
    } else if (mp->lastpos - mp->startpos + 1 == 3) {
	if (mp->io.inputr & ESUJIS) {
	    if (internalize_ujis(mp)) {
		mp->priority = PUJIS;
		return;
	    }
	}
	if (mp->io.inputr & ESUJIS) {
	    if (internalize_utf8(mp)) {
		mp->priority = PUTF8;
		return;
	    }
	}
    } else if (mp->lastpos - mp->startpos + 1 == 4) {
	if (mp->io.inputr & ESUJIS) {
	    if (internalize_utf8(mp)) {
		mp->priority = PUTF8;
		return;
	    }
	}
    } else if (mp->lastpos - mp->startpos + 1 == 5) {
	if (mp->io.inputr & ESUJIS) {
	    if (internalize_utf8(mp)) {
		mp->priority = PUTF8;
		return;
	    }
	}
    } else if (mp->lastpos - mp->startpos + 1 == 6) {
	if (mp->io.inputr & ESUJIS) {
	    if (internalize_utf8(mp)) {
		mp->priority = PUTF8;
		return;
	    }
	}
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
    if (type == TYPE_94_CHARSET) {
	switch (c) {
	case 'B': /* ASCII */
	    goto ok;
	case 'I': /* JIS X 0201 right half (Katakana) */
	case 'J': /* JIS X 0201 left half (Roman) */
	    if (mp->io.scs & SCSJISX0201_1976) goto ok;
	}
    } else if (type == TYPE_94N_CHARSET) {
	switch (c) {
	case '@': /* JIS C 6226-1978 */
	    if (mp->io.scs & SCSJISC6226_1978) goto ok;
	    break;
	case 'B': /* JIS X 0208-1983, JIS X 0208:1990, or JIS X 0208:1997 */
	    if (mp->io.scs & (SCSJISX0208_1983 | SCSJISX0208_1990)) goto ok;
	    break;
	case 'D': /* JIS X 0212:1990 */
	    if (mp->io.scs & SCSJISX0212_1990) goto ok;
	    break;
	case 'O': /* JIS X 0213:2000 plane 1 */
	    if (mp->io.scs & SCSJISX0213_2000) goto ok;
	    break;
	case 'P': /* JIS X 0213:2000 plane 2 or JIS X 0213:2004 plane 2 */
	    if (mp->io.scs & (SCSJISX0213_2000 | SCSJISX0213_2004)) goto ok;
	    break;
	case 'Q': /* JIS X 0213:2004 plane 1 */
	    if (mp->io.scs & SCSJISX0213_2004) goto ok;
	    break;
	}
    }
    if ((mp->io.scs & SCSOTHERISO) && 0x30 <= c && c <= 0x7e) {
	/* accepting all other ISO, so OK */
	goto ok;
    }
    return (-1);
ok:
    *plane = (mp->ms->irr ? IRR2CS(mp->ms->irr) : 0) | TYPE2CS(type) | FT2CS(c);
    mp->ms->irr = 0;
    mp->eseq = NOESC;
    return (0);
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
	case '|': if (!(mp->io.inputr & ESISO8)) goto wrong;
		  mp->ms->gr = 3; mp->eseq = NOESC; break;
	case '}': if (!(mp->io.inputr & ESISO8)) goto wrong;
		  mp->ms->gr = 2; mp->eseq = NOESC; break;
	case '~': if (!(mp->io.inputr & ESISO8)) goto wrong;
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
	if (mp->lastpos > mp->startpos) {
	    switch (c) {
	    case 0033:
	    case 0016:
	    case 0017:
	    case 0031: goto wrong;
	    case 0216:
	    case 0217: if (mp->io.inputr & ESISO8) goto wrong;
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
	case 0216: if (!(mp->io.inputr & ESISO8)) goto wrongone;
		   mp->ms->sg = 2; mp->eseq = NOESC; /*SS2*/ break;
	case 0217: if (!(mp->io.inputr & ESISO8)) goto wrongone;
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

void init_def_scs_es(scs, input, inputr, out)
SETCHARSET scs;
ENCSET input;
ENCSET inputr;
ENCSET out;
{
    def_scs = scs;
    def_input = input;
    def_inputr = inputr;
    output = out;
}

void init_def_priority(pri)
J_PRIORITY pri;
{
#if JAPANESE
    assert(pri == PUJIS || pri == PSJIS || pri == PUTF8);
    def_priority = pri;
#endif
}

void init_priority(mp)
MULBUF *mp;
{
#if JAPANESE
    if ((mp->io.inputr & ESSJIS) && (mp->io.inputr & ESUJIS))
	mp->priority = def_priority;
    else if (mp->io.inputr & ESUJIS)
	mp->priority = PUJIS;
    else if (mp->io.inputr & ESUTF8)
	mp->priority = PUTF8;
    else if (mp->io.inputr & ESSJIS)
	mp->priority = PSJIS;
    else
	mp->priority = PNONE;
    mp->sequence_counter = 0;
#endif
}

J_PRIORITY get_priority(mp)
MULBUF *mp;
{
#if JAPANESE
    return (mp->priority);
#else
    return (PNONE);
#endif
}

void set_priority(mp, pri)
MULBUF *mp;
J_PRIORITY pri;
{
#if JAPANESE
    assert(pri == PSJIS || pri == PUJIS || pri == PUTF8 || pri == PNONE);
    mp->priority = pri;
#endif
}

MULBUF *new_multibuf()
{
    MULBUF *mp = (MULBUF*) ecalloc(1, sizeof(MULBUF));
    mp->io.scs = def_scs;
    mp->io.input = def_input;
    mp->io.inputr = def_inputr;
    mp->orig_io_right = def_inputr;
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

    if (mp->io.input & (ESJIS83 | ESISO7 | ESISO8)) {
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
     * Check whether character is SJIS KANA or not.
     * If it is SJIS KANA, it means our prediction was failed.
     * Now going to fall back to SJIS KANA mode.
     */
    if ((mp->priority == PSJIS || (mp->io.inputr & ESSJIS)) &&
	CSISWRONG(mp->multics[mp->intindex - 1]) &&
	ISSJISKANA(mp->multiint[mp->intindex - 1])) {
	mp->cs = JISX0201KANA;
	mp->priority = PSJIS;
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

void set_codesets(mp, input, inputr)
MULBUF *mp;
ENCSET input;
ENCSET inputr;
{
    mp->io.input = input;
    mp->io.inputr = inputr;
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
	case SJIS2000:		return ("SJIS2000");
	case SJIS2004:		return ("SJIS2004");
	case UJIS:		return ("UJIS");
	case UJIS2000:		return ("UJIS2000");
	case UJIS2004:		return ("UJIS2004");
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
	case JISX02132004KANJI1:return ("JISX0213:2004KANJI1");
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
	if ((output & (ESISO7 | ESISO8)) && CS2IRR(charset) > 0)
	{
		p[len] = '&';
		p[len + 1] = IRR2CODE(CS2IRR(charset));
		p[len + 2] = '\033';
		len += 3;
	}
	/*
	 * Call 94 or 94N character set to G0 plane.
	 * Call 96 or 96N character set to G1 plane.
	 */
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
	/*
	 * If output is not ESISO8, use SO and SI to call G1 to GL.
	 * Otherwise, we use GR directly, so no need to call G1
	 * since G1 is called GR already.
	 */
	if (!(output & ESISO8))
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

	if ((output & ESISO8) && c != 0 &&
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
	} else if (cs == JISX02132004KANJI1)
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
		   cs == JISX0208_90KANJI || cs == JISX0213KANJI1 ||
		   cs == JISX02132004KANJI1)
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
		   cs == JISX0208_90KANJI || cs == JISX0213KANJI1 ||
		   cs == JISX02132004KANJI1)
	{
		register int c1, c2, c3;
		static unsigned char table_sjis[] = {
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
			   0, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
			0x88, 0x89, 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F,
			0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97,
			0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D, 0x9E, 0x9F,
			0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7,
			0xE8, 0xE9, 0xEA, 0xEB, 0xEC, 0xED, 0xEE, 0xEF,
		};

		if (cvindex == 1)
			return (nullcvbuffer);
		assert(cvindex == 2);
		if (cs == JISX0208_78KANJI)
			jis78to90(cvbuffer);
		c3 = cvbuffer[0] & 0x7f;
		c1 = c3 & 1;
		c2 = (cvbuffer[1] & 0x7f) + (c1 ? 0x40 - 0x21 : 0x9e - 0x21);
		c1 = table_sjis[c3 / 2 + c1];
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

static char *convert_to_utf8(c, cs)
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

	assert(0);
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
		   cs == JISX0208_90KANJI || cs == JISX0213KANJI1 ||
		   cs == JISX02132004KANJI1)
	{
		cvindex = 0;
		return (cvbuffer);
	} else if (cs == JISX0213KANJI2)
	{
		cvindex = 0;
		return (cvbuffer);
	}
	assert(0);
	cvindex = 0;
	return (cvbuffer);
}

char *outchar(c, cs)
int c;
CHARSET cs;
{
	if (c < 0)
	{
		c = 0;
		cs = ASCII;
	}

	if (output & (ESISO7 | ESISO8))
		return (convert_to_iso(c, cs));
	if (output & ESJIS83)
		return (convert_to_jis(c, cs));
#if JAPANESE
	if (output & ESUJIS)
		return (convert_to_ujis(c, cs));
	if (output & ESSJIS)
		return (convert_to_sjis(c, cs));
#endif
	if (output & ESUTF8)
		return (convert_to_utf8(c, cs));
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
	case 0: p = "original"; mp->io.inputr = mp->orig_io_right; break;
	case 1: p = "japanese"; mp->io.inputr = ESUJIS | ESSJIS; break;
	case 2: p = "ujis"; mp->io.inputr = ESUJIS; break;
	case 3: p = "sjis"; mp->io.inputr = ESSJIS; break;
	case 4: p = "iso8"; mp->io.inputr = ESISO8; break;
	case 5: p = "noconv"; mp->io.inputr = ESNOCONV; break;
	case 6: p = "none"; mp->io.inputr = ESNONE; break;
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
