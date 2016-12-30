/*
 * Copyright (c) 1998-2005  Kazushi (Jam) Marukawa
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
 * Routines to unify a multi bytes character.
 */

#include "defines.h"
#include "multi.h"


#if ISO

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

typedef struct {
	char* input1;	/* if input2 is null, convert input1 to output */
	char* input2;	/* if input2 is here, convert input1-input2 to output */
	char* output;
	CHARSET charset;
} convtab;

typedef struct {
	int num;
	convtab tab[1];
} sortedconvtab;

typedef struct {
	convtab* ctab;
	sortedconvtab* sctab;
} convtable;

static int comp_convtab(p, q)
void* p;
void* q;
{
	return strcmp(((convtab*)p)->input1, ((convtab*)q)->input1);
}

static sortedconvtab* make_sortedconvtab(tab)
convtab tab[];
{
	int i;
	sortedconvtab* sctab;

	for (i = 0; tab[i].input1 != NULL; i++)
		;
	sctab = (sortedconvtab*)malloc(sizeof(sortedconvtab) +
				       sizeof(convtab) * i - 1);
	if (sctab == NULL)
		return NULL;
	sctab->num = i;
	for (i = 0; i < sctab->num; i++)
		sctab->tab[i] = tab[i];
	qsort(sctab->tab, sctab->num, sizeof(convtab), comp_convtab);
	return sctab;
}

static convtab* find_convtab_from_sctab(sctab, input)
sortedconvtab* sctab;
char* input;
{
	int from = 0;
	int to = sctab->num;
	int cur;
	int cmp;

	if (to == 0)
		return NULL;
	while (1) {
		cur = (from + to) / 2;
		cmp = strcmp(input, sctab->tab[cur].input1);
		if (cmp == 0)
			return &sctab->tab[cur];
		if (sctab->tab[cur].input2 &&
		    cmp > 0 &&
		    strcmp(input, sctab->tab[cur].input2) <= 0)
			return &sctab->tab[cur];
		if (to - from == 1)
			return NULL;
		if (cmp < 0)
			to = cur;
		if (0 < cmp)
			from = cur;
	}
}

static void init_convtable(ctable)
convtable* ctable;
{
	if (ctable->sctab == NULL)
		ctable->sctab = make_sortedconvtab(ctable->ctab);
}

static convtab* find_convtab(ctable, input)
convtable* ctable;
char* input;
{
	convtab* ptab;
	int cmp;

	if (ctable->sctab == NULL)
		init_convtable(ctable);

	if (ctable->sctab != NULL)
		return find_convtab_from_sctab(ctable->sctab, input);

	for (ptab = ctable->ctab; ptab->input1; ptab++)
		if ((cmp = strcmp(input, ptab->input1)) == 0)
			return ptab;
		else if (ptab->input2 &&
			 cmp > 0 &&
			 strcmp(input, ptab->input2) <= 0)
			return ptab;
	return NULL;
}

