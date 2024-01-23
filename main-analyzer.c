#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>

#include "fatarena.h"
#include "lib/map.h"
#include "token.h"
#include "lex.h"
#include "parser.h"
#include "analyzer.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define TODO(message) \
    do { \
        fprintf(stderr, "TODO(%s): %s\n", TOSTRING(__LINE__), message); \
    } while (0)

#define ERR(...) fprintf(stderr, "ERR(%d,%d): ", __LINE__, line); fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")

const Str langtypes[NLANGTYPE] = {
	{(uint8_t*)"u8",   2},
	{(uint8_t*)"u16",  3},
	{(uint8_t*)"u32",  3},
	{(uint8_t*)"u64",  3},
	{(uint8_t*)"u128", 4},
	{(uint8_t*)"i8",   2},
	{(uint8_t*)"i16",  3},
	{(uint8_t*)"i32",  3},
	{(uint8_t*)"i64",  3},
	{(uint8_t*)"i128", 4},
	{(uint8_t*)"f32",  3},
	{(uint8_t*)"f64",  3},
};

int ident2langtype[NLANGTYPE * 3] = {-1};
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

Symbols funsym = {0};
Symbols identsym = {0};
Symbols typesym = {0};

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

	int pagesz = getpgsz();

	identsym.array = ftalloc(&ftsym, 10 * pagesz);
	funsym.array   = ftalloc(&ftsym, 10 * pagesz);
	typesym.array  = ftalloc(&ftsym, 10 * pagesz);

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

	int i = 0;
	int res = -1;
	intptr stmt = -1;

	while (tlst[i] != EOI) {
		res = parse_toplevel(tlst + i, &stmt);
		if (res < 0) {
			fprintf(stderr, "Error when parsing toplevel stmt.\n");
			return 1;
		}
		i += res;

		UnknownStmt *stmtptr = (UnknownStmt*) ftptr(&ftast, stmt);
		intptr ident = -1;
		switch (*stmtptr) {
		case SDECL: {
			SDecl* decl = ((SDecl*) stmtptr);
			ident = decl->ident;
			res = inserttopdcl(&identsym, ident, stmt);
			break;
		}
		case SFUN: {
			ident = ((SFun*) stmtptr)->ident;
			res = inserttopdcl(&funsym, ident, stmt);
			break;
		}
		case SSTRUCT: {
			ident = ((SFun*) stmtptr)->ident;
			res = inserttopdcl(&typesym, ident, stmt);
			break;
		}
		case SIMPORT: {
			TODO("save imports");
			break;
		}
		default:
			ERR("Unreachable statement here.");
		}

		if (!res) {
			ERR("Already declared: <%s>.", (char*) ftptr(&ftident, ident));
		}
	}

	// -------------------- Analyzing
	intptr info = ftalloc(&ftsym, sizeof(SymInfo) * 10000);
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
	printsymbolsinfo(nsym);

	Symbol *symf = (Symbol*) ftptr(&ftsym, funsym.array);
	for (int i = 0; i < funsym.nsym; i++) {
		SFun* fun = (SFun*) ftptr(&ftast, symf[i].stmt);
		assert(analyzefun(fun, nsym));
	}

	printsymbols(&typesym);
	printsymbols(&identsym);
	printsymbols(&funsym);
}


