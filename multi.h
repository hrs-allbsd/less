/*
 * Copyright (c) 1997-2005  Kazushi (Jam) Marukawa
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
 * The design of data structure of jless
 *
 * We use char[] byte data and CHARSET[] character set data to represent
 * multilingual text.  We defined CHARSET following ISO 2022 technique.
 * All characters represented in ISO 2022 can be stored in less without
 * any destructive conversion.
 *
 * For example, less can read text files using JIS C 6226-1978, JIS X
 * 0208-1983, and JIS X 0208:1990 character sets and output everything
 * using their original character set while searching a character encoded
 * by JIS X 0213:2004.  Inside of less, it buffers all text files using
 * their original character set, unifies them when matching with the
 * searching character, and outputs using their original character sets.
 *
 * If less needs conversions when it outputs internal data, it converts
 * them on the fly.
 *
 * On the other hand, text using SJIS or UJIS are buffered after
 * conversion while less is reading input stream.
 *
 * In addition, UTF-8 is buffered as UTF-8.  Less converts it to appropriate
 * character set/sets on the fly. (UTF-8 is notimplemented yet).
 */

/*
 * Definition of values to specify the character set.
 * And definitions some well known character sets and a types of set.
 */
typedef unsigned short CHARSET;

/*
 * The structure of CHARSET:
 *
 *   151413121110 9 8 7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |r|    IRR    |m|n|      F      |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * r: True if it is not the first byte of multi-byte characters.
 * IRR: Identification of Revisions of Registered character sets (IRR).
 *      Read ISO-2022 for mode details.  The value of IRR is ranged from
 *      00/01 to 03/15.  00/00 means no IRR.  IRR (from 00/01 to 03/15)
 *      is mapped to a code from 04/00 to 07/14 in ISO-2022.
 * m: True if it is part of multi-byte characters.
 * n: True if it is one of 96 or 96x96 graphic sets, otherwise it is one
 *    of 94 or 94x94 graphic sets.
 * F: Final byte (F).  This select graphi sets of characters.
 *    The value of F is ranged from 00/00 to 04/14.  Such values are coded
 *    from 03/00 to 07/14 in ISO-2022.
 */

#define	REST_MASK		0x8000		/* r */
#define CSISHEAD(cs)		(!((cs) & REST_MASK))
#define CSISREST(cs)		((cs) & REST_MASK)

#define IRR_MASK		0x7e00		/* IRR */
#define IRR_SHIFT		9
#define CS2IRR(cs)		(((cs) & IRR_MASK) >> IRR_SHIFT)
#define IRR2CS(irr)		(((irr) << IRR_SHIFT) & IRR_MASK)

#define CODE_MASK		0x003f		/* coded IRR in ISO 2022 */
#define CODE_DIFF		0x0040
#define IRR2CODE(irr)		((((irr) - 1) & CODE_MASK) + CODE_DIFF)
#define CODE2IRR(code)		((((code) - CODE_DIFF) & CODE_MASK) + 1)

#define TYPE_94_CHARSET		0x0000		/* m & n */
#define TYPE_96_CHARSET		0x0080
#define TYPE_94N_CHARSET	0x0100
#define TYPE_96N_CHARSET	0x0180
#define TYPE_MASK		0x0180
#define CS2TYPE(cs)		((cs) & TYPE_MASK)
#define TYPE2CS(type)		((type) & TYPE_MASK)

#define FT_MASK			0x007f		/* F */
#define FT_DIFF			0x0030
#define CS2FT(cs)		(((cs) & FT_MASK) + FT_DIFF)
#define FT2CS(ft)		(((ft) - FT_DIFF) & FT_MASK)

/*
 * Each character sets is represented by IRR, TYPE and FT.
 */
#define CHARSET_MASK		(IRR_MASK | TYPE_MASK | FT_MASK)
#define CS2CHARSET(cs)		((cs) & CHARSET_MASK)

/*
 * There is a reserved empty set in every type of charset.  07/14.
 * So we cannot use (CS2CHARSET(cs) == WRONGCS) to check it.
 */
#define CSISWRONG(cs)		(CS2FT(cs) == '~')

/*
 * List of representative character sets.
 */
