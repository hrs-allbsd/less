#include <stdint.h>

#define U_error          (-1)
#define U_latin          (256)
#define U_kanji          (8836)

#define UMAP(n, c)       (((n) & 0x7fff) << 16 | (c))
#define UMAP_CS(c)       (((c) >> 16) & 0x7fff)
#define UMAP_CHAR(c)     ((c) & 0xffff)

extern int32_t ucode_latin[][U_latin];
extern int32_t ucode_kanji1[U_kanji];
extern int32_t ucode_kanji2[U_kanji];
extern int32_t ucode_cp932[U_kanji];  /* remaping 95-120 ku to 16-41 ku */

extern int32_t *unicode0_map;
extern int32_t *unicode2_map;
extern int16_t iso8859_list[];

#define UMAP_ISO8859_1   (0)
#define UMAP_ISO8859_2   (1)
#define UMAP_ISO8859_3   (2)
#define UMAP_ISO8859_4   (3)
#define UMAP_ISO8859_5   (4)
#define UMAP_ISO8859_6   (5)
#define UMAP_ISO8859_7   (6)
#define UMAP_ISO8859_8   (7)
#define UMAP_ISO8859_9   (8)
#define UMAP_ISO8859_10  (9)
#define UMAP_ISO8859_11  (10)
#define UMAP_ISO8859_13  (11)
#define UMAP_ISO8859_14  (12)
#define UMAP_ISO8859_15  (13)
#define UMAP_ISO8859_16  (14)
#define UMAP_JISX0201    (15)

void make_unicode_map();
