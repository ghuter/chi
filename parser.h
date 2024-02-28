#ifndef PARSER_H
#define PARSER_H

typedef enum {
	MODSIGN,
	MODIMPL,
	MODSKEL,
	MODDEF,
	NMODSYM,
} EModSym;

// -------------------- Memory

extern FatArena ftast;
extern FatArena ftsym;
extern int line;

typedef int intptr;

// -------------------- Statement

typedef enum {
	SNOP,
	SSEQ,
	SSTRUCT,
	SENUM,
	SFUN,
	SDECL,
	SIF,
	SELSE,
	SASSIGN,
	SRETURN,
	SFOR,
	SCALL,
	SACCESSMOD,
	SIMPORT,
	SBLOCK,
	SEXPRASSIGN,
	SSIGN,
	SMODSIGN,
	SMODIMPL,
	SDECLMODIMPL,
	SMODSKEL,
	SDECLMODSKEL,
	SMODDEF,
	NSTATEMENT,
} EStmt;

typedef EStmt UnknownStmt;

typedef struct {
	EStmt kind;
	intptr ident; // ftident -> char*
	intptr type;  // ftident -> char*
	int ptrlvl;
	Bool cst;
	intptr expr; // ftast -> UnknownExpr*
} SDecl;

typedef struct {
	intptr ident; // ftident -> char*
	intptr type; // ftident -> char*
	int ptrlvl;
} SMember;

typedef struct {
	EStmt kind;
	intptr stmt; // ftast -> UnknownStmt*
} SBlock;

typedef struct {
	EStmt kind;
	intptr ident; // ftident -> char*
	int nmember;
	intptr members; // ftast -> SMember*
} SStruct;

typedef struct {
	EStmt kind;
	intptr ident; // ftident -> char*
	intptr type; // ftident -> char*
	int ptrlvl;
	int nparam;
	intptr params; // ftast -> SMember*
	intptr stmt; // ftast -> UnknownStmt*
} SFun;
typedef SFun SSign;

typedef struct {
	EStmt kind;
	intptr ident; // ftident -> char*
	intptr expr; // ftast -> UnknownExpr*
} SAssign;

typedef struct {
	EStmt kind;
	intptr expr; // ftast -> UnknownExpr*
} SReturn;

typedef struct {
	EStmt kind;
	intptr cond;     // ftast -> UnknownExpr*
	intptr ifstmt;   // ftast -> UnknownStmt*
	intptr elsestmt; // ftast -> UnknownStmt*
} SIf;

typedef struct {
	EStmt kind;
	intptr stmt; // ftast -> UnknownStmt*
	intptr nxt;  // ftast -> UnknownStmt*
} SSeq;

typedef struct {
	EStmt kind;
	intptr stmt1;   // ftast -> UnknownStmt*
	intptr expr;    // ftast -> UnknownExpr*
	intptr stmt2;   // ftast -> UnknownStmt*
	intptr forstmt; // ftast -> UnknownStmt*
} SFor;

typedef struct {
	EStmt kind;
	intptr ident; // ftident -> char*
	int nparam;
	intptr params; // ftast -> [UnknownExpr]
} SCall;

typedef struct {
	EStmt kind;
	intptr mod; // ftident -> char*
	intptr stmt; // ftast -> UnknownStmt*
} SAccessMod;

typedef struct {
	EStmt kind;
	intptr left;  // ftast -> UnknownExpr*
	intptr right; // ftast -> UnknownExpr*
} SExprAssign;

typedef struct {
	EStmt kind;
	intptr lit; // ftlit -> char*
} SImport;

typedef struct {
	EStmt kind;

	// Impl name:
	intptr ident; // ftident -> char*

	// Generic params:
	intptr generics; // ftast -> SGenerics*

	// Module params:
	intptr signatures; // ftast -> SSignatures*

	// Stmts (signature / structs)
	int nstmt;
	intptr stmts; // ftast -> [UnknownStmt]
} SModSign;

typedef struct {
	EStmt kind;

	// Impl name:
	intptr ident; // ftident -> char*

	// Impl what:
	intptr signature; // ftident -> char*

	// Generic params:
	intptr generics; // ftast -> SGenerics*

	// Module params:
	intptr modules; // ftast -> SSignatures*

	// Conv table:
	intptr convtab; // [SConv]
	int nconv;

	// Stmts:
	int nstmt;
	intptr stmts; // ftast -> [UnknownStmt]
} SModImpl;

typedef SModImpl SDeclModImpl;

typedef struct {
	EStmt kind;
	// Impl name:
	intptr ident; // ftident -> char*

	// Impl what:
	intptr signature; // ftident -> char*

	// Stmts
	int nstmt;
	intptr stmts; // ftast -> [UnknownStmt]
} SModSkel;

typedef SModSkel SDeclModSkel;