static convtab conv_jisx0208_78_90[] = {
	/* 0x3646($@6F(B) -> 0x7421(&@$Bt!(B) */
	{ "6F", NULL, "t!", JISX0208_90KANJI },
	/* 0x4B6A($@Kj(B) -> 0x7422(&@$Bt"(B) */
	{ "Kj", NULL, "t\"", JISX0208_90KANJI },
	/* 0x4D5A($@MZ(B) -> 0x7423(&@$Bt#(B) */
	{ "MZ", NULL, "t#", JISX0208_90KANJI },
	/* 0x6076($@`v(B) -> 0x7424(&@$Bt$(B) */
	{ "`v", NULL, "t$", JISX0208_90KANJI },
	/* 0x3033($@03(B) -> 0x724D(&@$BrM(B) */
	{ "03", NULL, "rM", JISX0208_90KANJI },
	/* 0x724D($@rM(B) -> 0x3033(&@$B03(B) */
	{ "rM", NULL, "03", JISX0208_90KANJI },
	/* 0x3229($@2)(B) -> 0x7274(&@$Brt(B) */
	{ "2)", NULL, "rt", JISX0208_90KANJI },
	/* 0x7274($@rt(B) -> 0x3229(&@$B2)(B) */
	{ "rt", NULL, "2)", JISX0208_90KANJI },
	/* 0x3342($@3B(B) -> 0x695A(&@$BiZ(B) */
	{ "3B", NULL, "iZ", JISX0208_90KANJI },
	/* 0x695A($@iZ(B) -> 0x3342(&@$B3B(B) */
	{ "iZ", NULL, "3B", JISX0208_90KANJI },
	/* 0x3349($@3I(B) -> 0x5978(&@$BYx(B) */
	{ "3I", NULL, "Yx", JISX0208_90KANJI },
	/* 0x5978($@Yx(B) -> 0x3349(&@$B3I(B) */
	{ "Yx", NULL, "3I", JISX0208_90KANJI },
	/* 0x3376($@3v(B) -> 0x635E(&@$Bc^(B) */
	{ "3v", NULL, "c^", JISX0208_90KANJI },
	/* 0x635E($@c^(B) -> 0x3376(&@$B3v(B) */
	{ "c^", NULL, "3v", JISX0208_90KANJI },
	/* 0x3443($@4C(B) -> 0x5E75(&@$B^u(B) */
	{ "4C", NULL, "^u", JISX0208_90KANJI },
	/* 0x5E75($@^u(B) -> 0x3443(&@$B4C(B) */
	{ "^u", NULL, "4C", JISX0208_90KANJI },
	/* 0x3452($@4R(B) -> 0x6B5D(&@$Bk](B) */
	{ "4R", NULL, "k]", JISX0208_90KANJI },
	/* 0x6B5D($@k](B) -> 0x3452(&@$B4R(B) */
	{ "k]", NULL, "4R", JISX0208_90KANJI },
	/* 0x375B($@7[(B) -> 0x7074(&@$Bpt(B) */
	{ "7[", NULL, "pt", JISX0208_90KANJI },
	/* 0x7074($@pt(B) -> 0x375B(&@$B7[(B) */
	{ "pt", NULL, "7[", JISX0208_90KANJI },
	/* 0x395C($@9\(B) -> 0x6268(&@$Bbh(B) */
	{ "9\\", NULL, "bh", JISX0208_90KANJI },
	/* 0x6268($@bh(B) -> 0x395C(&@$B9\(B) */
	{ "bh", NULL, "9\\", JISX0208_90KANJI },
	/* 0x3C49($@<I(B) -> 0x6922(&@$Bi"(B) */
	{ "<I", NULL, "i\"", JISX0208_90KANJI },
	/* 0x6922($@i"(B) -> 0x3C49(&@$B<I(B) */
	{ "i\"", NULL, "<I", JISX0208_90KANJI },
	/* 0x3F59($@?Y(B) -> 0x7057(&@$BpW(B) */
	{ "?Y", NULL, "pW", JISX0208_90KANJI },
	/* 0x7057($@pW(B) -> 0x3F59(&@$B?Y(B) */
	{ "pW", NULL, "?Y", JISX0208_90KANJI },
	/* 0x4128($@A((B) -> 0x6C4D(&@$BlM(B) */
	{ "A(", NULL, "lM", JISX0208_90KANJI },
	/* 0x6C4D($@lM(B) -> 0x4128(&@$BA((B) */
	{ "lM", NULL, "A(", JISX0208_90KANJI },
	/* 0x445B($@D[(B) -> 0x5464(&@$BTd(B) */
	{ "D[", NULL, "Td", JISX0208_90KANJI },
	/* 0x5464($@Td(B) -> 0x445B(&@$BD[(B) */
	{ "Td", NULL, "D[", JISX0208_90KANJI },
	/* 0x4557($@EW(B) -> 0x626A(&@$Bbj(B) */
	{ "EW", NULL, "bj", JISX0208_90KANJI },
	/* 0x626A($@bj(B) -> 0x4557(&@$BEW(B) */
	{ "bj", NULL, "EW", JISX0208_90KANJI },
	/* 0x456E($@En(B) -> 0x5B6D(&@$B[m(B) */
	{ "En", NULL, "[m", JISX0208_90KANJI },
	/* 0x5B6D($@[m(B) -> 0x456E(&@$BEn(B) */
	{ "[m", NULL, "En", JISX0208_90KANJI },
	/* 0x4573($@Es(B) -> 0x5E39(&@$B^9(B) */
	{ "Es", NULL, "^9", JISX0208_90KANJI },
	/* 0x5E39($@^9(B) -> 0x4573(&@$BEs(B) */
	{ "^9", NULL, "Es", JISX0208_90KANJI },
	/* 0x4676($@Fv(B) -> 0x6D6E(&@$Bmn(B) */
	{ "Fv", NULL, "mn", JISX0208_90KANJI },
	/* 0x6D6E($@mn(B) -> 0x4676(&@$BFv(B) */
	{ "mn", NULL, "Fv", JISX0208_90KANJI },
	/* 0x4768($@Gh(B) -> 0x6A24(&@$Bj$(B) */
	{ "Gh", NULL, "j$", JISX0208_90KANJI },
	/* 0x6A24($@j$(B) -> 0x4768(&@$BGh(B) */
	{ "j$", NULL, "Gh", JISX0208_90KANJI },
	/* 0x4930($@I0(B) -> 0x5B58(&@$B[X(B) */
	{ "I0", NULL, "[X", JISX0208_90KANJI },
	/* 0x5B58($@[X(B) -> 0x4930(&@$BI0(B) */
	{ "[X", NULL, "I0", JISX0208_90KANJI },
	/* 0x4B79($@Ky(B) -> 0x5056(&@$BPV(B) */
	{ "Ky", NULL, "PV", JISX0208_90KANJI },
	/* 0x5056($@PV(B) -> 0x4B79(&@$BKy(B) */
	{ "PV", NULL, "Ky", JISX0208_90KANJI },
	/* 0x4C79($@Ly(B) -> 0x692E(&@$Bi.(B) */
	{ "Ly", NULL, "i.", JISX0208_90KANJI },
	/* 0x692E($@i.(B) -> 0x4C79(&@$BLy(B) */
	{ "i.", NULL, "Ly", JISX0208_90KANJI },
	/* 0x4F36($@O6(B) -> 0x6446(&@$BdF(B) */
	{ "O6", NULL, "dF", JISX0208_90KANJI },
	/* 0x6446($@dF(B) -> 0x4F36(&@$BO6(B) */
	{ "dF", NULL, "O6", JISX0208_90KANJI },
	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable ctable_jisx0208_78_90 = { conv_jisx0208_78_90, NULL };

static convtab unify_jisx0208[] = {
	/* 0x2121(&@$B!!(B) -> 0x20( ) */
	{ "!!", NULL, " ", ASCII },
	/* 0x2122(&@$B!"(B) -> 0x2C(,) */
	{ "!\"", NULL, ",", ASCII },
	/* 0x2123(&@$B!#(B) -> 0x2E(.) */
	{ "!#", NULL, ".", ASCII },
	/* 0x2124(&@$B!$(B) -> 0x2C(,) */
	{ "!$", NULL, ",", ASCII },
	/* 0x2125(&@$B!%(B) -> 0x2E(.) */
	{ "!%", NULL, ".", ASCII },
	/* 0x2127(&@$B!'(B) -> 0x3A(:) */
	{ "!'", NULL, ":", ASCII },
	/* 0x2128(&@$B!((B) -> 0x3B(;) */
	{ "!(", NULL, ";", ASCII },
	/* 0x2129(&@$B!)(B) -> 0x3F(?) */
	{ "!)", NULL, "?", ASCII },
	/* 0x212A(&@$B!*(B) -> 0x21(!) */
	{ "!*", NULL, "!", ASCII },
	/* 0x2130(&@$B!0(B) -> 0x5E(^) */
	{ "!0", NULL, "^", ASCII },
	/* 0x2132(&@$B!2(B) -> 0x5F(_) */
	{ "!2", NULL, "_", ASCII },
	/* 0x213D(&@$B!=(B) -> 0x2D(-) */
	{ "!=", NULL, "-", ASCII },
	/* 0x213E(&@$B!>(B) -> 0x2D(-) */
	{ "!>", NULL, "-", ASCII },
	/* 0x213F(&@$B!?(B) -> 0x2F(/) */
	{ "!?", NULL, "/", ASCII },
	/* 0x2140(&@$B!@(B) -> 0x5C(\) */
	{ "!@", NULL, "\\", ASCII },
	/* 0x2141(&@$B!A(B) -> 0x2D(-) */
	{ "!A", NULL, "-", ASCII },
	/* 0x2143(&@$B!C(B) -> 0x7C(|) */
	{ "!C", NULL, "|", ASCII },
	/* 0x2146(&@$B!F(B) -> 0x27(') */
	{ "!F", NULL, "'", ASCII },
	/* 0x2147(&@$B!G(B) -> 0x27(') */
	{ "!G", NULL, "'", ASCII },
	/* 0x2148(&@$B!H(B) -> 0x22(") */
	{ "!H", NULL, "\"", ASCII },
	/* 0x2149(&@$B!I(B) -> 0x22(") */
	{ "!I", NULL, "\"", ASCII },
	/* 0x214A(&@$B!J(B) -> 0x28(() */
	{ "!J", NULL, "(", ASCII },
	/* 0x214B(&@$B!K(B) -> 0x29()) */
	{ "!K", NULL, ")", ASCII },
	/* 0x214C(&@$B!L(B) -> 0x5B([) */
	{ "!L", NULL, "[", ASCII },
	/* 0x214D(&@$B!M(B) -> 0x5D(]) */
	{ "!M", NULL, "]", ASCII },
	/* 0x214E(&@$B!N(B) -> 0x5B([) */
	{ "!N", NULL, "[", ASCII },
	/* 0x214F(&@$B!O(B) -> 0x5D(]) */
	{ "!O", NULL, "]", ASCII },
	/* 0x2150(&@$B!P(B) -> 0x7B({) */
	{ "!P", NULL, "{", ASCII },
	/* 0x2151(&@$B!Q(B) -> 0x7D(}) */
	{ "!Q", NULL, "}", ASCII },
	/* 0x2152(&@$B!R(B) -> 0x5B([) */
	{ "!R", NULL, "[", ASCII },
	/* 0x2153(&@$B!S(B) -> 0x5D(]) */
	{ "!S", NULL, "]", ASCII },
	/* 0x2154(&@$B!T(B) -> 0x5B([) */
	{ "!T", NULL, "[", ASCII },
	/* 0x2155(&@$B!U(B) -> 0x5D(]) */
	{ "!U", NULL, "]", ASCII },
	/* 0x2156(&@$B!V(B) -> 0x5B([) */
	{ "!V", NULL, "[", ASCII },
	/* 0x2157(&@$B!W(B) -> 0x5D(]) */
	{ "!W", NULL, "]", ASCII },
	/* 0x2158(&@$B!X(B) -> 0x5B([) */
	{ "!X", NULL, "[", ASCII },
	/* 0x2159(&@$B!Y(B) -> 0x5D(]) */
	{ "!Y", NULL, "]", ASCII },
	/* 0x215A(&@$B!Z(B) -> 0x5B([) */
	{ "!Z", NULL, "[", ASCII },
	/* 0x215B(&@$B![(B) -> 0x5D(]) */
	{ "![", NULL, "]", ASCII },
	/* 0x215C(&@$B!\(B) -> 0x2B(+) */
	{ "!\\", NULL, "+", ASCII },
	/* 0x215D(&@$B!](B) -> 0x2D(-) */
	{ "!]", NULL, "-", ASCII },
	/* 0x215F(&@$B!_(B) -> 0x2A(*) */
	{ "!_", NULL, "*", ASCII },
	/* 0x2160(&@$B!`(B) -> 0x2F(/) */
	{ "!`", NULL, "/", ASCII },
	/* 0x2161(&@$B!a(B) -> 0x3D(=) */
	{ "!a", NULL, "=", ASCII },
	/* 0x2163(&@$B!c(B) -> 0x3C(<) */
	{ "!c", NULL, "<", ASCII },
	/* 0x2164(&@$B!d(B) -> 0x3E(>) */
	{ "!d", NULL, ">", ASCII },
	/* 0x216C(&@$B!l(B) -> 0x27(') */
	{ "!l", NULL, "'", ASCII },
	/* 0x216D(&@$B!m(B) -> 0x22(") */
	{ "!m", NULL, "\"", ASCII },
	/* 0x2170(&@$B!p(B) -> 0x24($) */
	{ "!p", NULL, "$", ASCII },
	/* 0x2173(&@$B!s(B) -> 0x25(%) */
	{ "!s", NULL, "%", ASCII },
	/* 0x2174(&@$B!t(B) -> 0x23(#) */
	{ "!t", NULL, "#", ASCII },
	/* 0x2175(&@$B!u(B) -> 0x26(&) */
	{ "!u", NULL, "&", ASCII },
	/* 0x2176(&@$B!v(B) -> 0x2A(*) */
	{ "!v", NULL, "*", ASCII },
	/* 0x2177(&@$B!w(B) -> 0x40(@) */
	{ "!w", NULL, "@", ASCII },
	/* 0x2330(&@$B#0(B) -> 0x30(0) */
	{ "#0", NULL, "0", ASCII },
	/* 0x2331(&@$B#1(B) -> 0x31(1) */
	{ "#1", NULL, "1", ASCII },
	/* 0x2332(&@$B#2(B) -> 0x32(2) */
	{ "#2", NULL, "2", ASCII },
	/* 0x2333(&@$B#3(B) -> 0x33(3) */
	{ "#3", NULL, "3", ASCII },
	/* 0x2334(&@$B#4(B) -> 0x34(4) */
	{ "#4", NULL, "4", ASCII },
	/* 0x2335(&@$B#5(B) -> 0x35(5) */
	{ "#5", NULL, "5", ASCII },
	/* 0x2336(&@$B#6(B) -> 0x36(6) */
	{ "#6", NULL, "6", ASCII },
	/* 0x2337(&@$B#7(B) -> 0x37(7) */
	{ "#7", NULL, "7", ASCII },
	/* 0x2338(&@$B#8(B) -> 0x38(8) */
	{ "#8", NULL, "8", ASCII },
	/* 0x2339(&@$B#9(B) -> 0x39(9) */
	{ "#9", NULL, "9", ASCII },
	/* 0x2341(&@$B#A(B) -> 0x41(A) */
	{ "#A", NULL, "A", ASCII },
	/* 0x2342(&@$B#B(B) -> 0x42(B) */
	{ "#B", NULL, "B", ASCII },
	/* 0x2343(&@$B#C(B) -> 0x43(C) */
	{ "#C", NULL, "C", ASCII },
	/* 0x2344(&@$B#D(B) -> 0x44(D) */
	{ "#D", NULL, "D", ASCII },
	/* 0x2345(&@$B#E(B) -> 0x45(E) */
	{ "#E", NULL, "E", ASCII },
	/* 0x2346(&@$B#F(B) -> 0x46(F) */
	{ "#F", NULL, "F", ASCII },
	/* 0x2347(&@$B#G(B) -> 0x47(G) */
	{ "#G", NULL, "G", ASCII },
	/* 0x2348(&@$B#H(B) -> 0x48(H) */
	{ "#H", NULL, "H", ASCII },
	/* 0x2349(&@$B#I(B) -> 0x49(I) */
	{ "#I", NULL, "I", ASCII },
	/* 0x234A(&@$B#J(B) -> 0x4A(J) */
	{ "#J", NULL, "J", ASCII },
	/* 0x234B(&@$B#K(B) -> 0x4B(K) */
	{ "#K", NULL, "K", ASCII },
	/* 0x234C(&@$B#L(B) -> 0x4C(L) */
	{ "#L", NULL, "L", ASCII },
	/* 0x234D(&@$B#M(B) -> 0x4D(M) */
	{ "#M", NULL, "M", ASCII },
	/* 0x234E(&@$B#N(B) -> 0x4E(N) */
	{ "#N", NULL, "N", ASCII },
	/* 0x234F(&@$B#O(B) -> 0x4F(O) */
	{ "#O", NULL, "O", ASCII },
	/* 0x2350(&@$B#P(B) -> 0x50(P) */
	{ "#P", NULL, "P", ASCII },
	/* 0x2351(&@$B#Q(B) -> 0x51(Q) */
	{ "#Q", NULL, "Q", ASCII },
	/* 0x2352(&@$B#R(B) -> 0x52(R) */
	{ "#R", NULL, "R", ASCII },
	/* 0x2353(&@$B#S(B) -> 0x53(S) */
	{ "#S", NULL, "S", ASCII },
	/* 0x2354(&@$B#T(B) -> 0x54(T) */
	{ "#T", NULL, "T", ASCII },
	/* 0x2355(&@$B#U(B) -> 0x55(U) */
	{ "#U", NULL, "U", ASCII },
	/* 0x2356(&@$B#V(B) -> 0x56(V) */
	{ "#V", NULL, "V", ASCII },
	/* 0x2357(&@$B#W(B) -> 0x57(W) */
	{ "#W", NULL, "W", ASCII },
	/* 0x2358(&@$B#X(B) -> 0x58(X) */
	{ "#X", NULL, "X", ASCII },
	/* 0x2359(&@$B#Y(B) -> 0x59(Y) */
	{ "#Y", NULL, "Y", ASCII },
	/* 0x235A(&@$B#Z(B) -> 0x5A(Z) */
	{ "#Z", NULL, "Z", ASCII },
	/* 0x2361(&@$B#a(B) -> 0x61(a) */
	{ "#a", NULL, "a", ASCII },
	/* 0x2362(&@$B#b(B) -> 0x62(b) */
	{ "#b", NULL, "b", ASCII },
	/* 0x2363(&@$B#c(B) -> 0x63(c) */
	{ "#c", NULL, "c", ASCII },
	/* 0x2364(&@$B#d(B) -> 0x64(d) */
	{ "#d", NULL, "d", ASCII },
	/* 0x2365(&@$B#e(B) -> 0x65(e) */
	{ "#e", NULL, "e", ASCII },
	/* 0x2366(&@$B#f(B) -> 0x66(f) */
	{ "#f", NULL, "f", ASCII },
	/* 0x2367(&@$B#g(B) -> 0x67(g) */
	{ "#g", NULL, "g", ASCII },
	/* 0x2368(&@$B#h(B) -> 0x68(h) */
	{ "#h", NULL, "h", ASCII },
	/* 0x2369(&@$B#i(B) -> 0x69(i) */
	{ "#i", NULL, "i", ASCII },
	/* 0x236A(&@$B#j(B) -> 0x6A(j) */
	{ "#j", NULL, "j", ASCII },
	/* 0x236B(&@$B#k(B) -> 0x6B(k) */
	{ "#k", NULL, "k", ASCII },
	/* 0x236C(&@$B#l(B) -> 0x6C(l) */
	{ "#l", NULL, "l", ASCII },
	/* 0x236D(&@$B#m(B) -> 0x6D(m) */
	{ "#m", NULL, "m", ASCII },
	/* 0x236E(&@$B#n(B) -> 0x6E(n) */
	{ "#n", NULL, "n", ASCII },
	/* 0x236F(&@$B#o(B) -> 0x6F(o) */
	{ "#o", NULL, "o", ASCII },
	/* 0x2370(&@$B#p(B) -> 0x70(p) */
	{ "#p", NULL, "p", ASCII },
	/* 0x2371(&@$B#q(B) -> 0x71(q) */
	{ "#q", NULL, "q", ASCII },
	/* 0x2372(&@$B#r(B) -> 0x72(r) */
	{ "#r", NULL, "r", ASCII },
	/* 0x2373(&@$B#s(B) -> 0x73(s) */
	{ "#s", NULL, "s", ASCII },
	/* 0x2374(&@$B#t(B) -> 0x74(t) */
	{ "#t", NULL, "t", ASCII },
	/* 0x2375(&@$B#u(B) -> 0x75(u) */
	{ "#u", NULL, "u", ASCII },
	/* 0x2376(&@$B#v(B) -> 0x76(v) */
	{ "#v", NULL, "v", ASCII },
	/* 0x2377(&@$B#w(B) -> 0x77(w) */
	{ "#w", NULL, "w", ASCII },
	/* 0x2378(&@$B#x(B) -> 0x78(x) */
	{ "#x", NULL, "x", ASCII },
	/* 0x2379(&@$B#y(B) -> 0x79(y) */
	{ "#y", NULL, "y", ASCII },
	/* 0x237a(&@$B#z(B) -> 0x7A(z) */
	{ "#z", NULL, "z", ASCII },
	/* 0x2621(&@$B&!(B) -> 0x41(-FA) */
	{ "&!", NULL, "A", GREEK },
	/* 0x2622(&@$B&"(B) -> 0x42(-FB) */
	{ "&\"", NULL, "B", GREEK },
	/* 0x2623(&@$B&#(B) -> 0x43(-FC) */
	{ "&#", NULL, "C", GREEK },
	/* 0x2624(&@$B&$(B) -> 0x44(-FD) */
	{ "&$", NULL, "D", GREEK },
	/* 0x2625(&@$B&%(B) -> 0x45(-FE) */
	{ "&%", NULL, "E", GREEK },
	/* 0x2626(&@$B&&(B) -> 0x46(-FF) */
	{ "&&", NULL, "F", GREEK },
	/* 0x2627(&@$B&'(B) -> 0x47(-FG) */
	{ "&'", NULL, "G", GREEK },
	/* 0x2628(&@$B&((B) -> 0x48(-FH) */
	{ "&(", NULL, "H", GREEK },
	/* 0x2629(&@$B&)(B) -> 0x49(-FI) */
	{ "&)", NULL, "I", GREEK },
	/* 0x262A(&@$B&*(B) -> 0x4A(-FJ) */
	{ "&*", NULL, "J", GREEK },
	/* 0x262B(&@$B&+(B) -> 0x4B(-FK) */
	{ "&+", NULL, "K", GREEK },
	/* 0x262C(&@$B&,(B) -> 0x4C(-FL) */
	{ "&,", NULL, "L", GREEK },
	/* 0x262D(&@$B&-(B) -> 0x4D(-FM) */
	{ "&-", NULL, "M", GREEK },
	/* 0x262E(&@$B&.(B) -> 0x4E(-FN) */
	{ "&.", NULL, "N", GREEK },
	/* 0x262F(&@$B&/(B) -> 0x4F(-FO) */
	{ "&/", NULL, "O", GREEK },
	/* 0x2630(&@$B&0(B) -> 0x50(-FP) */
	{ "&0", NULL, "P", GREEK },
	/* 0x2631(&@$B&1(B) -> 0x51(-FQ) */
	{ "&1", NULL, "Q", GREEK },
	/* 0x2632(&@$B&2(B) -> 0x53(-FS) */
	{ "&2", NULL, "S", GREEK },
	/* 0x2633(&@$B&3(B) -> 0x54(-FT) */
	{ "&3", NULL, "T", GREEK },
	/* 0x2634(&@$B&4(B) -> 0x55(-FU) */
	{ "&4", NULL, "U", GREEK },
	/* 0x2635(&@$B&5(B) -> 0x56(-FV) */
	{ "&5", NULL, "V", GREEK },
	/* 0x2636(&@$B&6(B) -> 0x57(-FW) */
	{ "&6", NULL, "W", GREEK },
	/* 0x2637(&@$B&7(B) -> 0x58(-FX) */
	{ "&7", NULL, "X", GREEK },
	/* 0x2638(&@$B&8(B) -> 0x59(-FY) */
	{ "&8", NULL, "Y", GREEK },
	/* 0x2641(&@$B&A(B) -> 0x61(-Fa) */
	{ "&A", NULL, "a", GREEK },
	/* 0x2642(&@$B&B(B) -> 0x62(-Fb) */
	{ "&B", NULL, "b", GREEK },
	/* 0x2643(&@$B&C(B) -> 0x63(-Fc) */
	{ "&C", NULL, "c", GREEK },
	/* 0x2644(&@$B&D(B) -> 0x64(-Fd) */
	{ "&D", NULL, "d", GREEK },
	/* 0x2645(&@$B&E(B) -> 0x65(-Fe) */
	{ "&E", NULL, "e", GREEK },
	/* 0x2646(&@$B&F(B) -> 0x66(-Ff) */
	{ "&F", NULL, "f", GREEK },
	/* 0x2647(&@$B&G(B) -> 0x67(-Fg) */
	{ "&G", NULL, "g", GREEK },
	/* 0x2648(&@$B&H(B) -> 0x68(-Fh) */
	{ "&H", NULL, "h", GREEK },
	/* 0x2649(&@$B&I(B) -> 0x69(-Fi) */
	{ "&I", NULL, "i", GREEK },
	/* 0x264A(&@$B&J(B) -> 0x6A(-Fj) */
	{ "&J", NULL, "j", GREEK },
	/* 0x264B(&@$B&K(B) -> 0x6B(-Fk) */
	{ "&K", NULL, "k", GREEK },
	/* 0x264C(&@$B&L(B) -> 0x6C(-Fl) */
	{ "&L", NULL, "l", GREEK },
	/* 0x264D(&@$B&M(B) -> 0x6D(-Fm) */
	{ "&M", NULL, "m", GREEK },
	/* 0x264E(&@$B&N(B) -> 0x6E(-Fn) */
	{ "&N", NULL, "n", GREEK },
	/* 0x264F(&@$B&O(B) -> 0x6F(-Fo) */
	{ "&O", NULL, "o", GREEK },
	/* 0x2650(&@$B&P(B) -> 0x70(-Fp) */
	{ "&P", NULL, "p", GREEK },
	/* 0x2651(&@$B&Q(B) -> 0x71(-Fq) */
	{ "&Q", NULL, "q", GREEK },
	/* 0x2652(&@$B&R(B) -> 0x73(-Fs) */
	{ "&R", NULL, "s", GREEK },
	/* 0x2653(&@$B&S(B) -> 0x74(-Ft) */
	{ "&S", NULL, "t", GREEK },
	/* 0x2654(&@$B&T(B) -> 0x75(-Fu) */
	{ "&T", NULL, "u", GREEK },
	/* 0x2655(&@$B&U(B) -> 0x76(-Fv) */
	{ "&U", NULL, "v", GREEK },
	/* 0x2656(&@$B&V(B) -> 0x77(-Fw) */
	{ "&V", NULL, "w", GREEK },
	/* 0x2657(&@$B&W(B) -> 0x78(-Fx) */
	{ "&W", NULL, "x", GREEK },
	/* 0x2658(&@$B&X(B) -> 0x79(-Fy) */
	{ "&X", NULL, "y", GREEK },
	/* 0x2721(&@$B'!(B) -> 0x30(-L0) */
	{ "'!", NULL, "0", CYRILLIC },
	/* 0x2722(&@$B'"(B) -> 0x31(-L1) */
	{ "'\"", NULL, "1", CYRILLIC },
	/* 0x2723(&@$B'#(B) -> 0x32(-L2) */
	{ "'#", NULL, "2", CYRILLIC },
	/* 0x2724(&@$B'$(B) -> 0x33(-L3) */
	{ "'$", NULL, "3", CYRILLIC },
	/* 0x2725(&@$B'%(B) -> 0x34(-L4) */
	{ "'%", NULL, "4", CYRILLIC },
	/* 0x2726(&@$B'&(B) -> 0x35(-L5) */
	{ "'&", NULL, "5", CYRILLIC },
	/* 0x2727(&@$B''(B) -> 0x21(-L!) */
	{ "''", NULL, "!", CYRILLIC },
	/* 0x2728(&@$B'((B) -> 0x36(-L6) */
	{ "'(", NULL, "6", CYRILLIC },
	/* 0x2729(&@$B')(B) -> 0x37(-L7) */
	{ "')", NULL, "7", CYRILLIC },
	/* 0x272A(&@$B'*(B) -> 0x38(-L8) */
	{ "'*", NULL, "8", CYRILLIC },
	/* 0x272B(&@$B'+(B) -> 0x39(-L9) */
	{ "'+", NULL, "9", CYRILLIC },
	/* 0x272C(&@$B',(B) -> 0x3A(-L:) */
	{ "',", NULL, ":", CYRILLIC },
	/* 0x272D(&@$B'-(B) -> 0x3B(-L;) */
	{ "'-", NULL, ";", CYRILLIC },
	/* 0x272E(&@$B'.(B) -> 0x3C(-L<) */
	{ "'.", NULL, "<", CYRILLIC },
	/* 0x272F(&@$B'/(B) -> 0x3D(-L=) */
	{ "'/", NULL, "=", CYRILLIC },
	/* 0x2730(&@$B'0(B) -> 0x3E(-L>) */
	{ "'0", NULL, ">", CYRILLIC },
	/* 0x2731(&@$B'1(B) -> 0x3F(-L?) */
	{ "'1", NULL, "?", CYRILLIC },
	/* 0x2732(&@$B'2(B) -> 0x40(-L@) */
	{ "'2", NULL, "@", CYRILLIC },
	/* 0x2733(&@$B'3(B) -> 0x41(-LA) */
	{ "'3", NULL, "A", CYRILLIC },
	/* 0x2734(&@$B'4(B) -> 0x42(-LB) */
	{ "'4", NULL, "B", CYRILLIC },
	/* 0x2735(&@$B'5(B) -> 0x43(-LC) */
	{ "'5", NULL, "C", CYRILLIC },
	/* 0x2736(&@$B'6(B) -> 0x44(-LD) */
	{ "'6", NULL, "D", CYRILLIC },
	/* 0x2737(&@$B'7(B) -> 0x45(-LE) */
	{ "'7", NULL, "E", CYRILLIC },
	/* 0x2738(&@$B'8(B) -> 0x46(-LF) */
	{ "'8", NULL, "F", CYRILLIC },
	/* 0x2739(&@$B'9(B) -> 0x47(-LG) */
	{ "'9", NULL, "G", CYRILLIC },
	/* 0x273A(&@$B':(B) -> 0x48(-LH) */
	{ "':", NULL, "H", CYRILLIC },
	/* 0x273B(&@$B';(B) -> 0x49(-LI) */
	{ "';", NULL, "I", CYRILLIC },
	/* 0x273C(&@$B'<(B) -> 0x4A(-LJ) */
	{ "'<", NULL, "J", CYRILLIC },
	/* 0x273D(&@$B'=(B) -> 0x4B(-LK) */
	{ "'=", NULL, "K", CYRILLIC },
	/* 0x273E(&@$B'>(B) -> 0x4C(-LL) */
	{ "'>", NULL, "L", CYRILLIC },
	/* 0x273F(&@$B'?(B) -> 0x4D(-LM) */
	{ "'?", NULL, "M", CYRILLIC },
	/* 0x2740(&@$B'@(B) -> 0x4E(-LN) */
	{ "'@", NULL, "N", CYRILLIC },
	/* 0x2741(&@$B'A(B) -> 0x4F(-LO) */
	{ "'A", NULL, "O", CYRILLIC },
	/* 0x2751(&@$B'Q(B) -> 0x50(-LP) */
	{ "'Q", NULL, "P", CYRILLIC },
	/* 0x2752(&@$B'R(B) -> 0x51(-LQ) */
	{ "'R", NULL, "Q", CYRILLIC },
	/* 0x2753(&@$B'S(B) -> 0x52(-LR) */
	{ "'S", NULL, "R", CYRILLIC },
	/* 0x2754(&@$B'T(B) -> 0x53(-LS) */
	{ "'T", NULL, "S", CYRILLIC },
	/* 0x2755(&@$B'U(B) -> 0x54(-LT) */
	{ "'U", NULL, "T", CYRILLIC },
	/* 0x2756(&@$B'V(B) -> 0x55(-LU) */
	{ "'V", NULL, "U", CYRILLIC },
	/* 0x2757(&@$B'W(B) -> 0x71(-Lq) */
	{ "'W", NULL, "q", CYRILLIC },
	/* 0x2758(&@$B'X(B) -> 0x56(-LV) */
	{ "'X", NULL, "V", CYRILLIC },
	/* 0x2759(&@$B'Y(B) -> 0x57(-LW) */
	{ "'Y", NULL, "W", CYRILLIC },
	/* 0x275A(&@$B'Z(B) -> 0x58(-LX) */
	{ "'Z", NULL, "X", CYRILLIC },
	/* 0x275B(&@$B'[(B) -> 0x59(-LY) */
	{ "'[", NULL, "Y", CYRILLIC },
	/* 0x275C(&@$B'\(B) -> 0x5A(-LZ) */
	{ "'\\", NULL, "Z", CYRILLIC },
	/* 0x275D(&@$B'](B) -> 0x5B(-L[) */
	{ "']", NULL, "[", CYRILLIC },
	/* 0x275E(&@$B'^(B) -> 0x5C(-L\) */
	{ "'^", NULL, "\\", CYRILLIC },
	/* 0x275F(&@$B'_(B) -> 0x5D(-L]) */
	{ "'_", NULL, "]", CYRILLIC },
	/* 0x2760(&@$B'`(B) -> 0x5E(-L^) */
	{ "'`", NULL, "^", CYRILLIC },
	/* 0x2761(&@$B'a(B) -> 0x5F(-L_) */
	{ "'a", NULL, "_", CYRILLIC },
	/* 0x2762(&@$B'b(B) -> 0x60(-L`) */
	{ "'b", NULL, "`", CYRILLIC },
	/* 0x2763(&@$B'c(B) -> 0x61(-La) */
	{ "'c", NULL, "a", CYRILLIC },
	/* 0x2764(&@$B'd(B) -> 0x62(-Lb) */
	{ "'d", NULL, "b", CYRILLIC },
	/* 0x2765(&@$B'e(B) -> 0x63(-Lc) */
	{ "'e", NULL, "c", CYRILLIC },
	/* 0x2766(&@$B'f(B) -> 0x64(-Ld) */
	{ "'f", NULL, "d", CYRILLIC },
	/* 0x2767(&@$B'g(B) -> 0x65(-Le) */
	{ "'g", NULL, "e", CYRILLIC },
	/* 0x2768(&@$B'h(B) -> 0x66(-Lf) */
	{ "'h", NULL, "f", CYRILLIC },
	/* 0x2769(&@$B'i(B) -> 0x67(-Lg) */
	{ "'i", NULL, "g", CYRILLIC },
	/* 0x276A(&@$B'j(B) -> 0x68(-Lh) */
	{ "'j", NULL, "h", CYRILLIC },
	/* 0x276B(&@$B'k(B) -> 0x69(-Li) */
	{ "'k", NULL, "i", CYRILLIC },
	/* 0x276C(&@$B'l(B) -> 0x6A(-Lj) */
	{ "'l", NULL, "j", CYRILLIC },
	/* 0x276D(&@$B'm(B) -> 0x6B(-Lk) */
	{ "'m", NULL, "k", CYRILLIC },
	/* 0x276E(&@$B'n(B) -> 0x6C(-Ll) */
	{ "'n", NULL, "l", CYRILLIC },
	/* 0x276F(&@$B'o(B) -> 0x6D(-Lm) */
	{ "'o", NULL, "m", CYRILLIC },
	/* 0x2770(&@$B'p(B) -> 0x6E(-Ln) */
	{ "'p", NULL, "n", CYRILLIC },
	/* 0x2771(&@$B'q(B) -> 0x6F(-Lo) */
	{ "'q", NULL, "o", CYRILLIC },
	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable utable_jisx0208 = { unify_jisx0208, NULL };

static convtab unify_n_jisx0201roman[] = {
	/* 0x5C((J\(B) -X 0x5C(\) */
	{ "\\", NULL, "\\", ASCII },
	/* 0x7E((J~(B) -X 0x7E(~) */
	{ "~", NULL, "~", ASCII },
	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable utable_n_jisx0201roman = { unify_n_jisx0201roman, NULL };

static convtab unify_n_iso646[] = {
	/* 0x23((@#(B) -X 0x23(#) */
	{ "#", NULL, "#", ASCII },
	/* 0x24((@$(B) -X 0x24($) */
	{ "$", NULL, "$", ASCII },
	/* 0x40((@@(B) -X 0x40(@) */
	{ "@", NULL, "@", ASCII },
	/* 0x5B((@[(B) -X 0x5B([) */
	{ "[", NULL, "[", ASCII },
	/* 0x5C((@\(B) -X 0x5C(\) */
	{ "\\", NULL, "\\", ASCII },
	/* 0x5D((@](B) -X 0x5D(]) */
	{ "]", NULL, "]", ASCII },
	/* 0x5E((@^(B) -X 0x5E(^) */
	{ "^", NULL, "^", ASCII },
	/* 0x60((@`(B) -X 0x60(`) */
	{ "`", NULL, "`", ASCII },
	/* 0x7B((@{(B) -X 0x7B({) */
	{ "{", NULL, "{", ASCII },
	/* 0x7C((@|(B) -X 0x7C(|) */
	{ "|", NULL, "|", ASCII },
	/* 0x7D((@}(B) -X 0x7D(}) */
	{ "}", NULL, "}", ASCII },
	/* 0x7E((@~(B) -X 0x7E(~) */
	{ "~", NULL, "~", ASCII },
	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable utable_n_iso646 = { unify_n_iso646, NULL };

static convtab eliminate_wrong_jisx0208_78[] = {
	/* empty rows */
	/* 8-15 KU 0x2821($@(!(B)-0x2F7E($@/~(B) -> 0x222E($B".(B) */
	{ "(!", "/~", "\".", JISX0208KANJI },
	/* 84-94 KU 0x7421($@t!(B)-0x7E7E($@~~(B) -> 0x222E($B".(B) */
	{ "t!", "~~", "\".", JISX0208KANJI },

	/* sequences of empty columns */
	/* 2 KU 0x222F($@"/(B)-0x227E($@"~(B) -> 0x222E($B".(B) */
	{ "\"/", "\"~", "\".", JISX0208KANJI },
	/* 3 KU 0x2321($@#!(B)-0x232F($@#/(B) -> 0x222E($B".(B) */
	{ "#!", "#/", "\".", JISX0208KANJI },
	/* 3 KU 0x233A($@#:(B)-0x2340($@#@(B) -> 0x222E($B".(B) */
	{ "#:", "#@", "\".", JISX0208KANJI },
	/* 3 KU 0x235B($@#[(B)-0x2360($@#`(B) -> 0x222E($B".(B) */
	{ "#[", "#`", "\".", JISX0208KANJI },
	/* 3 KU 0x237B($@#{(B)-0x237E($@#~(B) -> 0x222E($B".(B) */
	{ "#{", "#~", "\".", JISX0208KANJI },
	/* 4 KU 0x2474($@$t(B)-0x247E($@$~(B) -> 0x222E($B".(B) */
	{ "$t", "$~", "\".", JISX0208KANJI },
	/* 5 KU 0x2577($@%w(B)-0x257E($@%~(B) -> 0x222E($B".(B) */
	{ "%w", "%~", "\".", JISX0208KANJI },
	/* 6 KU 0x2639($@&9(B)-0x2640($@&@(B) -> 0x222E($B".(B) */
	{ "&9", "&@", "\".", JISX0208KANJI },
	/* 6 KU 0x2659($@&Y(B)-0x267E($@&~(B) -> 0x222E($B".(B) */
	{ "&Y", "&~", "\".", JISX0208KANJI },
	/* 7 KU 0x2742($@'B(B)-0x2750($@'P(B) -> 0x222E($B".(B) */
	{ "'B", "'P", "\".", JISX0208KANJI },
	/* 7 KU 0x2772($@'r(B)-0x277E($@'~(B) -> 0x222E($B".(B) */
	{ "'r", "'~", "\".", JISX0208KANJI },
	/* 47 KU 0x4F54($@OT(B)-0x4F7E($@O~(B) -> 0x222E($B".(B) */
	{ "OT", "O~", "\".", JISX0208KANJI },

	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable etable_jisx0208_78 = { eliminate_wrong_jisx0208_78, NULL };

static convtab eliminate_wrong_jisx0208_83[] = {
	/* empty rows */
	/* 9-15 KU 0x2921($B)!(B)-0x2F7E($B/~(B) -> 0x222E($B".(B) */
	{ ")!", "/~", "\".", JISX0208KANJI },
	/* 85-94 KU 0x7521($Bu!(B)-0x7E7E($B~~(B) -> 0x222E($B".(B) */
	{ "u!", "~~", "\".", JISX0208KANJI },

	/* sequences of empty columns */
	/* 2 KU 0x222F($B"/(B)-0x2239($B"9(B) -> 0x222E($B".(B) */
	{ "\"/", "\"9", "\".", JISX0208KANJI },
	/* 2 KU 0x2242($B"B(B)-0x2249($B"I(B) -> 0x222E($B".(B) */
	{ "\"B", "\"I", "\".", JISX0208KANJI },
	/* 2 KU 0x2251($B"Q(B)-0x225B($B"[(B) -> 0x222E($B".(B) */
	{ "\"Q", "\"[", "\".", JISX0208KANJI },
	/* 2 KU 0x226B($B"k(B)-0x2271($B"q(B) -> 0x222E($B".(B) */
	{ "\"k", "\"q", "\".", JISX0208KANJI },
	/* 2 KU 0x227A($B"z(B)-0x227D($B"}(B) -> 0x222E($B".(B) */
	{ "\"z", "\"}", "\".", JISX0208KANJI },
	/* 3 KU 0x2321($B#!(B)-0x232F($B#/(B) -> 0x222E($B".(B) */
	{ "#!", "#/", "\".", JISX0208KANJI },
	/* 3 KU 0x233A($B#:(B)-0x2340($B#@(B) -> 0x222E($B".(B) */
	{ "#:", "#@", "\".", JISX0208KANJI },
	/* 3 KU 0x235B($B#[(B)-0x2360($B#`(B) -> 0x222E($B".(B) */
	{ "#[", "#`", "\".", JISX0208KANJI },
	/* 3 KU 0x237B($B#{(B)-0x237E($B#~(B) -> 0x222E($B".(B) */
	{ "#{", "#~", "\".", JISX0208KANJI },
	/* 4 KU 0x2474($B$t(B)-0x247E($B$~(B) -> 0x222E($B".(B) */
	{ "$t", "$~", "\".", JISX0208KANJI },
	/* 5 KU 0x2577($B%w(B)-0x257E($B%~(B) -> 0x222E($B".(B) */
	{ "%w", "%~", "\".", JISX0208KANJI },
	/* 6 KU 0x2639($B&9(B)-0x2640($B&@(B) -> 0x222E($B".(B) */
	{ "&9", "&@", "\".", JISX0208KANJI },
	/* 6 KU 0x2659($B&Y(B)-0x267E($B&~(B) -> 0x222E($B".(B) */
	{ "&Y", "&~", "\".", JISX0208KANJI },
	/* 7 KU 0x2742($B'B(B)-0x2750($B'P(B) -> 0x222E($B".(B) */
	{ "'B", "'P", "\".", JISX0208KANJI },
	/* 7 KU 0x2772($B'r(B)-0x277E($B'~(B) -> 0x222E($B".(B) */
	{ "'r", "'~", "\".", JISX0208KANJI },
	/* 8 KU 0x2841($B(A(B)-0x287E($B(~(B) -> 0x222E($B".(B) */
	{ "(A", "(~", "\".", JISX0208KANJI },
	/* 47 KU 0x4F54($BOT(B)-0x4F7E($BO~(B) -> 0x222E($B".(B) */
	{ "OT", "O~", "\".", JISX0208KANJI },
	/* 84 KU 0x7425($Bt%(B)-0x747E($Bt~(B) -> 0x222E($B".(B) */
	{ "t%", "t~", "\".", JISX0208KANJI },

	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable etable_jisx0208_83 = { eliminate_wrong_jisx0208_83, NULL };

static convtab eliminate_wrong_jisx0208_90[] = {
	/* empty rows */
	/* 9-15 KU 0x2921(&@$B)!(B)-0x2F7E(&@$B/~(B) -> 0x222E($B".(B) */
	{ ")!", "/~", "\".", JISX0208KANJI },
	/* 85-94 KU 0x7521(&@$Bu!(B)-0x7E7E(&@$B~~(B) -> 0x222E($B".(B) */
	{ "u!", "~~", "\".", JISX0208KANJI },

	/* sequences of empty columns */
	/* 2 KU 0x222F(&@$B"/(B)-0x2239(&@$B"9(B) -> 0x222E($B".(B) */
	{ "\"/", "\"9", "\".", JISX0208KANJI },
	/* 2 KU 0x2242(&@$B"B(B)-0x2249(&@$B"I(B) -> 0x222E($B".(B) */
	{ "\"B", "\"I", "\".", JISX0208KANJI },
	/* 2 KU 0x2251(&@$B"Q(B)-0x225B(&@$B"[(B) -> 0x222E($B".(B) */
	{ "\"Q", "\"[", "\".", JISX0208KANJI },
	/* 2 KU 0x226B(&@$B"k(B)-0x2271(&@$B"q(B) -> 0x222E($B".(B) */
	{ "\"k", "\"q", "\".", JISX0208KANJI },
	/* 2 KU 0x227A(&@$B"z(B)-0x227D(&@$B"}(B) -> 0x222E($B".(B) */
	{ "\"z", "\"}", "\".", JISX0208KANJI },
	/* 3 KU 0x2321(&@$B#!(B)-0x232F(&@$B#/(B) -> 0x222E($B".(B) */
	{ "#!", "#/", "\".", JISX0208KANJI },
	/* 3 KU 0x233A(&@$B#:(B)-0x2340(&@$B#@(B) -> 0x222E($B".(B) */
	{ "#:", "#@", "\".", JISX0208KANJI },
	/* 3 KU 0x235B(&@$B#[(B)-0x2360(&@$B#`(B) -> 0x222E($B".(B) */
	{ "#[", "#`", "\".", JISX0208KANJI },
	/* 3 KU 0x237B(&@$B#{(B)-0x237E(&@$B#~(B) -> 0x222E($B".(B) */
	{ "#{", "#~", "\".", JISX0208KANJI },
	/* 4 KU 0x2474(&@$B$t(B)-0x247E(&@$B$~(B) -> 0x222E($B".(B) */
	{ "$t", "$~", "\".", JISX0208KANJI },
	/* 5 KU 0x2577(&@$B%w(B)-0x257E(&@$B%~(B) -> 0x222E($B".(B) */
	{ "%w", "%~", "\".", JISX0208KANJI },
	/* 6 KU 0x2639(&@$B&9(B)-0x2640(&@$B&@(B) -> 0x222E($B".(B) */
	{ "&9", "&@", "\".", JISX0208KANJI },
	/* 6 KU 0x2659(&@$B&Y(B)-0x267E(&@$B&~(B) -> 0x222E($B".(B) */
	{ "&Y", "&~", "\".", JISX0208KANJI },
	/* 7 KU 0x2742(&@$B'B(B)-0x2750(&@$B'P(B) -> 0x222E($B".(B) */
	{ "'B", "'P", "\".", JISX0208KANJI },
	/* 7 KU 0x2772(&@$B'r(B)-0x277E(&@$B'~(B) -> 0x222E($B".(B) */
	{ "'r", "'~", "\".", JISX0208KANJI },
	/* 8 KU 0x2841(&@$B(A(B)-0x287E(&@$B(~(B) -> 0x222E($B".(B) */
	{ "(A", "(~", "\".", JISX0208KANJI },
	/* 47 KU 0x4F54(&@$BOT(B)-0x4F7E(&@$BO~(B) -> 0x222E($B".(B) */
	{ "OT", "O~", "\".", JISX0208KANJI },
	/* 84 KU 0x7427(&@$Bt'(B)-0x747E(&@$Bt~(B) -> 0x222E($B".(B) */
	{ "t'", "t~", "\".", JISX0208KANJI },

	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable etable_jisx0208_90 = { eliminate_wrong_jisx0208_90, NULL };

static convtab eliminate_wrong_jisx0212[] = {
	/* empty rows */
	/* 1 KU 0x2121($(D!!(B)-0x217E($(D!~(B) -> 0x222E($B".(B) */
	{ "!!", "!~", "\".", JISX0208KANJI },
	/* 3-5 KU 0x2321($(D#!(B)-0x257E($(D%~(B) -> 0x222E($B".(B) */
	{ "#!", "%~", "\".", JISX0208KANJI },
	/* 8 KU 0x2821($(D(!(B)-0x287E($(D(~(B) -> 0x222E($B".(B) */
	{ "(!", "(~", "\".", JISX0208KANJI },
	/* 12-15 KU 0x2C21($(D,!(B)-0x2F7E($(D/~(B) -> 0x222E($B".(B) */
	{ ",!", "/~", "\".", JISX0208KANJI },
	/* 78-94 KU 0x6E21($(Dn!(B)-0x7E7E($(D~~(B) -> 0x222E($B".(B) */
	{ "n!", "~~", "\".", JISX0208KANJI },

	/* sequences of empty columns */
	/* 2 KU 0x2221($(D"!(B)-0x222E($(D".(B) -> 0x222E($B".(B) */
	{ "\"!", "\".", "\".", JISX0208KANJI },
	/* 2 KU 0x223A($(D":(B)-0x2241($(D"A(B) -> 0x222E($B".(B) */
	{ "\":", "\"A", "\".", JISX0208KANJI },
	/* 2 KU 0x2245($(D"E(B)-0x226A($(D"j(B) -> 0x222E($B".(B) */
	{ "\"E", "\"j", "\".", JISX0208KANJI },
	/* 2 KU 0x2272($(D"r(B)-0x227E($(D"~(B) -> 0x222E($B".(B) */
	{ "\"r", "\"~", "\".", JISX0208KANJI },
	/* 6 KU 0x2621($(D&!(B)-0x2660($(D&`(B) -> 0x222E($B".(B) */
	{ "&!", "&`", "\".", JISX0208KANJI },
	/* 6 KU 0x2666($(D&f(B) -> 0x222E($B".(B) */
	{ "&f", NULL, "\".", JISX0208KANJI },
	/* 6 KU 0x2668($(D&h(B) -> 0x222E($B".(B) */
	{ "&h", NULL, "\".", JISX0208KANJI },
	/* 6 KU 0x266B($(D&k(B) -> 0x222E($B".(B) */
	{ "&k", NULL, "\".", JISX0208KANJI },
	/* 6 KU 0x266D($(D&m(B)-0x2670($(D&p(B) -> 0x222E($B".(B) */
	{ "&m", "&p", "\".", JISX0208KANJI },
	/* 6 KU 0x267D($(D&}(B)-0x267E($(D&~(B) -> 0x222E($B".(B) */
	{ "&}", "&~", "\".", JISX0208KANJI },
	/* 7 KU 0x2721($(D'!(B)-0x2741($(D'A(B) -> 0x222E($B".(B) */
	{ "'!", "'A", "\".", JISX0208KANJI },
	/* 7 KU 0x274F($(D'O(B)-0x2771($(D'q(B) -> 0x222E($B".(B) */
	{ "'O", "'q", "\".", JISX0208KANJI },
	/* 9 KU 0x2923($(D)#(B) -> 0x222E($B".(B) */
	{ ")#", NULL, "\".", JISX0208KANJI },
	/* 9 KU 0x2925($(D)%(B) -> 0x222E($B".(B) */
	{ ")%", NULL, "\".", JISX0208KANJI },
	/* 9 KU 0x2927($(D)'(B) -> 0x222E($B".(B) */
	{ ")'", NULL, "\".", JISX0208KANJI },
	/* 9 KU 0x292A($(D)*(B) -> 0x222E($B".(B) */
	{ ")*", NULL, "\".", JISX0208KANJI },
	/* 9 KU 0x292E($(D).(B) -> 0x222E($B".(B) */
	{ ").", NULL, "\".", JISX0208KANJI },
	/* 9 KU 0x2931($(D)1(B)-0x2940($(D)@(B) -> 0x222E($B".(B) */
	{ ")1", ")@", "\".", JISX0208KANJI },
	/* 9 KU 0x2951($(D)Q(B)-0x297E($(D)~(B) -> 0x222E($B".(B) */
	{ ")Q", ")~", "\".", JISX0208KANJI },
	/* 10 KU 0x2A39($(D*9(B) -> 0x222E($B".(B) */
	{ "*9", NULL, "\".", JISX0208KANJI },
	/* 10 KU 0x2A78($(D*x(B)-0x2A7E($(D*~(B) -> 0x222E($B".(B) */
	{ "*x", "*~", "\".", JISX0208KANJI },
	/* 11 KU 0x2B3C($(D+<(B) -> 0x222E($B".(B) */
	{ "+<", NULL, "\".", JISX0208KANJI },
	/* 11 KU 0x2B44($(D+D(B) -> 0x222E($B".(B) */
	{ "+D", NULL, "\".", JISX0208KANJI },
	/* 11 KU 0x2B78($(D+x(B)-0x2B7E($(D+~(B) -> 0x222E($B".(B) */
	{ "+x", "+~", "\".", JISX0208KANJI },
	/* 77 KU 0x6D64($(Dmd(B)-0x6D7E($(Dm~(B) -> 0x222E($B".(B) */
	{ "md", "m~", "\".", JISX0208KANJI },

	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable etable_jisx0212 = { eliminate_wrong_jisx0212, NULL };

static convtab eliminate_wrong_jisx0213_1[] = {
	/* no empty row */

	/* sequences of empty columns */
	/* 4 KU 0x247C($(O$|(B)-0x247E($(O$~(B) -> 0x222E($B".(B) */
	{ "$|", "$~", "\".", JISX0208KANJI },
	/* 8 KU 0x285F($(O(_(B)-0x2866($(O(f(B) -> 0x222E($B".(B) */
	{ "(_", "(f", "\".", JISX0208KANJI },
	/* 8 KU 0x287D($(O(}(B)-0x287E($(O(~(B) -> 0x222E($B".(B) */
	{ "(}", "(~", "\".", JISX0208KANJI },
	/* 12 KU 0x2C74($(O,t(B)-0x2C7C($(O,|(B) -> 0x222E($B".(B) */
	{ ",t", ",|", "\".", JISX0208KANJI },
	/* 13 KU 0x2D58($(O-X(B)-0x2D5E($(O-^(B) -> 0x222E($B".(B) */
	{ "-X", "-^", "\".", JISX0208KANJI },
	/* 13 KU 0x2D70($(O-p(B)-0x2D72($(O-r(B) -> 0x222E($B".(B) */
	{ "-p", "-r", "\".", JISX0208KANJI },
	/* 13 KU 0x2D74($(O-t(B)-0x2D77($(O-w(B) -> 0x222E($B".(B) */
	{ "-t", "-w", "\".", JISX0208KANJI },
	/* 13 KU 0x2D7A($(O-z(B)-0x2D7C($(O-|(B) -> 0x222E($B".(B) */
	{ "-z", "-|", "\".", JISX0208KANJI },
	/* 14 KU 0x2E21($(O.!(B) -> 0x222E($B".(B) */
	{ ".!", NULL, "\".", JISX0208KANJI },
	/* 15 KU 0x2F7E($(O/~(B) -> 0x222E($B".(B) */
	{ "/~", NULL, "\".", JISX0208KANJI },
	/* 47 KU 0x4F54($(OOT(B) -> 0x222E($B".(B) */
	{ "OT", NULL, "\".", JISX0208KANJI },
	/* 47 KU 0x4F7E($(OO~(B) -> 0x222E($B".(B) */
	{ "O~", NULL, "\".", JISX0208KANJI },
	/* 84 KU 0x7427($(Ot'(B) -> 0x222E($B".(B) */
	{ "t'", NULL, "\".", JISX0208KANJI },
	/* 94 KU 0x7E7A($(O~z(B)-0x7E7E($(O~~(B) -> 0x222E($B".(B) */
	{ "~z", "~~", "\".", JISX0208KANJI },

	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable etable_jisx0213_1 = { eliminate_wrong_jisx0213_1, NULL };

static convtab eliminate_wrong_jisx0213_2[] = {
	/* empty rows */
	/* 2 KU 0x2221($(P"!(B)-0x227E($(P"~(B) -> 0x222E($B".(B) */
	{ "\"!", "\"~", "\".", JISX0208KANJI },
	/* 6-7 KU 0x2621($(P&!(B)-0x277E($(P'~(B) -> 0x222E($B".(B) */
	{ "&!", "'~", "\".", JISX0208KANJI },
	/* 9-11 KU 0x2921($(P)!(B)-0x2B7E($(P+~(B) -> 0x222E($B".(B) */
	{ ")!", "+~", "\".", JISX0208KANJI },
	/* 16-77 KU 0x3021($(P0!(B)-0x6D7E($(Pm~(B) -> 0x222E($B".(B) */
	{ "0!", "m~", "\".", JISX0208KANJI },

	/* sequences of empty columns */
	/* 94 KU 0x7E77($(P~w(B)-0x7E7E($(P~~(B) -> 0x222E($B".(B) */
	{ "~w", "~~", "\".", JISX0208KANJI },

	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable etable_jisx0213_2 = { eliminate_wrong_jisx0213_2, NULL };

static convtab eliminate_wrong_sjis[] = {
#if SJIS0213
	/* JIS X 0213:2000 plane 1 for SJIS0213 */

	/* no empty row */

	/* sequences of empty columns */
	/* 4 KU 0x82FA($(O$|(B)-0x82FC($(O$~(B) -> 0x222E($B".(B) */
	{ "\202\372", "\202\374", "\201\254", SJIS },
	/* 8 KU 0x84DD($(O(_(B)-0x84E4($(O(f(B) -> 0x222E($B".(B) */
	{ "\204\335", "\204\344", "\201\254", SJIS },
	/* 8 KU 0x84FB($(O(}(B)-0x84FC($(O(~(B) -> 0x222E($B".(B) */
	{ "\204\373", "\204\374", "\201\254", SJIS },
	/* 12 KU 0x86F2($(O,t(B)-0x86FA($(O,|(B) -> 0x222E($B".(B) */
	{ "\206\362", "\206\372", "\201\254", SJIS },
	/* 13 KU 0x8777($(O-X(B)-0x877D($(O-^(B) -> 0x222E($B".(B) */
	{ "\207\167", "\207\175", "\201\254", SJIS },
	/* 13 KU 0x8790($(O-p(B)-0x8792($(O-r(B) -> 0x222E($B".(B) */
	{ "\207\220", "\207\222", "\201\254", SJIS },
	/* 13 KU 0x8794($(O-t(B)-0x8797($(O-w(B) -> 0x222E($B".(B) */
	{ "\207\224", "\207\227", "\201\254", SJIS },
	/* 13 KU 0x879A($(O-z(B)-0x879C($(O-|(B) -> 0x222E($B".(B) */
	{ "\207\232", "\207\234", "\201\254", SJIS },
	/* 14 KU 0x879F($(O.!(B) -> 0x222E($B".(B) */
	{ "\207\237", NULL, "\201\254", SJIS },
	/* 15 KU 0x889E($(O/~(B) -> 0x222E($B".(B) */
	{ "\210\236", NULL, "\201\254", SJIS },
	/* 47 KU 0x9873($(OOT(B) -> 0x222E($B".(B) */
	{ "\230\163", NULL, "\201\254", SJIS },
	/* 47 KU 0x989E($(OO~(B) -> 0x222E($B".(B) */
	{ "\230\236", NULL, "\201\254", SJIS },
	/* 84 KU 0xEAA5($(Ot'(B) -> 0x222E($B".(B) */
	{ "\352\245", NULL, "\201\254", SJIS },
	/* 94 KU 0xEFF8($(O~z(B)-0xEFFC($(O~~(B) -> 0x222E($B".(B) */
	{ "\357\370", "\357\374", "\201\254", SJIS },

	/* JIS X 0213:2000 plane 2 for SJIS0213 */
	/* In SJIS0213, JIS X 0213:2000 occupies from 96 to 120 KU */

	/* no empty row */

	/* sequences of empty columns */
	/* 94 KU 0xFCF5($(P~w(B)-0xFCFC($(P~~(B) -> 0x222E($B".(B) */
	{ "\374\365", "\374\374", "\201\254", SJIS },
#else /* SJIS0213 */
	/* JIS X 0208:1990 for SJIS */
	/* 2 KU 0x81AD(&@$B"/(B)-0x81B7(&@$B"9(B) -> 0x81AC($B".(B) */
	{ "\201\255", "\201\267", "\201\254", SJIS },
	/* 2 KU 0x81C0(&@$B"B(B)-0x81C7(&@$B"I(B) -> 0x81AC($B".(B) */
	{ "\201\300", "\201\307", "\201\254", SJIS },
	/* 2 KU 0x81CF(&@$B"Q(B)-0x81D9(&@$B"[(B) -> 0x81AC($B".(B) */
	{ "\201\317", "\201\331", "\201\254", SJIS },
	/* 2 KU 0x81E9(&@$B"k(B)-0x81EF(&@$B"q(B) -> 0x81AC($B".(B) */
	{ "\201\351", "\201\357", "\201\254", SJIS },
	/* 2 KU 0x81F8(&@$B"z(B)-0x81FB(&@$B"}(B) -> 0x81AC($B".(B) */
	{ "\201\370", "\201\373", "\201\254", SJIS },
	/* 3 KU 0x8240(&@$B#!(B)-0x824E(&@$B#/(B) -> 0x81AC($B".(B) */
	{ "\202\100", "\202\116", "\201\254", SJIS },
	/* 3 KU 0x8259(&@$B#:(B)-0x825F(&@$B#@(B) -> 0x81AC($B".(B) */
	{ "\202\131", "\202\137", "\201\254", SJIS },
	/* 3 KU 0x827A(&@$B#[(B)-0x8280(&@$B#`(B) -> 0x81AC($B".(B) */
	{ "\202\172", "\202\200", "\201\254", SJIS },
	/* 3 KU 0x829B(&@$B#{(B)-0x829E(&@$B#~(B) -> 0x81AC($B".(B) */
	{ "\202\233", "\202\236", "\201\254", SJIS },
	/* 4 KU 0x82F2(&@$B$t(B)-0x82FC(&@$B$~(B) -> 0x81AC($B".(B) */
	{ "\202\362", "\202\374", "\201\254", SJIS },
	/* 5 KU 0x8397(&@$B%w(B)-0x839E(&@$B%~(B) -> 0x81AC($B".(B) */
	{ "\203\227", "\203\236", "\201\254", SJIS },
	/* 6 KU 0x83B7(&@$B&9(B)-0x83BE(&@$B&@(B) -> 0x81AC($B".(B) */
	{ "\203\267", "\203\276", "\201\254", SJIS },
	/* 6 KU 0x83D7(&@$B&Y(B)-0x83FC(&@$B&~(B) -> 0x81AC($B".(B) */
	{ "\203\327", "\203\374", "\201\254", SJIS },
	/* 7 KU 0x8461(&@$B'B(B)-0x846F(&@$B'P(B) -> 0x81AC($B".(B) */
	{ "\204\141", "\204\157", "\201\254", SJIS },
	/* 7 KU 0x8492(&@$B'r(B)-0x849E(&@$B'~(B) -> 0x81AC($B".(B) */
	{ "\204\222", "\204\236", "\201\254", SJIS },
	/* 8 KU 0x84BF(&@$B(A(B)-0x84FC(&@$B(~(B) -> 0x81AC($B".(B) */
	{ "\204\277", "\204\374", "\201\254", SJIS },
	/* 9-14 KU 0x8540(&@$B)!(B)-0x87FC(&@$B.~(B) -> 0x81AC($B".(B) */
	{ "\205\100", "\207\374", "\201\254", SJIS },
	/* 15 KU 0x8840(&@$B/!(B)-0x889E(&@$B/~(B) -> 0x81AC($B".(B) */
	{ "\210\100", "\210\236", "\201\254", SJIS },
	/* 47 KU 0x9873(&@$BOT(B)-0x989E(&@$BO~(B) -> 0x81AC($B".(B) */
	{ "\230\163", "\230\236", "\201\254", SJIS },
	/* 84 KU 0xEAA5(&@$Bt'(B)-0xEAFC(&@$Bt~(B) -> 0x81AC($B".(B) */
	{ "\352\245", "\352\374", "\201\254", SJIS },

	/*
	 * SJIS uses area from 85 KU to 120 KU for GAIJI, but current less
	 * doesn't allow GAIJI.
	 */
	/* 85-94 KU 0xEB40(&@$Bu!(B)-0xEFFC(&@$B~~(B) -> 0x81AC($B".(B) */
	{ "\353\100", "\357\374", "\201\254", SJIS },
	/* 95-120 KU 0xF040(none)-0xFC9E(none) -> 0x81AC($B".(B) */
	{ "\360\100", "\374\374", "\201\254", SJIS },
#endif /* SJIS0213 */

	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable etable_sjis = { eliminate_wrong_sjis, NULL };

static convtab eliminate_wrong_ujis[] = {
#if UJIS0213
	/* JIS X 0213:2000 plane 1 for UJIS0213 */

	/* no empty row */

	/* sequences of empty columns */
	/* 4 KU 0xA4FC($(O$|(B)-0xA4FE($(O$~(B) -> 0xA2AE($B".(B) */
	{ "\244\374", "\244\376", "\242\256", UJIS },
	/* 8 KU 0xA8DF($(O(_(B)-0xA8E6($(O(f(B) -> 0xA2AE($B".(B) */
	{ "\250\337", "\250\346", "\242\256", UJIS },
	/* 8 KU 0xA8FD($(O(}(B)-0xA8FE($(O(~(B) -> 0xA2AE($B".(B) */
	{ "\250\375", "\250\376", "\242\256", UJIS },
	/* 12 KU 0xACF4($(O,t(B)-0xACFC($(O,|(B) -> 0xA2AE($B".(B) */
	{ "\254\364", "\254\374", "\242\256", UJIS },
	/* 13 KU 0xADD8($(O-X(B)-0xADDE($(O-^(B) -> 0xA2AE($B".(B) */
	{ "\255\330", "\255\336", "\242\256", UJIS },
	/* 13 KU 0xADF0($(O-p(B)-0xADF2($(O-r(B) -> 0xA2AE($B".(B) */
	{ "\255\360", "\255\362", "\242\256", UJIS },
	/* 13 KU 0xADF4($(O-t(B)-0xADF7($(O-w(B) -> 0xA2AE($B".(B) */
	{ "\255\364", "\255\367", "\242\256", UJIS },
	/* 13 KU 0xADFA($(O-z(B)-0xADFC($(O-|(B) -> 0xA2AE($B".(B) */
	{ "\255\372", "\255\374", "\242\256", UJIS },
	/* 14 KU 0xAEA1($(O.!(B) -> 0xA2AE($B".(B) */
	{ "\256\241", NULL, "\242\256", UJIS },
	/* 15 KU 0xAFFE($(O/~(B) -> 0xA2AE($B".(B) */
	{ "\257\376", NULL, "\242\256", UJIS },
	/* 47 KU 0xCFD4($(OOT(B) -> 0xA2AE($B".(B) */
	{ "\317\324", NULL, "\242\256", UJIS },
	/* 47 KU 0xCFFE($(OO~(B) -> 0xA2AE($B".(B) */
	{ "\317\376", NULL, "\242\256", UJIS },
	/* 84 KU 0xF4A7($(Ot'(B) -> 0xA2AE($B".(B) */
	{ "\364\247", NULL, "\242\256", UJIS },
	/* 94 KU 0xFEFA($(O~z(B)-0xFEFE($(O~~(B) -> 0xA2AE($B".(B) */
	{ "\376\372", "\376\376", "\242\256", UJIS },

	/*
	 * UJIS0213 shares G2 space by JIS X 0213:2000 plane 2 and
	 * JIS X 0212:1990.  later has some empty rows and some empty
	 * columns in particular rows.  JIS X 0213:2000 plane 2 shares
	 * those empty rows.  So, totally UJIS0213 has no empty row.
	 */

	/* JIS X 0212:1990 for UJIS0213 */
	/* Empty columns in particular rows are defined after below #endif */

	/* JIS X 0213:2000 plane 2 for UJIS0213 */
	/* sequences of empty columns */
	/* 94 KU 0xFEF7($(P~w(B)-0xFEFE($(P~~(B) -> 0xA2AE($B".(B) */
	{ "\217\376\367", "\217\376\376", "\242\256", UJIS },
#else /* UJIS0213 */
	/* UJIS uses JIS X 0208 1983 */

	/* empty rows */
	/* 9-15 KU 0xA9A1($B)!(B)-0xAFFE($B/~(B) -> 0xA2AE($B".(B) */
	{ "\251\241", "\257\376", "\242\256", UJIS },
	/*
	 * UJIS uses area from 85 KU to 94 KU for GAIJI, but current less
	 * doesn't allow GAIJI.
	 */
	/* 85-94 KU 0xF5A1($Bu!(B)-0xFEFE($B~~(B) -> 0xA2AE($B".(B) */
	{ "\365\241", "\376\376", "\242\256", UJIS },

	/* sequences of empty columns */
	/* 2 KU 0xA2AF($B"/(B)-0xA2B9($B"9(B) -> 0xA2AE($B".(B) */
	{ "\242\257", "\242\271", "\242\256", UJIS },
	/* 2 KU 0xA2C2($B"B(B)-0xA2C9($B"I(B) -> 0xA2AE($B".(B) */
	{ "\242\302", "\242\311", "\242\256", UJIS },
	/* 2 KU 0xA2D1($B"Q(B)-0xA2DB($B"[(B) -> 0xA2AE($B".(B) */
	{ "\242\321", "\242\333", "\242\256", UJIS },
	/* 2 KU 0xA2EB($B"k(B)-0xA2F1($B"q(B) -> 0xA2AE($B".(B) */
	{ "\242\353", "\242\361", "\242\256", UJIS },
	/* 2 KU 0xA2FA($B"z(B)-0xA2FD($B"}(B) -> 0xA2AE($B".(B) */
	{ "\242\372", "\242\375", "\242\256", UJIS },
	/* 3 KU 0xA3A1($B#!(B)-0xA3AF($B#/(B) -> 0xA2AE($B".(B) */
	{ "\243\241", "\243\257", "\242\256", UJIS },
	/* 3 KU 0xA3BA($B#:(B)-0xA3C0($B#@(B) -> 0xA2AE($B".(B) */
	{ "\243\272", "\243\300", "\242\256", UJIS },
	/* 3 KU 0xA3DB($B#[(B)-0xA3E0($B#`(B) -> 0xA2AE($B".(B) */
	{ "\243\333", "\243\340", "\242\256", UJIS },
	/* 3 KU 0xA3FB($B#{(B)-0xA3FE($B#~(B) -> 0xA2AE($B".(B) */
	{ "\243\373", "\243\376", "\242\256", UJIS },
	/* 4 KU 0xA4F4($B$t(B)-0xA4FE($B$~(B) -> 0xA2AE($B".(B) */
	{ "\244\364", "\244\376", "\242\256", UJIS },
	/* 5 KU 0xA5F7($B%w(B)-0xA5FE($B%~(B) -> 0xA2AE($B".(B) */
	{ "\245\367", "\245\376", "\242\256", UJIS },
	/* 6 KU 0xA6B9($B&9(B)-0xA6C0($B&@(B) -> 0xA2AE($B".(B) */
	{ "\246\271", "\246\300", "\242\256", UJIS },
	/* 6 KU 0xA6D9($B&Y(B)-0xA6FE($B&~(B) -> 0xA2AE($B".(B) */
	{ "\246\331", "\246\376", "\242\256", UJIS },
	/* 7 KU 0xA7C2($B'B(B)-0xA7D0($B'P(B) -> 0xA2AE($B".(B) */
	{ "\247\302", "\247\320", "\242\256", UJIS },
	/* 7 KU 0xA7F2($B'r(B)-0xA7FE($B'~(B) -> 0xA2AE($B".(B) */
	{ "\247\362", "\247\376", "\242\256", UJIS },
	/* 8 KU 0xA8C1($B(A(B)-0xA8FE($B(~(B) -> 0xA2AE($B".(B) */
	{ "\250\301", "\250\376", "\242\256", UJIS },
	/* 47 KU 0xCFD4($BOT(B)-0xCFFE($BO~(B) -> 0xA2AE($B".(B) */
	{ "\317\324", "\317\376", "\242\256", UJIS },
	/* 84 KU 0xF4A5($Bt%(B)-0xF4FE($Bt~(B) -> 0xA2AE($B".(B) */
	{ "\364\245", "\364\376", "\242\256", UJIS },

	/* JIS X 0212:1990 for UJIS */
	/*
	 * Here, we defines only empty rows.  Empty columns in
	 * particular rows are defined after below #endif
	 */
	/* empty rows */
	/* 1 KU 0xA1A1($(D!!(B)-0xA1FE($(D!~(B) -> 0xA2AE($B".(B) */
	{ "\217\241\241", "\217\241\376", "\242\256", UJIS },
	/* 3-5 KU 0xA3A1($(D#!(B)-0xA5FE($(D%~(B) -> 0xA2AE($B".(B) */
	{ "\217\243\241", "\217\245\376", "\242\256", UJIS },
	/* 8 KU 0xA8A1($(D(!(B)-0xA8FE($(D(~(B) -> 0xA2AE($B".(B) */
	{ "\217\250\241", "\217\250\376", "\242\256", UJIS },
	/* 12-15 KU 0xACA1($(D,!(B)-0xACFE($(D/~(B) -> 0xA2AE($B".(B) */
	{ "\217\254\241", "\217\257\376", "\242\256", UJIS },
	/* 78-94 KU 0xEEA1($(Dn!(B)-0xFEFE($(D~~(B) -> 0xA2AE($B".(B) */
	{ "\217\356\241", "\217\376\376", "\242\256", UJIS },
#endif /* UJIS0213 */
	/* JIS X 0212:1990 */
	/*
	 * Here, we defines only empty columns in particular rows
	 * Empty rows are defined before above #endif
	 */
	/* sequences of empty columns */
	/* 2 KU 0xA2A1($(D"!(B)-0xA2AE($(D".(B) -> 0xA2AE($B".(B) */
	{ "\217\242\241", "\217\242\256", "\242\256", UJIS },
	/* 2 KU 0xA2BA($(D":(B)-0xA2C1($(D"A(B) -> 0xA2AE($B".(B) */
	{ "\217\242\272", "\217\242\301", "\242\256", UJIS },
	/* 2 KU 0xA2C5($(D"E(B)-0xA2EA($(D"j(B) -> 0xA2AE($B".(B) */
	{ "\217\242\305", "\217\242\352", "\242\256", UJIS },
	/* 2 KU 0xA2F2($(D"r(B)-0xA2FE($(D"~(B) -> 0xA2AE($B".(B) */
	{ "\217\242\362", "\217\242\376", "\242\256", UJIS },
	/* 6 KU 0xA6A1($(D&!(B)-0xA6E0($(D&`(B) -> 0xA2AE($B".(B) */
	{ "\217\246\241", "\217\246\340", "\242\256", UJIS },
	/* 6 KU 0xA6E6($(D&f(B) -> 0xA2AE($B".(B) */
	{ "\217\246\346", NULL, "\242\256", UJIS },
	/* 6 KU 0xA6E8($(D&h(B) -> 0xA2AE($B".(B) */
	{ "\217\246\350", NULL, "\242\256", UJIS },
	/* 6 KU 0xA6EB($(D&k(B) -> 0xA2AE($B".(B) */
	{ "\217\246\353", NULL, "\242\256", UJIS },
	/* 6 KU 0xA6ED($(D&m(B)-0xA6F0($(D&p(B) -> 0xA2AE($B".(B) */
	{ "\217\246\355", "\217\246\360", "\242\256", UJIS },
	/* 6 KU 0xA6FD($(D&}(B)-0xA6FE($(D&~(B) -> 0xA2AE($B".(B) */
	{ "\217\246\375", "\217\246\376", "\242\256", UJIS },
	/* 7 KU 0xA7A1($(D'!(B)-0xA7C1($(D'A(B) -> 0xA2AE($B".(B) */
	{ "\217\247\241", "\217\247\301", "\242\256", UJIS },
	/* 7 KU 0xA7CF($(D'O(B)-0xA7F1($(D'q(B) -> 0xA2AE($B".(B) */
	{ "\217\247\317", "\217\247\361", "\242\256", UJIS },
	/* 9 KU 0xA9A3($(D)#(B) -> 0xA2AE($B".(B) */
	{ "\217\251\243", NULL, "\242\256", UJIS },
	/* 9 KU 0xA9A5($(D)%(B) -> 0xA2AE($B".(B) */
	{ "\217\251\245", NULL, "\242\256", UJIS },
	/* 9 KU 0xA9A7($(D)'(B) -> 0xA2AE($B".(B) */
	{ "\217\251\247", NULL, "\242\256", UJIS },
	/* 9 KU 0xA9AA($(D)*(B) -> 0xA2AE($B".(B) */
	{ "\217\251\252", NULL, "\242\256", UJIS },
	/* 9 KU 0xA9AE($(D).(B) -> 0xA2AE($B".(B) */
	{ "\217\251\256", NULL, "\242\256", UJIS },
	/* 9 KU 0xA9B1($(D)1(B)-0xA9C0($(D)@(B) -> 0xA2AE($B".(B) */
	{ "\217\251\261", "\217\251\300", "\242\256", UJIS },
	/* 9 KU 0xA9D1($(D)Q(B)-0xA9FE($(D)~(B) -> 0xA2AE($B".(B) */
	{ "\217\251\321", "\217\251\376", "\242\256", UJIS },
	/* 10 KU 0xAAB9($(D*9(B) -> 0xA2AE($B".(B) */
	{ "\217\252\271", NULL, "\242\256", UJIS },
	/* 10 KU 0xAAF8($(D*x(B)-0xAAFE($(D*~(B) -> 0xA2AE($B".(B) */
	{ "\217\252\370", "\217\252\376", "\242\256", UJIS },
	/* 11 KU 0xABBC($(D+<(B) -> 0xA2AE($B".(B) */
	{ "\217\253\274", NULL, "\242\256", UJIS },
	/* 11 KU 0xABC4($(D+D(B) -> 0xA2AE($B".(B) */
	{ "\217\253\304", NULL, "\242\256", UJIS },
	/* 11 KU 0xABF8($(D+x(B)-0xABFE($(D+~(B) -> 0xA2AE($B".(B) */
	{ "\217\253\370", "\217\253\376", "\242\256", UJIS },
	/* 77 KU 0xEDE4($(Dmd(B)-0xEDFE($(Dm~(B) -> 0xA2AE($B".(B) */
	{ "\217\355\344", "\217\355\376", "\242\256", UJIS },

	/* NULL */
	{ 0, 0, 0, 0 }
};
static convtable etable_ujis = { eliminate_wrong_ujis, NULL };


static int iso646p(cs)
CHARSET cs;
{
	if (CS2TYPE(cs) != TYPE_94_CHARSET)
		return 0;
	switch (CS2CHARSET(cs)) {
	case TYPE_94_CHARSET | FT2CS('@'): /* ISO 646 IRV 1983 */
	case TYPE_94_CHARSET | FT2CS('A'): /* BSI 4730 United Kingdom */
	case TYPE_94_CHARSET | FT2CS('C'): /* NATS Standard Swedish/Finish */
	case TYPE_94_CHARSET | FT2CS('G'): /* ISO 646 Swedish */
					   /* (SEN 850200 Ann. B) */
	case TYPE_94_CHARSET | FT2CS('H'): /* ISO 646 Swedish Name */
					   /* (SEN 850200 Ann. C) */
	case JISX0201ROMAN:		   /* JIS X 0201-1976 Roman */
	case TYPE_94_CHARSET | FT2CS('K'): /* ISO 646 German (DIN 66083) */
	case TYPE_94_CHARSET | FT2CS('L'): /* ISO 646 Portuguese (ECMA) */
	case TYPE_94_CHARSET | FT2CS('R'): /* French */
	case TYPE_94_CHARSET | FT2CS('T'): /* China */
	case TYPE_94_CHARSET | FT2CS('Y'): /* Italian */
	case TYPE_94_CHARSET | FT2CS('Z'): /* Spanish */
	case TYPE_94_CHARSET | FT2CS('`'): /* NS 4551 Version 1 */
	case TYPE_94_CHARSET | FT2CS('a'): /* NS 4551 Version 2 */
	case TYPE_94_CHARSET | FT2CS('f'): /* NF Z 62-010-1982 */
	case TYPE_94_CHARSET | FT2CS('g'): /* IBM Portuguese */
	case TYPE_94_CHARSET | FT2CS('h'): /* IBM Spanish */
	case TYPE_94_CHARSET | FT2CS('i'): /* MS Z 7795/3 [Hungary] */
	case TYPE_94_CHARSET | FT2CS('n'): /* JIS C 6229-1984 OCR-B [Japan] */
	case TYPE_94_CHARSET | FT2CS('u'): /* CCITT Recommendation T.61 */
					   /* Teletex Primary Set */
	case TYPE_94_CHARSET | FT2CS('w'): /* CSA Z 243.4-1985 Alternate */
					   /* Primary Set No.1 [Canada] */
	case TYPE_94_CHARSET | FT2CS('x'): /* CSA Z 243.4-1985 Alternate */
					   /* Primary Set No.2 [Canada] */
	case TYPE_94_CHARSET | FT2CS('z'): /* JUS I.B1.002 [Yugoslavia] */
		return 1;
	default:
		return 0;
	}
}

#if 0
static char *
quote_it(src, cs, search_type)
	char *src;
	CHARSET *cs;
	int search_type;
{
	static char *buf = NULL;
	static int size = 0;
	int len = strlen_cs(src, cs) * 2;
	char *dst;

	if (len + 1 > size)
	{
		size = (len + 1 + 255) / 256 * 256;
		if (buf)
			free(buf);
		buf = (char *) ecalloc(size, sizeof(char));
	}
	dst = buf;
	while (*src != '\0')
	{
#if MSB_ENABLE
		if (CSISASCII(*cs) || CSISWRONG(*cs))
			*dst++ = *src++;
		else
			*dst++ = *src++ | 0x80;
		cs++;
#else
		if (!CSISASCII(*cs++) && !(search_type & SRCH_NO_REGEX))
		{
			switch (*src) {
			/* Basic Regular Expressions */
			case '[':
			case ']':
			case '.':
			case '*':
			case '\\':
			case '^':
			case '$':
#if (HAVE_POSIX_REGCOMP_CS || HAVE_POSIX_REGCOMP) && defined(REG_EXTENDED)
			/* Extended Regular Expressions */
			case '+':
			case '?':
			case '|':
			case '(':
			case ')':
			case '{':
			case '}':
#endif
#if HAVE_RE_COMP
			/* No Extended Regular Expressions */
#endif
#if HAVE_REGCMP
			/* Extended Regular Expressions */
			case '+':
			case '(':
			case ')':
			case '{':
			case '}':
#endif
#if HAVE_V8_REGCOMP_CS || HAVE_V8_REGCOMP
			/* Extended Regular Expressions */
			case '+':
			case '?':
			case '|':
			case '(':
			case ')':
#endif
				*dst++ = '\\';
				/* fall through */
			default:
				*dst++ = *src++;
				break;
			}
		} else
			*dst++ = *src++;
#endif
	}
	*dst = '\0';
	return (buf);
}
#endif

/*
 * convert JIS C 6226-1978 into JIS X 0208:1990
 */
void jis78to90(str)
char* str;
{
	convtab* ptab;

	/* convert JIS C 6226-1978 into JIS X 0208:1990 */
	ptab = find_convtab(&ctable_jisx0208_78_90, str);
	if (ptab) {
		str[0] = ptab->output[0];
		str[1] = ptab->output[1];
	}
}

void chconvert_cs(istr, ics, ostr, ocs, flag)
char* istr;
CHARSET* ics;
char* ostr;
CHARSET* ocs;
int flag;	/* quote regexp pattern */
{
	int i;
	convtab* ptab;

	if (istr[0] == NULCH && CSISNULLCS(ics[0])) {
		ostr[0] = NULCH;
		ocs[0] = NULLCS;
		return;
	}
	/* convert codes into some traditional character sets */
	if (CS2CHARSET(*ics) == JISX0208_78KANJI) {
		/* convert JIS C 6226-1978 into JIS X 0208:1990 */
		ptab = find_convtab(&ctable_jisx0208_78_90, istr);
		if (ptab) {
			ostr[0] = ptab->output[0];
			ostr[1] = ptab->output[1];
			ocs[0] = ptab->charset;
			ocs[1] = ptab->charset | REST_MASK;
		} else {
			ostr[0] = istr[0];
			ostr[1] = istr[1];
			ocs[0] = JISX0208_90KANJI;
			ocs[1] = JISX0208_90KANJI | REST_MASK;
		}
		ostr[2] = NULCH;
		ocs[2] = NULLCS;
	} else if (CS2CHARSET(*ics) == JISX0208KANJI) {
		/* convert JIS X 0208-1983 into JIS X 0208:1990 */
		ostr[0] = istr[0];
		ostr[1] = istr[1];
		ocs[0] = JISX0208_90KANJI;
		ocs[1] = JISX0208_90KANJI | REST_MASK;

		/*
		 * Difference betwen 1983 and 1990 are two added characters,
		 * 0x7425 and 0x7426.  So, here is nothing to do.
		 */
		ostr[2] = NULCH;
		ocs[2] = NULLCS;
	} else if (CS2CHARSET(*ics) == JISX0201ROMAN) {
		/* convert JIS X 0201:1976 into ASCII */
		ptab = find_convtab(&utable_n_jisx0201roman, istr);
		ostr[0] = istr[0];
		if (!ptab) {
			ocs[0] = ASCII;
		}
		ostr[1] = NULCH;
		ocs[1] = NULLCS;
	} else if (iso646p(*ics)) {
		/* convert domestic ISO 646 into ASCII */
		ptab = find_convtab(&utable_n_iso646, istr);
		ostr[0] = istr[0];
		if (!ptab) {
			ocs[0] = ASCII;
		}
		ostr[1] = NULCH;
		ocs[1] = NULLCS;
	} else {
		/* copy input to output */
		i = 0;
		do {
			ostr[i] = istr[i];
			ocs[i] = ics[i];
			i++;
		} while (CSISREST(ics[i]));
		ostr[i] = NULCH;
		ocs[i] = NULLCS;
	}
}

void chunify_cs(istr, ics, ostr, ocs)
char* istr;
CHARSET* ics;
char* ostr;
CHARSET* ocs;
{
	int i;
	convtab* ptab;

	chconvert_cs(istr, ics, ostr, ocs);
	/* unify codes */
	if (CS2CHARSET(*ocs) == JISX0208_90KANJI) {
		/*
		 * convert ASCII, GREEK and CYRILLIC character in
		 * JIS X 0208-1990 into ASCII, ISO 8859-7 and ISO 8859-5
		 * respectively.
		 */
		ptab = find_convtab(&utable_jisx0208, ostr);
		if (ptab) {
			int len = strlen(ptab->output);
			assert(len <= (int)strlen(ostr));
			ostr[0] = ptab->output[0];
			ocs[0] = ptab->charset;
			for (i = 1; i < len; i++) {
				ostr[i] = ptab->output[i];
				ocs[i] = ptab->charset | REST_MASK;
			}
			ostr[i] = NULCH;
			ocs[i] = NULLCS;
		}
	}
}

int chcmp_cs(str1, cs1, str2, cs2)
char* str1;
CHARSET* cs1;
char* str2;
CHARSET* cs2;
{
	char buf1[32];
	CHARSET bcs1[32];
	char buf2[32];
	CHARSET bcs2[32];

	/* if there is no character set, compare them as ASCII */
	if (cs1 == NULL && cs2 == NULL)
		return *str1 - *str2;
	if (cs1 == NULL)
		return chcmp_cs(str2, cs2, str1, cs1);
	if (cs2 == NULL)
		return MAKECV(*str1, *cs1) - MAKECV(*str2, ASCII);

	/* unify both of inputs */
	chunify_cs(str1, cs1, buf1, bcs1);
	str1 = buf1;
	cs1 = bcs1;
	chunify_cs(str2, cs2, buf2, bcs2);
	str2 = buf2;
	cs2 = bcs2;
	/* compare them */
	if ((*str1 == NULCH && CSISNULLCS(*cs1)) ||
	    (*str2 == NULCH && CSISNULLCS(*cs2)))
		return MAKECV(*str1, *cs1) - MAKECV(*str2, *cs2);
	do {
		if (*str1 != *str2 || *cs1 != *cs2) {
			return MAKECV(*str1, *cs1) - MAKECV(*str2, *cs2);
		}
		str1++;
		cs1++;
		str2++;
		cs2++;
	} while (CSISREST(*cs1));
	return 0;
}

int chisvalid_cs(istr, ics)
char* istr;
CHARSET* ics;
{
	int i;
	convtab* ptab;

	if (istr[0] == NULCH && CSISNULLCS(ics[0]))
		return 0;

	/* check wrong codes if it is some traditional character set */
	if (CS2CHARSET(*ics) == JISX0208_78KANJI) {
		ptab = find_convtab(&etable_jisx0208_78, istr);
		if (ptab)
			return 0;
		else
			return 1;
	} else if (CS2CHARSET(*ics) == JISX0208KANJI) {
		ptab = find_convtab(&etable_jisx0208_83, istr);
		if (ptab)
			return 0;
		else
			return 1;
	} else if (CS2CHARSET(*ics) == JISX0208_90KANJI) {
		/* eliminate wrong codes */
		ptab = find_convtab(&etable_jisx0208_90, istr);
		if (ptab)
			return 0;
		else
			return 1;
	} else if (CS2CHARSET(*ics) == JISX0212KANJISUP) {
		ptab = find_convtab(&etable_jisx0212, istr);
		if (ptab)
			return 0;
		else
			return 1;
	} else if (CS2CHARSET(*ics) == JISX0213KANJI1) {
		ptab = find_convtab(&etable_jisx0213_1, istr);
		if (ptab)
			return 0;
		else
			return 1;
	} else if (CS2CHARSET(*ics) == JISX0213KANJI2) {
		ptab = find_convtab(&etable_jisx0213_2, istr);
		if (ptab)
			return 0;
		else
			return 1;
	} else if (CS2CHARSET(*ics) == SJIS) {
		/* eliminate wrong codes */
		ptab = find_convtab(&etable_sjis, istr);
		if (ptab)
			return 0;
		else
			return 1;
	} else if (CS2CHARSET(*ics) == UJIS) {
		/* eliminate wrong codes */
		ptab = find_convtab(&etable_ujis, istr);
		if (ptab)
			return 0;
		else
			return 1;
		/* TODO: G2 */
	}
	return 1;
}

#endif
