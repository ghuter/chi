#ifndef FAT_ARENA_H
#define FAT_ARENA_H

typedef struct FatArena {
	uint8_t *addr;
	size_t remain;
	size_t size;
} FatArena;

typedef int Bool;

size_t getpgsz();
Bool ftnew(FatArena *a, size_t pages);
int ftalloc(FatArena *a, size_t sz);
Bool ftwrite(FatArena *a, int idx, const uint8_t *ptr, size_t sz);
Bool ftread(const FatArena *a, int idx, uint8_t *ptr, size_t sz);
uint8_t* ftptr(const FatArena *a, int idx);
int ftidx(const FatArena *a, const uint8_t *ptr);

#endif // FAT_ARENA_H