typedef struct {
	EStmt kind;
	// Impl name:
	intptr ident; // ftident -> char*

	// Generic params:
	intptr generics; // ftast -> SGenerics*

	// Module params:
	intptr modules; // ftast -> SSignatures*

	// Conv table:
	intptr convtab; // ftast -> [SConv]
	int nconv;

	// Impl what:
	intptr skeleton; // ftident -> char*
} SModDef;

typedef struct {
	int ngen;
	intptr generics; // ftast -> [intptr]
} SGenerics;

typedef struct {
	intptr ident; // ftident -> char*
	intptr signature; // ftident -> char*
	intptr generics; // ftast -> *SGenerics
} SSignature;

typedef struct {
	int nsign;
	intptr signs; // -> SSignature*
} SSignatures;

typedef struct {
	intptr gen; // ftident -> char*
	intptr real; // ftident -> char*
} SConv;

typedef struct {
	EStmt kind;
	intptr ident; // ftident -> char*
} SToplevl;

// -------------------- Expression

typedef enum {
	ENONE,
	ECSTI,
	ECSTF,
	ECSTS,
	ETRUE,
	EFALSE,
	EMEM,
	EBINOP,
	EUNOP,
	ECALL,
	EACCESS,
	ESUBSCR,
	ESTRUCT,
	ECAST,
	NEXPR,
} EExpr;

typedef enum {
	OP_MUL,
	OP_DIV,
	OP_MOD,
	OP_ADD,
	OP_SUB,
	OP_LSHIFT,
	OP_RSHIFT,
	OP_LT,
	OP_GT,
	OP_LE,
	OP_GE,
	OP_EQUAL,
	OP_NEQUAL,
	OP_BAND,
	OP_BXOR,
	OP_BOR,
	OP_LAND,
	OP_LOR,
	OP_BNOT,
	OP_LNOT,
	OP_NUM,
} Op;


typedef enum {
	UOP_SUB,
	UOP_LNOT,
	UOP_BNOT,
	UOP_DEREF,
	UOP_AT,
	UOP_SIZEOF,
	UOP_NUM,
} Uop;

typedef EExpr UnknownExpr;

typedef struct {
	EExpr kind;
	intptr addr;
} Csti;

typedef Csti Cstf;
typedef Csti Csts;

typedef struct {
	EExpr  kind;
	intptr addr; // ftast -> UnknownExpr*
	intptr type; // ftident -> char*
	int    ptrlvl;
} Mem;

typedef struct {
	EExpr  kind;
	Op     op;
	intptr left; // ftast -> UnknownExpr*
	intptr right; // ftast -> UnknownExpr*
	intptr type; // ftident -> char*
	int    ptrlvl;
} EBinop;

typedef struct {
	EExpr  kind;
	Uop    op;
	intptr expr; // ftast -> UnknownExpr*
	intptr type; // ftident -> char*
	int    ptrlvl;
} EUnop;

typedef struct {
	EExpr  kind;
	intptr expr; // ftast -> UnknownExpr*
	int    nparam;
	intptr params; // ftast -> [UnknownExpr]
	intptr type; // ftident -> char*
	int    ptrlvl;
} ECall;

typedef struct {
	EExpr  kind;
	intptr expr; // ftast -> UnknownExpr*
	intptr ident; // ftident -> char*
	intptr type; // ftident -> char*
	int    ptrlvl;
} EAccess;

typedef struct {
	EExpr  kind;
	intptr expr; // ftast -> UnknownExpr*
	intptr idxexpr; // ftast -> UnknownExpr*
	intptr type; // ftident -> char*
	int    ptrlvl;
} ESubscr;

typedef struct {
	intptr ident; // ftident -> char*
	intptr expr; // ftast -> UnknownExpr*
	intptr type; // ftident -> char*
	int    ptrlvl;
} EElem;

typedef struct {
	EExpr kind;
	intptr ident; // ftident -> char*
	int nelem;
	intptr elems; // ftast -> [EElem]
} EStruct;

typedef struct {
	EExpr kind;
	intptr expr; // ftast -> UnknownExpr*
	intptr type; // ftident -> char*
	int ptrlvl;
} ECast;

// Symbols

typedef struct {
	intptr ident; // ftident -> char*
	intptr stmt; // ftast -> UnknownStmt*
} Symbol;

typedef struct {
	intptr array; // Symbol*
	int nsym;
} Symbols;

typedef struct {
	intptr stmts; // [UnknownStmt]
	int nstmt;
} StmtArray;

// -------------------- Functions

int parse_toplevel(const ETok *t, intptr *stmt, int *pub);
void printexpr(FILE *fd, intptr expr);
void printstmt(FILE *fd, intptr stmt);
int parse_tokens(const ETok *t, Symbols *signatures, Symbols *identsym, Symbols *typesym, Symbols modsym[NMODSYM], StmtArray *pubsym, StmtArray *imports);

extern const char *uopstrs[UOP_NUM];
extern const char *stmtstrs[NSTATEMENT];
extern const char *opstrs[OP_NUM];
extern const char *exprstrs[NEXPR];
extern const char *modsymstrs[NMODSYM];

#endif /* PARSER_H */

