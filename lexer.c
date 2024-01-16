#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "fatarena.h"
#include "token.h"
#include "lex.h"
#include "parser.c"

int
main(int argc, char *argv[])
{
	(void) argc;
	(void) argv;

	POK(ftnew(&ftident, 1000000) != 0, "fail to create a FatArena");
	ftalloc(&ftident, NKEYWORDS + 1);// burn significant int

	POK(ftnew(&ftimmed, 1000000) != 0, "fail to create a FatArena");
	ftalloc(&ftimmed, NKEYWORDS + 1);// burn significant int

	POK(ftnew(&ftlit, 1000000) != 0, "fail to create a FatArena");
	ftalloc(&ftlit, NKEYWORDS + 1);// burn significant int

	POK(ftnew(&fttok, 1000000) != 0, "fail to create a FatArena");
	int ntok = 0;

	POK(ftnew(&ftast, 1000000) != 0, "fail to create a FatArena");

	ETok *tlst = (ETok*) ftptr(&fttok, 0);
	Tok t;
	do {
		t = getnext();
		ftalloc(&fttok, sizeof(Tok));
		if (t.type != SPC) {
			tlst[ntok++] = t.type;
		}
		printtok(stdout, t);
	} while (t.type != EOI);

	parse_toplevel(tlst);
}

