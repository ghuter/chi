#ifndef ANALYSE_H
#define ANALYSE_H

// -------------------- SymInfo

typedef struct {
	intptr ident;
	intptr type;
	int    ptrlvl;
	Bool cst;
} SymInfo;

// -------------------- Analyze context

typedef struct {
	SModSign *sign;
	SModImpl *impl;
} AnalyzeCtx;

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

void printsymbols(FILE* fd, Symbols *syms);
void printsymbolsinfo(FILE* fd, int nsym);
void printstmtarray(FILE *fd, StmtArray *array);
Bool ispub(const StmtArray *pubsym, const intptr ident, const EStmt *kind);
Bool analyseimport(StmtArray *imports);
Bool analyzetype(SStruct *stmt);
Bool analyzeglobalcst(SDecl *decl, int nelem);
Bool analyzefun(AnalyzeCtx *ctx, SFun *fun, intptr stmt, int nsym);
Bool analyzemodsign(SModSign *sign);
Bool analyzemodimpl(SModImpl *impl);
Bool analyzemodskel(SModSkel *skel);
Bool analyzemoddef(SModDef *def);
Bool analyzepub(StmtArray *pubsym);
Symbol* searchtopdcl(Symbols *syms, intptr ident);
#endif /* ANALYSE_H */
