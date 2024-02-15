#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>

#include "fatarena.h"
#include "token.h"
#include "lib/map.h"
#include "lex.h"
#include "parser.h"

FatArena fttok   = {0};
FatArena ftident = {0};
FatArena ftlit   = {0};
FatArena ftimmed = {0};
FatArena fttmp   = {0};
FatArena ftast   = {0};
FatArena ftsym   = {0};

int line = 0;

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

	POK(ftnew(&fttmp, 1000000) != 0, "fail to create a FatArena");
	POK(ftnew(&ftast, 1000000) != 0, "fail to create a FatArena");

	ETok *tlst = (ETok*) ftptr(&fttok, 0);
	Tok t;
	do {
		t = getnext();
		ftalloc(&fttok, sizeof(Tok));
		if (t.type != SPC && t.type != NEWLINE) {
			tlst[ntok++] = t.type;
		}
	} while (t.type != EOI);

	int i = 0;
	int res = -1;
	intptr stmt = -1;
	int pub = 0;
	while (tlst[i] != EOI) {
		res = parse_toplevel(tlst + i, &stmt, &pub);
		if (res < 0) {
			fprintf(stderr, "Error when parsing toplevel stmt.\n");
			return 1;
		}
		printstmt(stdout, stmt);
		fprintf(stdout, "\n");
		i += res;
	}
}

