#define _DEFAULT_SOURCE

#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

typedef struct FatArena {
	uint8_t *addr;
	size_t remain;
	size_t size;
} FatArena;

typedef int Bool;

Bool
ftnew(FatArena *a, size_t sz)
{
	size_t pgsz = sysconf(_SC_PAGESIZE);
	size_t fatsz = pgsz;
	while (fatsz < sz) {
		fatsz += pgsz;
	}

	uint8_t *pg = mmap(NULL, fatsz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
	if (pg == MAP_FAILED) {
		return 0;
	}

	a->addr = pg;
	a->remain = fatsz;
	a->size = fatsz;
	return 1;
}

int
ftalloc(FatArena *a, size_t sz)
{
	if ((sz > a->remain)) {
		return -1;
	}

	int idx = a->size - a->remain;
	a->remain -= sz;
	return idx;
}

Bool
ftwrite(FatArena *a, int idx, const uint8_t *ptr, size_t sz)
{
	uint8_t *addr = a->addr + idx;
	if (addr + sz > addr + a->size) {
		return 0;
	}
	memcpy(addr, ptr, sz);
	return 1;
}

Bool
ftread(const FatArena *a, int idx, uint8_t *ptr, size_t sz)
{
	uint8_t *addr = a->addr + idx;
	if (addr + sz > addr + a->size) {
		return 0;
	}
	memcpy(ptr, addr, sz);
	return 1;
}

uint8_t*
ftptr(const FatArena *a, int idx)
{
	if (idx < 0 || a->size < (unsigned int) idx) {
		return NULL;
	}
	return a->addr + idx;
}

int
ftidx(const FatArena *a, const uint8_t *ptr)
{
	if (ptr > a->addr + a->size) {
		return -1;
	}
	return ptr - a->addr;
}