#define ASCII			(TYPE_94_CHARSET | FT2CS('B'))
#define WRONGCS			(TYPE_94_CHARSET | FT2CS('~'))
#if ISO
#define JISX0201KANA		(TYPE_94_CHARSET | FT2CS('I'))
#define JISX0201ROMAN		(TYPE_94_CHARSET | FT2CS('J'))
#define LATIN1			(TYPE_96_CHARSET | FT2CS('A'))
#define LATIN2			(TYPE_96_CHARSET | FT2CS('B'))
#define LATIN3			(TYPE_96_CHARSET | FT2CS('C'))
#define LATIN4			(TYPE_96_CHARSET | FT2CS('D'))
#define GREEK			(TYPE_96_CHARSET | FT2CS('F'))
#define ARABIC			(TYPE_96_CHARSET | FT2CS('G'))
#define HEBREW			(TYPE_96_CHARSET | FT2CS('H'))
#define CYRILLIC		(TYPE_96_CHARSET | FT2CS('L'))
#define LATIN5			(TYPE_96_CHARSET | FT2CS('M'))
/*
 * JISX0208_78KANJI means JIS C 6226-1978
 * JISX0208KANJI means JIS X 0208-1983 (same as JIS C 6226-1983)
 *   This is similar to JIS C 6226-1978.  Several characters are moved
 *   or exchanged in code space.  Conversion table is available in unify.c.
 * JISX0208_90KANJI means JIS X 0208:1990 (same as JIS X 0208-1990)
 *   This is super set of JIS X 0208-1983.  Two characters are added from
 *   JIS X 0208-1983.  In addition, this covers JIS X 0208:1997 too.
 *   They have the same code space.  The difference between them is
 *   historical description.  JIS X 0208:1997 defines ans describes
 *   all characters.
 * JISX0213KANJI1 means JIS X 0213:2000 plane 1
 *   This is super set of JIS X 0208:1990 and JIS X 0208:1997.  Several
 *   characters are added.
 * JISX02132004KANJI1 means JIS X 0213:2004 plane 1
 *   This is super set of JIS X 0213:2000.  10 characters are added.
 *   And, glyph of several characters is modified.
 *
 * JISX0212KANJISUP means JIS X 0212:1990 (same as JIS X 0212-1990)
 * JISX0213KANJI2 means JIS X 0213:2000 plane 1
 * JISX02132004KANJI2 means JIS X 0213:2004 plane 1
 *
 * JISX0201KANA means JIS X 0201:1976 right plane (same as JIS X 0201-1976
 * and JIS C 6220-1976 right plane)
 * JISX0201ROMAN means JIS X 0201:1976 left plane (same as JIS X 0201-1976
 * and JIS C 6220-1976 left plane)
 *   These cover JIS X 0201:1997 too.  They have the same code space.
 *   The difference between them is historical description.
 *   JIS X 0201:1997 defines ans describes all characters.
 */
#define JISX0208_78KANJI	(TYPE_94N_CHARSET | FT2CS('@'))
#define GB2312			(TYPE_94N_CHARSET | FT2CS('A'))
#define JISX0208KANJI		(TYPE_94N_CHARSET | FT2CS('B'))
#define JISX0208_90KANJI	(IRR2CS(1) | TYPE_94N_CHARSET | FT2CS('B'))
#define KSC5601			(TYPE_94N_CHARSET | FT2CS('C'))
#define JISX0212KANJISUP	(TYPE_94N_CHARSET | FT2CS('D'))
#define JISX0213KANJI1		(TYPE_94N_CHARSET | FT2CS('O'))
#define JISX0213KANJI2		(TYPE_94N_CHARSET | FT2CS('P'))
#define JISX02132004KANJI1	(TYPE_94N_CHARSET | FT2CS('Q'))
#define JISX02132004KANJI2	(TYPE_94N_CHARSET | FT2CS('P'))
#if JAPANESE
/*
 * Special number for Japanese code set.  Only input_set use following with
 * above definitions.  The 07/15 or 07/14 are not valid for F.  So, we are
 * using them as indications of special character sets.
 *
 * SJIS contains ASCII, JIS X 0201:1976 right plane, and JIS X 0208:1997
 * UJIS contains ASCII, JIS X 0201:1976, and JIS X 0208:1997
 * SJIS2000 contains ASCII, JIS X 0201:1976 right plane, and JIS X 0213:2000
 * UJIS2000 contains ASCII, JIS X 0201:1976, JIS X 0213:2000,
 * and JIS X 0212:1990
 * SJIS2004 contains ASCII, JIS X 0201:1976 right plane, and JIS X 0213:2004
 * UJIS2004 contains ASCII, JIS X 0201:1976, JIS X 0213:2004,
 * and JIS X 0212:1990
 */
