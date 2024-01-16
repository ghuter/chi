#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "lib/fatarena.h"
#include "lib/map.h"

#define STROF(s) ((Str) { (uint8_t*)s, sizeof(s)-1 })
#define nelem(a) (sizeof(a)/sizeof(*a))

int
main(int argc, char **argv)
{
	FatArena perm;

	(void)argc;
	(void)argv;

	if (!ftnew(&perm, 1)) {
		exit(1);
	}

	Str strings[] = {
		STROF("hello"),
		STROF("world"),
		STROF("how"),
		STROF("have"),
		STROF("you been"),
		STROF("hello"),
		STROF("world"),
	};

	Strimap *m = {0};
	for (size_t i = 0; i < nelem(strings); i++) {
		uint32_t *val = stri_upsert(&m, strings[i], &perm);
		if (!val) {
			printf("key not found: '%s'\n", strings[i].data);
		} else {
			printf("key: '%s', cur. value: %d, new value: %ld\n",
				strings[i].data, *val, i);
			*val = i;
		}
	}
}
