#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "lib/fatarena.h"
#include "lib/map.h"
#include "lib/token.h"

#include "lex.h"
#include "parser.h"
#include "analyzer.h"
#include "marshal.h"

#define A(...) do { dprintf(hd, __VA_ARGS__); } while(0)


static void
gensig(int fd, intptr stmt)
{
	UnknownStmt *ptr = (UnknownStmt*) ftptr(&ftast, stmt);
	if (stmt == -1) {
		return;
	}
	if (*ptr != SMODSIGN) {
		return;
	}

	SModSign *s = (SModSign*) ptr;
	A("%s :: signature ", identstr(s->ident));
	gengenerics(fd, s->generics);
	A(", ");
	gensignatures(fd, s->signatures);
	A(", {");
	intptr *stmt = (intptr*) ftptr(&ftast, s->stmts);
	for (int i = 0; i < s->nstmt; i++) {
		genstmt(fd, stmt[i]);
		if (i < s->nstmt - 1) {
			A(", ");
		}
	}
	A("})");
}

static void
genimpl(int fd, intptr stmt)
{
	UnknownStmt *ptr = (UnknownStmt*) ftptr(&ftast, stmt);
	if (stmt == -1) {
		return;
	}
	if (*ptr != SMODIMPL) {
		return;
	}

	// TODO(ghuter): gen prototypes ?
}

void
marshal(int fd, Symbols modsym[NMODSYM])
{
	Symbols *syms;
	Symbol *sym;

	syms = &modsym[MODSIGN];
	sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		gensig(fd, sym[i].stmt);
	}

	syms = &modsym[MODIMPL];
	sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		genimpl(fd, sym[i].stmt);
	}
}

void
unmarshal(int fd, Symbols modsym[NMODSYM])
{
	(void) fd;
	(void) modsym;
}
