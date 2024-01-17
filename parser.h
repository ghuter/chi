#ifndef PARSER_H
#define PARSER_H

// extern variables

extern FatArena ftast;
extern int line;

typedef int intptr;

// -------------------- Statement

typedef enum {
	NOP,
	SSEQ,
	SSTRUCT,
	SENUM,
	SINTERFACE,
	SFUN,
	SDECL,
	SIF,
	SELSE,
	SASSIGN,
	SRETURN,
	NSTATEMENT,
} Stmt;

typedef Stmt UnknownStmt;

typedef struct {
	Stmt type;
	intptr ident;
	intptr itype;
	int cst;
	intptr expr;
} Decl;

typedef struct {
	intptr ident;
	intptr type;
} Member;

typedef struct {
	Stmt type;
	intptr ident;
	int nmember;
	intptr members; // (Member*)
} Struct;

typedef struct {
	Stmt type;
	intptr ident;
	intptr ret;
	int nparam;
	intptr params; // (Member*)
	intptr stmt;
} Fun;

typedef struct {
	Stmt type;
	intptr ident;
	intptr expr;
} Assign;

typedef struct {
	Stmt type;
	intptr expr;
} Return;

typedef struct {
	Stmt type;
	intptr cond;
	intptr ifstmt;   // UnknownStmt*
	intptr elsestmt; // UnknownStmt*
} If;

typedef struct {
	Stmt type;
	intptr stmt;  // UnknownStmt*
	intptr nxt; // UnknownStmt*
} Seq;

// -------------------- Expression

typedef enum {
	ENONE,
	ECSTI,
	ECSTF,
	ECSTS,
	EMEM,
	EBINOP,
	EUNOP,
	ECALL,
	EPAREN,
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
	UOP_BXOR,
	UOP_AT,
	UOP_SIZEOF,
	UOP_NUM,
} Uop;

typedef EExpr UnknownExpr;

typedef struct {
	EExpr type;
	intptr addr;
} Csti;

typedef Csti Cstf;
typedef Csti Csts;
typedef Csti Mem;

typedef struct {
	EExpr type;
	Op op;
	intptr left;
	intptr right;
} Binop;

typedef struct {
	EExpr type;
	Uop op;
	intptr expr;
} Unop;

typedef struct {
	EExpr type;
	intptr ident;
	int nparam;
	intptr params;
} Call;

typedef struct {
	EExpr type;
	intptr expr;
} Paren;

int parse_toplevel(const ETok *t);
void printexpr(FILE *fd, intptr expr);
void printstmt(FILE *fd, intptr stmt);

#endif /* PARSER_H */