#define SJIS			(IRR2CS(0) | TYPE_94N_CHARSET | FT_MASK)
#define SJIS2000		(IRR2CS(1) | TYPE_94N_CHARSET | FT_MASK)
#define SJIS2004		(IRR2CS(2) | TYPE_94N_CHARSET | FT_MASK)
#define UJIS			(IRR2CS(0) | TYPE_94N_CHARSET | (FT_MASK-1))
#define UJIS2000		(IRR2CS(1) | TYPE_94N_CHARSET | (FT_MASK-1))
#define UJIS2004		(IRR2CS(2) | TYPE_94N_CHARSET | (FT_MASK-1))

#define UTF8			(IRR2CS(0) | TYPE_94N_CHARSET | (FT_MASK-2))

/*
 * Make SJIS/UJIS character set from mp.
 *
 * SJIS and UJIS are using only fixed number of plane sets.  Therefore,
 * it is impossible to use JIS X 0208:1990 and JIS X 0213:2004 at the
 * same time.  SJIS use only one of them.  And, it is declared by
 * MULBUF->io.right.  This function constructs appropriate SJIS 
 * character set number from it.
 *
 * Usage: sjiscs = MAKESUJISCS(mp, SJIS);
 *        ujiscs = MAKESUJISCS(mp, UJIS);
 */
#define MAKESUJISCS(mp,su) \
	((su)| (((mp)->io.right&CJISX0213_2004)?IRR2CS(2):\
		(((mp)->io.right&CJISX0213_2000)?IRR2CS(1):0)))
#endif
#endif

/*
 * List of special characters and character set for it.
 *
 *	A terminator of string with character set is represented by
 *    both a NULCH and a NULLCS.  A padding character in string with
 *    character set is represented by both a PADCH and a NULLCS.  A
 *    binary data '\0' and '\1' are represented by both '\0' and a
 *    WRONGCS, and both '\1' and a WRONGCS respectively.
 */
#define NULCH			('\0')
#define PADCH			('\1')
#define NULLCS			(ASCII)

/*
 * Macros for easy checking.
 */
#define CSISASCII(cs)		(CS2CHARSET(cs) == ASCII)
#define CSISNULLCS(cs)		(CS2CHARSET(cs) == NULLCS)


/*
 * Definition of values to specify the character set and character.
 */
typedef int CHARVAL;

#define MAKECV(ch, cs)		(((cs) << 8 * sizeof(char)) | ch)
#define CV2CH(cv)		((cv) & ((1 << 8 * sizeof(char)) - 1))
#define CV2CS(cv)		((cv) >> 8 * sizeof(char))


/*
 * Definition of SETCHARSET.
 *
 * SETCHARSET represents a set of character sets.  This is used to
 * specify character sets less accepts.
 *
 * Although, ISO 2022 can accept any character sets, the output device
 * cannot represents all.  Therefore, we add less ability to specify
 * character sets that a user want to use.
 *
 * SCSASCII is a value to specify ASCII character set.
 * SCSJISX0201_1976..SCSJISX0213_2004 specify Japanese character sets.
 *   All of these are character sets are defined in Japan.  However,
 *   Japanese terminal devices can display only few of them.  So, we
 *   decide to give users the ability to specify character sets that
 *   their terminal device can display.
 * SCSOTHERISO is used to allow all other ISO 2022 character sets.
 *   There are too many character sets in the world.  And the number
 *   of them is increasing.  Therefore, we also decide to give users
 *   the ability to try all of them.  ;-)
 */
typedef int SETCHARSET;
#define SCSASCII		0x0000
#define SCSJISX0201_1976	0x0001
#define SCSJISC6226_1978	0x0002
#define SCSJISX0208_1983	0x0004
#define SCSJISX0208_1990	0x0008
#define SCSJISX0212_1990	0x0010
#define SCSJISX0213_2000	0x0020
#define SCSJISX0213_2004	0x0040
#define SCSJISX0213_2ND		0x0080	/* 2nd plane of JIS X 0213:2000 and */
					/* JIS X 0213:2004 */
