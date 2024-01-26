#ifndef ANALYSE_H
#define ANALYSE_H

// -------------------- Symbols

typedef struct {
	intptr ident; // Char*
	intptr stmt; // UnknownStmt*
} Symbol;

typedef struct {
	intptr ident;
	intptr type;
	int    ptrlvl;
	Bool cst;
} SymInfo;

typedef struct {
	intptr array; // Symbol*
	int nsym;
} Symbols;

// -------------------- System types

typedef enum {
	// Boolean
	BOOL,

	// Unsigned
	U8,
	U16,
	U32,
	U64,
	U128,

	// Signed
	I8,
	I16,
	I32,
	I64,
	I128,

	// Float
	F32,
	F64,

	// Always last.
	NLANGTYPE,
} LangType;

extern const char* langtypestrs[NLANGTYPE];

// -------------------- Functions

void printsymbols(Symbols *syms);
void printsymbolsinfo(int nsym);
Bool inserttopdcl(Symbols *syms, intptr ident, intptr stmt);
Bool analyzetype(SStruct *stmt);
Bool analyzeglobalcst(SDecl *decl, int nelem);
Bool analyzefun(SFun *fun, int nsym);
#endif /* ANALYSE_H */
