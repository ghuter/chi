#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "fatarena.h"
#include "map.h"

uint64_t
hash64(uint8_t *data, ptrdiff_t len)
{
	uint64_t h = 0x100;
	for (ptrdiff_t i = 0; i < len; i++) {
		h ^= data[i];
		h *= 1111111111111111111u;
	}
	return h;
}

Bool
streq(Str a, Str b)
{
	return a.len == b.len && !memcmp(a.data, b.data, a.len);
}

uint32_t*
stri_upsert(Strimap **m, Str key, FatArena *perm)
{
	for (uint64_t h = strhash(key); *m; h <<= 2) {
		if (streq(key, (*m)->key)) {
			return &(*m)->val;
		}
		m = &(*m)->child[h >> 62];
	}
	if (!perm) {
		return 0;
	}
	*m = (Strimap*)ftptr(perm, ftalloc(perm, sizeof(Strimap)));
	(*m)->key = key;
	(*m)->val = -1;
	return &(*m)->val;
}