#define SCSOTHERISO		0x0100
#define SCSUTF8			0x0200
/*
 * SCSALLJIS - everything
 * SCSALLJISTRAD - everything except JIS X 0213 plane 2 and JIS X 0212.
 * SCSALLSJIS - everything except JIS X 0212
 */
#define SCSALLJIS	(SCSJISX0201_1976|SCSJISC6226_1978|SCSJISX0208_1983|\
			 SCSJISX0208_1990|SCSJISX0213_2000|SCSJISX0213_2004|\
			 SCSJISX0213_2ND|SCSJISX0212_1990)
#define SCSALLJISTRAD	(SCSJISX0201_1976|SCSJISC6226_1978|SCSJISX0208_1983|\
			 SCSJISX0208_1990|SCSJISX0213_2000|SCSJISX0213_2004)
#define SCSALLSJIS	(SCSJISX0201_1976|SCSJISC6226_1978|SCSJISX0208_1983|\
			 SCSJISX0208_1990|SCSJISX0213_2000|SCSJISX0213_2004|\
			 SCSJISX0213_2ND)

/*
 * Definition of ENCSET.
 *
 * ENCSET represents a set of encoding schemes less accepts.  ENCSET is
 * used as a triplet like { input, inputr, output }.  "input" represents
 * a set of encoding schemes for input stream left plane (0x00..0x7f).
 * "inputr" represents a set of encoding schemes for input stream right
 * plane (0x80..0xff).  "output" represents an encoding scheme for output
 * stream.
 *
 * ESNONE has to be used exclusively to specify no-data.  This is used
 *   as only "inputr" to specify no right plane (0x80..0xff) data.
 * ESNOCONV has to be used exclusively to specify no-conversion.
 * ESISO7 and ESISO8 specify ISO style encoding techniques.  ESISO7 can
 *   be used as "input" or "output".  ESISO8 can be used as "inputr" or
 *   "output".
 * ESJIS83, ESSJIS, and ESUJIS specify Japanese encoding techniques.
 *   Note: As input, users can use any combination of these values.
 *   However, as output, users need to use only one of them.
 *   Note: If ESJIS83 is used as "output", less output all KANJI
 *   character set using only JIS X 0208-1983 character set (ESC$B) with
 *   a hope that user's terminal device is using glyph of JIS X 0213:2004
 *   plane 1 character set as its default glyph.  It is hard to update
 *   terminal device to understand JIS X 0213:2004 completely, but it is
 *   easy to change the glyph.
 * ESUTF8 specifies encoding technique and character set.  This have to
 *   be used exclusively as output.
 */
typedef int ENCSET;
#define ESNONE		0x0000
#define ESNOCONV	0x0001
#define ESISO7		0x0002
#define ESISO8		0x0004
#define ESJIS83		0x0008
#define ESSJIS		0x0010
#define ESUJIS		0x0020
#define ESUTF8		0x0040
#define ESALLJA		(ESSJIS|ESUJIS|ESUTF8)

/*
 * J_PRIORITY: priority to select either UJIS or SJIS as encoding scheme.
 */
typedef enum {
    PUJIS,
    PSJIS,
    PUTF8,
    PNONE
} J_PRIORITY;

/*
 * A structure used as a return value in multi_parse().
 */
typedef struct {
	char *cbuf;
	CHARSET *csbuf;
	int byte;
} M_BUFDATA;

/*
 * struct multibuf is internal data structure for multi.c.
 * Defines it name only.
 */
typedef struct multibuf MULBUF;


/*
 * in multi.c
 */
extern int set_planeset ();
extern void init_def_scs_es ();
extern void init_def_priority ();
extern void init_priority ();
extern J_PRIORITY get_priority ();
extern void set_priority ();
extern MULBUF * new_multibuf ();
extern void clear_multibuf ();
extern void init_multibuf ();
extern void multi_start ();
extern void multi_parse ();
extern void multi_flush ();
extern void multi_discard ();
extern void set_codesets ();
extern char * get_icharset_string ();
extern char * outchar();
extern char * outbuf();
extern int mwidth();
extern char * rotate_right_codeset ();
extern int strlen_cs();
extern int chlen_cs();
extern char* strdup_cs();

/*
 * in unify.c
 */
extern void jis78to90();
extern void chconvert_cs();
extern void chunify_cs();
extern int chcmp_cs();
extern int chisvalid_cs();
