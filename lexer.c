#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "fatarena.h"
#include "token.h"
#include "lib/map.h"
#include "lex.h"

FatArena ftident = {0};
FatArena ftlit = {0};
FatArena ftimmed = {0};
FatArena fttmp = {0};

int
main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	POK(ftnew(&ftident, 1000000) != 0, "fail to create a FatArena");
	ftalloc(&ftident, NKEYWORDS + 1);// burn significant int

	POK(ftnew(&ftident, 1000000) != 0, "fail to create a FatArena");
	ftalloc(&ftident, NKEYWORDS + 1);// burn significant int

	POK(ftnew(&ftimmed, 1000000) != 0, "fail to create a FatArena");
	ftalloc(&ftimmed, NKEYWORDS + 1);// burn significant int

	POK(ftnew(&ftlit, 1000000) != 0, "fail to create a FatArena");
	ftalloc(&ftlit, NKEYWORDS + 1);// burn significant int

	POK(ftnew(&fttmp, 1000000) != 0, "fail to create a FatArena");

	Tok t = {0};
	do {
		t = getnext();
		printtok(stdout, t);
	} while (t.type != EOI);
}

