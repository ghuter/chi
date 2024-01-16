#ifndef MAP_H
#define MAP_H

typedef struct {
	uint8_t *data;
	ptrdiff_t len;
} Str;

typedef struct Strimap Strimap;
struct Strimap {
	Strimap *child[4];
	Str key;
	uint32_t val;
};

#define strhash(s) do { hash64(s.data, s.len); } while(0)

uint64_t hash64(uint8_t *data, ptrdiff_t len);
Bool streq(Str a, Str b);
uint32_t *stri_upsert(Strimap **m, Str key, FatArena *perm);

#endif
