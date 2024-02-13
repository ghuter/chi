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
	SMODSKEL,
	SMODDEF,
	NSTATEMENT,
} EStmt;

typedef EStmt UnknownStmt;

typedef struct {
	EStmt kind;
	intptr ident;
	intptr type;
	int ptrlvl;
	Bool cst;
	intptr expr;
} SDecl;

typedef struct {
	intptr ident;
	intptr type;
	int ptrlvl;
} SMember;

typedef struct {
	EStmt kind;
	intptr stmt;
} SBlock;

typedef struct {
	EStmt kind;
	intptr ident;
	int nmember;
	intptr members; // (SMember*)
} SStruct;

typedef struct {
	EStmt kind;
	intptr ident;
	intptr type; // Return type
	int ptrlvl;
	int nparam;
	intptr params; // (SMember*)
	intptr stmt;
} SFun;
typedef SFun SSign;

typedef struct {
	EStmt kind;
	intptr ident;
	intptr expr;
} SAssign;

typedef struct {
	EStmt kind;
	intptr expr;
} SReturn;

typedef struct {
	EStmt kind;
	intptr cond;     // UnknownExpr*
	intptr ifstmt;   // UnknownStmt*
	intptr elsestmt; // UnknownStmt*
} SIf;

typedef struct {
	EStmt kind;
	intptr stmt; // UnknownStmt*
	intptr nxt;  // UnknownStmt*
} SSeq;

typedef struct {
	EStmt kind;
	intptr stmt1;   // UnknownStmt*
	intptr expr;    // UnknownExpr*
	intptr stmt2;   // UnknownStmt*
	intptr forstmt; // UnknownStmt*
} SFor;

typedef struct {
	EStmt kind;
	intptr ident;
	int nparam;
	intptr params; // [Expr*]
} SCall;

typedef struct {
	EStmt kind;
	intptr mod;
	intptr stmt; // UnknownStmt*
} SAccessMod;

typedef struct {
	EStmt kind;
	intptr left;  // UnknownExpr*
	intptr right; // UnknownExpr*
} SExprAssign;

typedef struct {
	EStmt kind;
	intptr ident;
} SImport;

typedef struct {
	EStmt kind;

	// Impl name:
	intptr ident;

	// Generic params:
	intptr generics;

	// Module params:
	intptr signatures;

	// Stmts (signature / structs)
	int nstmt;
	intptr stmts; // [UnknownStmt]
} SModSign;

typedef struct {
	EStmt kind;

	// Impl name:
	intptr ident;

	// Impl what:
	intptr signature;

	// Generic params:
	intptr generics;

	// Module params:
	intptr modules;

	// Conv table:
	intptr convtab; // [SConv]
	int nconv;

	// Stmts:
	int nstmt;
	intptr stmts; // [UnknownStmt]
} SModImpl;

typedef struct {
	EStmt kind;
	// Impl name:
	intptr ident;

	// Impl what:
	intptr signature;

	// Stmts
	int nstmt;
	intptr stmts;
} SModSkel;

typedef struct {
	EStmt kind;
	// Impl name:
	intptr ident;

	// Generic params:
	intptr generics;

	// Module params:
	intptr modules;

	// Conv table:
	intptr convtab; // [SConv]
	int nconv;

	// Impl what:
	intptr skeleton;
} SModDef;

typedef struct {
	int ngen;
	intptr generics; // intptr *ident;
} SGenerics;

typedef struct {
	intptr ident;
	intptr signature;
	intptr generics; // *SGenerics
} SSignature;

typedef struct {
	int nsign;
	intptr signs; // -> SSignature*
} SSignatures;

typedef struct {
	intptr gen;
	intptr real;
} SConv;

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
	intptr addr;
	intptr type;
	int    ptrlvl;
} Mem;

typedef struct {
	EExpr  kind;
	Op     op;
	intptr left;
	intptr right;
	intptr type;
	int    ptrlvl;
} EBinop;

typedef struct {
	EExpr  kind;
	Uop    op;
	intptr expr;
	intptr type;
	int    ptrlvl;
} EUnop;

typedef struct {
	EExpr  kind;
	intptr expr;
	int    nparam;
	intptr params; // [Expr*]
	intptr type;
	int    ptrlvl;
} ECall;

typedef struct {
	EExpr  kind;
	intptr expr;
	intptr ident;
	intptr type;
	int    ptrlvl;
} EAccess;

typedef struct {
	EExpr  kind;
	intptr expr;
	intptr idxexpr;
	intptr type;
	int    ptrlvl;
} ESubscr;

typedef struct {
	intptr ident;
	intptr expr;
	intptr type;
	int    ptrlvl;
} EElem;

typedef struct {
	EExpr kind;
	intptr ident;
	int nelem;
	intptr elems; // EElem*
} EStruct;

// Symbols

typedef struct {
	intptr ident; // Char*
	intptr stmt; // UnknownStmt*
} Symbol;

typedef struct {
	intptr array; // Symbol*
	int nsym;
} Symbols;

// -------------------- Functions

int parse_toplevel(const ETok *t, intptr *stmt);
void printexpr(FILE *fd, intptr expr);
void printstmt(FILE *fd, intptr stmt);
int parse_tokens(const ETok *t, Symbols *signatures, Symbols *identsym, Symbols *typesym, Symbols modsym[NMODSYM]);


extern const char *uopstrs[UOP_NUM];
extern const char *stmtstrs[NSTATEMENT];
extern const char *opstrs[OP_NUM];
extern const char *exprstrs[NEXPR];
extern const char *modsymstrs[NMODSYM];

#endif /* PARSER_H */

