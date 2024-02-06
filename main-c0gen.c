#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fatarena.h"
#include "lib/map.h"
#include "token.h"
#include "lex.h"
#include "parser.h"
#include "analyzer.h"
#include "gen.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define TODO(message) \
    do { \
        fprintf(stderr, "TODO(%s): %s\n", TOSTRING(__LINE__), message); \
    } while (0)

#define ERR(...) fprintf(stderr, "ERR(%d,%d): ", __LINE__, line); fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")

const Str langtypes[NLANGTYPE] = {
	{(uint8_t*)"bool",   4},
	{(uint8_t*)"u8",     2},
	{(uint8_t*)"u16",    3},
	{(uint8_t*)"u32",    3},
	{(uint8_t*)"u64",    3},
	{(uint8_t*)"u128",   4},
	{(uint8_t*)"i8",     2},
	{(uint8_t*)"i16",    3},
	{(uint8_t*)"i32",    3},
	{(uint8_t*)"i64",    3},
	{(uint8_t*)"i128",   4},
	{(uint8_t*)"f32",    3},
	{(uint8_t*)"f64",    3},
};

int ident2langtype[NLANGTYPE * 4] = {-1};
int langtype2ident[NLANGTYPE]     = {-1};
int typeoffset = -1;
int typeend = -1;

FatArena fttok   = {0};
FatArena ftident = {0};
FatArena ftlit   = {0};
FatArena ftimmed = {0};
FatArena fttmp   = {0};
FatArena ftast   = {0};
FatArena ftsym   = {0};
FatArena ftcode  = {0};

Symbols funsym = {0};
Symbols identsym = {0};
Symbols typesym = {0};
Symbols signatures = {0};

SymInfo *syminfo = NULL;

int fstlangtype = -1;
int lstlangtype = -1;

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
	POK(ftnew(&ftsym, 4000000) != 0, "fail to create a FatArena");

	POK(ftnew(&ftcode, 1000000) != 0, "fail to create a FatArena");

	int pagesz = getpgsz();

	identsym.array = ftalloc(&ftsym, sizeof(Symbol) * pagesz);
	funsym.array   = ftalloc(&ftsym, sizeof(Symbol) * pagesz);
	typesym.array  = ftalloc(&ftsym, sizeof(Symbol) * pagesz);
	signatures.array = ftalloc(&ftsym, sizeof(Symbol) * pagesz);

	// -------------------- Alloc lang types
	int type = -1;
	for (unsigned long i = 0; i < NLANGTYPE; i++) {
		if (typeoffset == -1) {
			typeoffset = stri_insert(langtypes[i]);
			type = typeoffset;
		} else if (i == NLANGTYPE - 1) {
			typeend = stri_insert(langtypes[i]);
			type = typeend;
		} else {
			type = stri_insert(langtypes[i]);
		}

		// save conversion table.
		langtype2ident[i] = type;
		ident2langtype[type - typeoffset] = i;
	}

	// -------------------- Lexing
	ETok *tlst = (ETok*) ftptr(&fttok, 0);
	Tok t;
	do {
		t = getnext();
		ftalloc(&fttok, sizeof(Tok));
		if (t.type != SPC && t.type != NEWLINE) {
			tlst[ntok++] = t.type;
		}
	} while (t.type != EOI);

	// -------------------- Parsing
	int res = -1;
	res = parse_tokens(tlst, &signatures, &identsym, &typesym);
	if (res < 0) {
		ERR("Error when parsing tokens.");
		return 1;
	}

	// -------------------- Analyzing
	intptr info = ftalloc(&ftsym, sizeof(SymInfo) * pagesz);
	syminfo = (SymInfo*) ftptr(&ftsym, info);
	int nsym = 0;

	Symbol *sym = (Symbol*) ftptr(&ftsym, typesym.array);
	for (int i = 0; i < typesym.nsym; i++) {
		SStruct* str = (SStruct*) ftptr(&ftast, sym[i].stmt);
		assert(analyzetype(str));
	}

	Symbol *symd = (Symbol*) ftptr(&ftsym, identsym.array);
	for (int i = 0; i < identsym.nsym; i++) {
		SDecl *decl = (SDecl*) ftptr(&ftast, symd[i].stmt);
		assert(analyzeglobalcst(decl, nsym));
		nsym++;
	}
	/* printsymbolsinfo(nsym); */

	Symbol *symf = (Symbol*) ftptr(&ftsym, signatures.array);
	for (int i = 0; i < signatures.nsym; i++) {
		intptr stmt = symf[i].stmt;
		SFun* fun = (SFun*) ftptr(&ftast, stmt);
		assert(analyzefun(fun, stmt, nsym));
	}

	printsymbols(&typesym);
	printsymbols(&identsym);
	printsymbols(&signatures);
	printsymbols(&funsym);


	// -------------------- C0 Generation
	char *code = 0;
	int idx = ftalloc(&ftcode, 1);
	code = (char*)ftptr(&ftcode, idx);
	size_t sz = gen(code, typesym, identsym, signatures, funsym);
	/* dump to stdout */
	write(1, code, sz);
}

