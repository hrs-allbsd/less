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
#define JISX0208_78KANJI	(TYPE_94N_CHARSET | FT2CS('@'))
#define GB2312			(TYPE_94N_CHARSET | FT2CS('A'))
#define JISX0208KANJI		(TYPE_94N_CHARSET | FT2CS('B'))
#define JISX0208_90KANJI	(IRR2CS(1) | TYPE_94N_CHARSET | FT2CS('B'))
#define KSC5601			(TYPE_94N_CHARSET | FT2CS('C'))
#define JISX0212KANJISUP	(TYPE_94N_CHARSET | FT2CS('D'))
#define JISX0213KANJI1		(TYPE_94N_CHARSET | FT2CS('O'))
#define JISX0213KANJI2		(TYPE_94N_CHARSET | FT2CS('P'))
#if JAPANESE
/*
 * Special number for Japanese code set.  Only input_set use following with
 * above definitions.  The 07/15 is not valid for F.  Thus I use it to
 * indicate the special character sets.
 */
#define SJIS			(IRR2CS(1) | TYPE_94N_CHARSET | FT_MASK)
#define UJIS			(IRR2CS(2) | TYPE_94N_CHARSET | FT_MASK)
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
 * Definition of code sets.  The code set is not character set.
 * It is only means of code, and we use these value when we
 * decide what input data are.
 */
typedef enum {
	/* code sets for left, right and output plane */
	noconv,		/* A code set which doesn't need converting */
	/* code sets for left and output plane */
	jis,		/* A subset of ISO 2022 */
		/*
		 * It may contain JIS C 6226-1978, JIS X 0208-1983, 
		 * JIS X 0208:1990/1997, JIS X 0212:1990, JIS X 0213:2000,
		 * JIS X 0201:1976/1997 left/right planes, and ASCII as input.
		 *
		 * In the case of output, this means all JIS C 6226-1978,
		 * JIS X 0208-1983, JIS X 0208:1990/1997, and JIS X 0213:2000
		 * are converted into JIS X 0208-1983 encode with an assumption
		 * that character set of JIS X 0208-1983 encode is
		 * JIS X 0213:2000.  And JIS X 0212:1990 and 2nd plane of
		 * JIS X 0213:2000 are rejected when output.
		 *
		 * If you need the same code as the output, please use iso7
		 * or iso8.
		 */
	iso7,		/* A code set which is extented by iso2022 */
	/* code sets for only right plane */
	none,		/* No code set */
	japanese,	/* Both of UJIS and SJIS */
	/* code sets for right and output plane */
	ujis,		/* Japanese code set named UJIS */
	sjis,		/* Japanese code set named SJIS */
	iso8		/* A code set which is extented by iso2022 */
} CODESET;


/*
 * struct multibuf is internal data structure for multi.c.
 * Defines it name only.
 */
typedef struct multibuf MULBUF;


/*
 * in multi.c
 */
extern int set_planeset ();
extern void init_def_codesets ();
extern void init_def_priority ();
extern void init_priority ();
extern CODESET get_priority ();
extern void set_priority ();
extern MULBUF * new_multi ();
extern void clear_multi ();
extern void init_multi ();
extern void multi_start_buffering ();
extern void multi_buffering ();
extern void multi_parsing ();
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
