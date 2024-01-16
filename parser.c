#include <assert.h>
#include <string.h>

#include "lib/token.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define TODO(message) \
    do { \
        fprintf(stderr, "TODO(%s): %s\n", TOSTRING(__LINE__), message); \
    } while (0)

typedef int intptr;

static int line = 0;
FatArena fttok = {0};
FatArena ftast = {0};

#define BURNSEPARATORS(_t, _i)                 \
	do {                                       \
		while (1) {                            \
			if (_t[_i] == NEWLINE) {           \
				line++;                        \
				_i++;                          \
			} else if (_t[_i] == SEMICOLON) {  \
				_i++;                          \
			}                                  \
			else {                             \
				break;                         \
			}                                  \
		}                                      \
	} while (0)

#define ISTOK(_t, _eoe, _res)                  \
do {                                           \
	int _i = 0;                                \
	_res = 0;                                  \
	while (_eoe[_i] != UNDEFINED) {            \
		if ((_t) == _eoe[_i]) {                \
			_res = 1;                          \
			break;                             \
		}                                      \
		_i++;                                  \
	}                                          \
} while (0)

#define SAVECST(_dest, _type, _addr)             \
do {                                             \
	intptr _tmp = ftalloc(&ftast, sizeof(Csti)); \
	*_dest = _tmp;                               \
	Csti *_csti = (Csti *)ftptr(&ftast, _tmp);   \
	_csti->addr = _addr;                         \
	_csti->type = _type;                         \
} while (0)

#define ISMONADICOP(_op) (_op == SUB || _op == LNOT || _op == BNOT || _op == BXOR || _op == AT || _op == SIZEOF)
#define ISBINOP(_op) (_op >= MUL && _op <= LNOT)
#define TOKTOOP(_t) ((_t) - MUL)
#define PRINTI() fprintf(stderr, "i(%d): %d\n", __LINE__, i)


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

static const char *stmtstrs[NSTATEMENT] = {
	[NOP]        = "NOP",
	[SSEQ]       = "SSEQ",
	[SSTRUCT]    = "SSTRUCT",
	[SENUM]      = "SENUM",
	[SINTERFACE] = "SINTERFACE",
	[SFUN]       = "SFUN",
	[SDECL]      = "SDECL",
	[SIF]        = "SIF",
	[SELSE]      = "SELSE",
	[SASSIGN]    = "SASSIGN",
	[SRETURN]    = "SRETURN",
};

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

typedef Stmt UnknownStmt;

void printexpr(FILE *fd, intptr expr);
int parse_expression(const ETok *t, const ETok *eoe, intptr *expr);
int parse_fun_stmts(const ETok *t, intptr *stmt);

void
printstmt(FILE *fd, intptr stmt)
{
	UnknownStmt *ptr = (UnknownStmt*) ftptr(&ftast, stmt);
	if (stmt == -1) {
		fprintf(fd, "NOP");
		return;
	}

	switch (*ptr) {
	case NOP:
		fprintf(fd, "NOP");
		break;
	case SSTRUCT: {
		Struct *s = (Struct*) ptr;
		fprintf(fd, "%s(%s", stmtstrs[*ptr], (char*) ftptr(&ftident, s->ident));

		Member* members = (Member*) ftptr(&ftast, s->members);
		for (int i = 0; i < s->nmember; i++) {
			fprintf(fd, ", <%s : %s>", (char*) ftptr(&ftident, members->ident), (char*) ftptr(&ftident, members->type));
			members++;
		}
		fprintf(fd, ")");
		break;
	}
	case SDECL: {
		Decl *decl = (Decl*) ptr;
		char *type = decl->itype ==  -1 ? "UNKNOWN" : (char*) ftptr(&ftident, decl->itype);
		fprintf(fd, "%s(%s : %s, ", stmtstrs[*ptr], (char*) ftptr(&ftident, decl->ident), type);
		printexpr(fd, decl->expr);
		fprintf(fd, ")");
		break;
	}
	case SFUN: {
		Fun *fun = (Fun*) ptr;
		fprintf(fd, "%s(%s", stmtstrs[*ptr], (char*)ftptr(&ftident, fun->ident));

		Member* members = (Member*) ftptr(&ftast, fun->params);
		for (int i = 0; i < fun->nparam; i++) {
			fprintf(fd, ", %s : %s", (char*) ftptr(&ftident, members->ident), (char*) ftptr(&ftident, members->type));
			members++;
		}

		char *ret = fun->ret ==  -1 ? "UNKNOWN" : (char*) ftptr(&ftident, fun->ret);
		fprintf(fd, ", -> %s, {", ret);
		printstmt(fd, fun->stmt);
		fprintf(fd, "}");
		fprintf(fd, ")");

		break;
	}
	case SRETURN: {
		Return *ret = (Return*) ptr;
		fprintf(fd, "%s(", stmtstrs[*ptr]);
		printexpr(fd, ret->expr);
		fprintf(fd, ")");
		break;
	}
	case SASSIGN: {
		Assign *assign = (Assign*) ptr;
		fprintf(fd, "%s(%s, ", stmtstrs[*ptr], (char*) ftptr(&ftident, assign->ident));
		printexpr(fd, assign->expr);
		fprintf(fd, ")");
		break;
	}
	case SIF: {
		If *_if = (If*) ptr;
		fprintf(fd, "%s(", stmtstrs[*ptr]);
		printexpr(fd, _if->cond);
		fprintf(fd, ", ");
		printstmt(fd, _if->ifstmt);

		if (_if->elsestmt > 0) {
			fprintf(fd, ", ");
			printstmt(fd, _if->elsestmt);
		}

		break;
	}
	case SSEQ: {
		Seq *seq = (Seq*) ptr;
		fprintf(fd, "%s(", stmtstrs[*ptr]);
		printstmt(fd, seq->stmt);
		fprintf(fd, ", ");
		printstmt(fd, seq->nxt);
		fprintf(fd, ")");
		break;
	}
	default:
		fprintf(fd, "ERR: Unreachable statement in printstmt.\n");
	}
}

int
parse_param(const ETok *t, Fun* fun)
{
	int i = 0;
	if (t[i] != LPAREN) {
		fprintf(stderr, "ERR: Unexpected token <%s> when parsing the functions param.\n Expected `fun` `(`<param> `)`\n", tokenstrs[t[i]]);
		return -1;
	}
	i++;


	intptr maddr = ftalloc(&ftast, sizeof(Member));
	fun->params = maddr;
	fun->nparam = 0;
	Member* member = (Member*) ftptr(&ftast, maddr);

	while (t[i] != RPAREN) {
		int ident;
		int type;
		fun->nparam = 1;

		if (t[i] != IDENTIFIER) {
			fprintf(stderr, "ERR: Unexpected token <%s> when parsing the functions param.\n|> Expected a param: <IDENTIFIER> `:` <IDENTIFIER>\n", tokenstrs[t[i]]);
			return -1;
		}
		i++;
		ident = t[i];
		i++;

		if (t[i] != COLON) {
			fprintf(stderr, "ERR: Unexpected token <%s> when parsing the functions param.\n|> Expected a param: <IDENTIFIER> `:` <IDENTIFIER>\n", tokenstrs[t[i]]);
			return -1;
		}
		i++;

		if (t[i] != IDENTIFIER) {
			fprintf(stderr, "ERR: Unexpected token <%s> when parsing the functions param.\n|> Expected a param: <IDENTIFIER> `:` <IDENTIFIER>\n", tokenstrs[t[i]]);
			return -1;
		}
		i++;
		type = t[i];
		i++;

		member->type = type;
		member->ident = ident;

		if (t[i] != COMMA && t[i] != RPAREN) {
			fprintf(stderr, "ERR: Unexpected token <%s> when parsing the functions param.\n|> Expected a `,` or `)`.\n", tokenstrs[t[i]]);
			return -1;
		}

		if (t[i] == COMMA) {
			i++;
			member++;
			ftalloc(&ftast, sizeof(Member));
		}
	}

	i++;
	return i;
}

int
parse_fun_stmt(const ETok *t, intptr *stmt)
{
	const ETok eoe[] = {SEMICOLON, RBRACES, NEWLINE, UNDEFINED};
	int i = 0;
	int res = -1;

	switch (t[i]) {
	case RETURN: {
		i++;
		intptr saddr = ftalloc(&ftast, sizeof(Return));
		*stmt = saddr;

		Return *ret = (Return*) ftptr(&ftast, saddr);
		ret->type = SRETURN;

		res = parse_expression(t + i, eoe, &ret->expr);
		if (res < 0) {
			fprintf(stderr, "ERR: parse expression in a function.\n");
			return -1;
		}
		i += res;

		return i;
		break;
	}
	case IDENTIFIER:
		i++;
		int ident = t[i];
		i++;

		if (t[i] >= ASSIGN && t[i] <= MOD_ASSIGN) {
			i++;

			intptr aaddr = ftalloc(&ftast, sizeof(Assign));
			*stmt = aaddr;

			Assign *assign = (Assign*) ftptr(&ftast, aaddr);
			assign->type = SASSIGN;
			assign->ident = ident;

			res = parse_expression(t + i, eoe, &assign->expr);
			if (res < 0) {
				fprintf(stderr, "ERR: parse expression in a function.\n");
				return -1;
			}
			i += res;

			return i;
		}

		int cst = -1;
		int itype = -1;

		if (t[i] != COLON) {
			fprintf(stderr, "ERR: Unexpected token <%s> at the beginning of a statement.\n|> Expects `:`\n", tokenstrs[t[i]]);
			return -1;
		}
		i++;

		if (t[i] == IDENTIFIER) {
			i++;
			itype = t[i];
			i++;
		}

		if (t[i] == COLON) {
			i++;
			cst = 1;
		} else if (t[i] == ASSIGN) {
			i++;
			cst = 0;
		} else {
			fprintf(stderr, "ERR: Unexpected token <%s> in a declaration statement.\n", tokenstrs[t[i]]);
			return -1;
		}

		intptr daddr = ftalloc(&ftast, sizeof(Decl));
		*stmt = daddr;

		Decl *decl = (Decl*) ftptr(&ftast, daddr);
		decl->type = SDECL;
		decl->cst = cst;
		decl->itype = itype;
		decl->ident = ident;

		res = parse_expression(t + i, eoe, &decl->expr);
		if (res < 0) {
			fprintf(stderr, "ERR: parsing expression in a declaration statement\n");
			return -1;
		}
		i += res;

		return i;
		break;
	case IF: {
		const ETok eoe[] = {LBRACES, UNDEFINED};
		TODO("IF");
		i++;
		intptr iaddr = ftalloc(&ftast, sizeof(If));
		*stmt = iaddr;

		If *_if = (If*) ftptr(&ftast, iaddr);
		_if->type = SIF;

		res = parse_expression(t + i, eoe, &_if->cond);
		if (res < 0) {
			fprintf(stderr, "ERR: parsing expression in a declaration statement\n");
			return -1;
		}
		i += res;

		intptr ifstmt = -1;
		intptr elsestmt = -1;
		res = parse_fun_stmts(t + i, &ifstmt);
		if (res < 0) {
			fprintf(stderr, "ERR: parsing statements in a if statement\n");
			return -1;
		}
		i += res;
		
		if (t[i] != RBRACES) {
			fprintf(stderr, "ERR: Unexpected token <%s>, expects a `}`\n", tokenstrs[t[i]]);
		}
		i++;

		if (t[i] == ELSE) {
			i++;
			res = parse_fun_stmts(t + i, &elsestmt);
			if (res < 0) {
				fprintf(stderr, "ERR: parsing statements in a else statement\n");
				return -1;
			}
			i += res;

			if (t[i] != RBRACES) {
				fprintf(stderr, "ERR: Unexpected token <%s>, expects a `}`\n", tokenstrs[t[i]]);
			}
			i++;
		}

		_if->ifstmt = ifstmt;
		_if->elsestmt = elsestmt;

		return i;
		break;
	}
	default:
		TODO("OTHERS");
	}

	fprintf(stderr, "ERR: Unreachable when parsing statement\n");
	return -1;
}

int
parse_fun_stmts(const ETok *t, intptr *stmt) {
	TODO("fun stmts");

	int i = 0;
	int res = -1;

	if (t[i] != LBRACES) {
		fprintf(stderr, "ERR: Unexpected token <%s>, `{` is expected for a group of statement.\n", tokenstrs[t[i]]);
		return -1;
	}
	i++;

	intptr saddr = ftalloc(&ftast, sizeof(Seq));
	*stmt = saddr;

	Seq *seq = (Seq*) ftptr(&ftast, saddr);
	seq->type = SSEQ;
	seq->stmt = -1;
	seq->nxt = -1;

	while (t[i] != RBRACES) {
		res = parse_fun_stmt(t + i, &seq->stmt);
		if (res < 0) {
			fprintf(stderr, "ERR: parsing fun statements\n");
			return -1;
		}
		i += res;

		if (t[i] != RBRACES && t[i] != NEWLINE && t[i] != SEMICOLON) {
			fprintf(stderr, "ERR: Unexpected token <%s>, expects `}` or `;` or `\\n`.\n", tokenstrs[t[i]]);
			return -1;
		} 
		
		if (t[i] == NEWLINE) {
			line++;
			i++;
			intptr addr = ftalloc(&ftast, sizeof(Seq));
			seq->nxt=addr;
			seq = (Seq*) ftptr(&ftast, addr);
			seq->type = SSEQ;
			seq->stmt= -1;
			seq->nxt = -1;
		}

		if (t[i] == SEMICOLON) {
			i++;
			intptr addr = ftalloc(&ftast, sizeof(Seq));
			seq->nxt=addr;
			seq = (Seq*) ftptr(&ftast, addr);
			seq->type = SSEQ;
			seq->stmt= -1;
			seq->nxt = -1;
		}
	}

	return i;
}

int
parse_toplevel_fun(const ETok *t, intptr ident, intptr *stmt)
{
	TODO("Toplevel fun");

	int i = 0;
	int res = -1;

	intptr saddr = ftalloc(&ftast, sizeof(Fun));
	*stmt = saddr;

	Fun *fun = (Fun*) ftptr(&ftast, saddr);
	fun->ident = ident;
	fun->type = SFUN;
	fun->ret = -1;
	fun->stmt= -1;

	res = parse_param(t + i, fun);
	if (res < 0) {
		fprintf(stderr, "ERR: parsing params\n");
		return -1;
	}
	i += res;

	if (t[i] == IDENTIFIER) {
		i++;
		fun->ret = t[i];
		i++;
	}

	res = parse_fun_stmts(t + i, &fun->stmt);
	if (res < 0) {
		fprintf(stderr, "ERR: parsing fun statements\n");
		return -1;
	}
	i += res;

	return i;
}

int
parse_toplevel_interface(const ETok *t)
{
	(void)t;
	TODO("Toplevel interface");
	return -1;
}

int
parse_toplevel_struct(const ETok *t, intptr ident, intptr *stmt)
{
	TODO("Toplevel struct");
	int i = 0;

	intptr saddr = ftalloc(&ftast, sizeof(Struct));
	*stmt = saddr;
	Struct *s = (Struct*) ftptr(&ftast, saddr);

	s->type = SSTRUCT;
	s->ident = ident;
	s->nmember = 0;

	intptr current = ftalloc(&ftast, sizeof(Member));
	s->members = current;

	if (t[i] != LBRACES) {
		fprintf(stderr, "ERR: after 'struct' a '{' is expected\n");
		return -1;
	}
	i++;

	while (t[i] == NEWLINE) {
		i++;
		line++;
	}

	while (1) {
		int ident = -1;
		int type = -1;
		Member *member = (Member*)ftptr(&ftast, current);
		s->nmember++;

		if (t[i] == IDENTIFIER) {
			i++;
			ident = t[i];
			i++;
		} else {
			goto err;
		}

		if (t[i] == COLON) {
			i++;
		} else {
			goto err;
		}

		if (t[i] == IDENTIFIER) {
			i++;
			type = t[i];
			i++;
		} else {
			goto err;
		}

		member->ident = ident;
		member->type = type;
		current = ftalloc(&ftast, sizeof(Member));

		if (t[i] == RBRACES) {
			break;
		}

		if (t[i] == NEWLINE || t[i] == SEMICOLON) {
			BURNSEPARATORS(t, i);
		} else {
			fprintf(stderr, "ERR: line %d, expects a separator <NEWLINE> | <SEMICOLON>\n", line);
			return -1;
		}

		if (t[i] == RBRACES) {
			break;
		}
	}

	return i;

err:
	fprintf(stderr, "ERR: line %d, unexepcted Token <%s>, expects struct { <IDENTIFIER> ':' <IDENTIFIER> }\n", line, tokenstrs[t[i]]);
	return -1;
}

int
parse_toplevel_enum(const ETok *t)
{
	(void)t;
	TODO("Toplevel enum");
	return -1;
}

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

char *exprstrs[NEXPR] = {
	[ENONE]   = "ENONE",
	[ECSTI]   = "ECSTI",
	[ECSTF]   = "ECSTF",
	[ECSTS]   = "ECSTS",
	[EMEM]    = "EMEM",
	[EBINOP]  = "EBINOP",
	[EUNOP]   = "EUNOP",
	[ECALL]   = "ECALL",
	[EPAREN]  = "EPAREN",
};

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

const char *opstrs[OP_NUM] = {
	[OP_MUL]    = "MUL",
	[OP_DIV]    = "DIV",
	[OP_MOD]    = "MOD",
	[OP_ADD]    = "ADD",
	[OP_SUB]    = "SUB",
	[OP_LSHIFT] = "LSHIFT",
	[OP_RSHIFT] = "RSHIFT",
	[OP_LT]     = "LT",
	[OP_GT]     = "GT",
	[OP_LE]     = "LE",
	[OP_GE]     = "GE",
	[OP_EQUAL]  = "EQUAL",
	[OP_NEQUAL] = "NEQUAL",
	[OP_BAND]   = "BAND",
	[OP_BXOR]   = "BXOR",
	[OP_BOR]    = "BOR",
	[OP_LAND]   = "LAND",
	[OP_LOR]    = "LOR",
	[OP_BNOT]   = "BNOT",
	[OP_LNOT]   = "LNOT",
};

typedef enum {
	UOP_SUB,
	UOP_LNOT,
	UOP_BNOT,
	UOP_BXOR,
	UOP_AT,
	UOP_SIZEOF,
	UOP_NUM,
} Uop;

const char *uopstrs[UOP_NUM] = {
	[UOP_SUB]    = "SUB",
	[UOP_LNOT]   = "LNOT",
	[UOP_BNOT]   = "BNOT",
	[UOP_BXOR]   = "BXOR",
	[UOP_AT]     = "AT",
	[UOP_SIZEOF] = "SIZEOF",
};

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

void
printexpr(FILE* fd, intptr expr)
{
	UnknownExpr* ptr = (UnknownExpr*) ftptr(&ftast, expr);
	switch (*ptr) {
	case ENONE:
		fprintf(fd, "<NONE>\n");
		return;
		break;
	case ECSTI: {
		Csti *csti = (Csti*) ptr;
		fprintf(fd, "%s(%ld)", exprstrs[*ptr], *((int64_t*) ftptr(&ftimmed, csti->addr)));
		return;
		break;
	}
	case ECSTF: {
		Cstf *cstf = (Cstf*) ptr;
		fprintf(fd, "%s(%f)>", exprstrs[*ptr], *((double*) ftptr(&ftimmed, cstf->addr)));
		return;
		break;
	}
	case ECSTS: {
		Csts *csts = (Csts*) ptr;
		fprintf(fd, "%s(%s)", exprstrs[*ptr], (char*) ftptr(&ftlit, csts->addr));
		return;
		break;
	}
	case EMEM: {
		Mem *mem = (Mem*) ptr;
		fprintf(fd, "%s(%s)", exprstrs[*ptr], (char*) ftptr(&ftident, mem->addr));
		return;
		break;
	}
	case EBINOP: {
		Binop *binop = (Binop*) ptr;
		fprintf(fd, "%s(%s, ", exprstrs[*ptr], opstrs[binop->op]);
		printexpr(fd, binop->left);
		fprintf(fd, ", ");
		printexpr(fd, binop->right);
		fprintf(fd, ")");
		return;
		break;
	}
	case EUNOP: {
		Unop *unop = (Unop*) ptr;
		fprintf(fd, "%s(%s, ", exprstrs[*ptr], uopstrs[unop->op]);
		printexpr(fd, unop->expr);
		fprintf(fd, ")");
		return;
		break;
	}
	case ECALL: {
		Call *call = (Call*) ptr;
		intptr *paramtab = (intptr*) ftptr(&ftast, call->params);

		fprintf(fd, "%s(%s", exprstrs[*ptr], (char*) ftptr(&ftident, call->ident));
		for (int i = 0; i < call->nparam; i++) {
			fprintf(fd, ", ");
			printexpr(fd, paramtab[i]);
		}
		fprintf(fd, ")");
		return;
		break;
	}
	case EPAREN: {
		Paren *paren = (Paren*) ptr;
		fprintf(fd, "%s(", exprstrs[*ptr]);
		printexpr(fd, paren->expr);
		fprintf(fd, ")");
		return;
		break;
	}
	default:
		assert(1 || "Unreachable enum");
	}
}

int
parse_call(const ETok *t, intptr ident, intptr *d)
{
	const ETok eoe[3] = {COMMA, RPAREN, UNDEFINED};
	int i = 0;
	int res = -1;

	// Case it's only an identifier
	if (t[i] != LPAREN) {
		return 0;
	}

	// Case it's a function call
	i++;

	// Alloc and save a function expression
	intptr caddr = ftalloc(&ftast, sizeof(Call));
	*d = caddr;

	intptr params[64];
	int nparam = 0;

	// Parse the params
	while (t[i] != RPAREN) {
		res = parse_expression(t + i, eoe, params + nparam);
		if (res < 0) {
			goto err;
		}
		i += res;
		nparam++;

		if (!(t[i] == COMMA || t[i] == RPAREN)) {
			fprintf(stderr, "The param separator in a function call must be a <COMMA>\n");
			goto err;
		}
		if (t[i] == COMMA) {
			i++;
		}
	}

	// consume RPAREN
	i++;

	// Save the params
	Call *call = (Call*) ftptr(&ftast, caddr);
	call->type = ECALL;
	call->ident = ident;
	call->nparam = nparam;
	call->params = ftalloc(&ftast, nparam * sizeof(intptr));
	intptr *cparams = (intptr*) ftptr(&ftast, call->params);
	memcpy(cparams, params, sizeof(intptr) * nparam);

	return i;

err:
	fprintf(stderr, "ERR: error in the parsing of a function param\n");
	return -1;
}

int
parse_unop(const ETok *t, const ETok *eoe, intptr *expr)
{
	int i = 0;
	int add = 0;
	int ires = -1;

	switch (t[i]) {
	case IDENTIFIER: {
		i++;
		EExpr type = EMEM;
		intptr addr = t[i];
		i++;

		// case FUNCALL
		ires = parse_call(t + i, addr, expr);
		if (ires < 0) {
			fprintf(stderr, "ERR: error in the parsing of an expression\n");
			return -1;
		} else if (ires != 0) {
			i += ires;
			break;
		}

		// case IDENTIFIER
		SAVECST(expr, type, addr);
		break;
	}
	// Cst
	case LITERAL:
		add++;
	//fallthrough
	case FLOAT:
		add++;
	// fallthrough
	case INT: {
		i++;
		EExpr type = ECSTI + add;
		intptr addr = t[i];
		i++;
		SAVECST(expr, type, addr);
		break;
	}

	// Unop
	case SIZEOF:
		add++;
	// fallthrough
	case AT:
		add++;
	// fallthrough
	case BXOR:
		add++;
	// fallthrough
	case BNOT:
		add++;
	// fallthrough
	case LNOT:
		add++;
	// fallthrough
	case SUB: {
		// Alloc unop
		intptr uaddr = ftalloc(&ftast, sizeof(Unop));
		*expr = uaddr;

		// Save unop
		Unop *unop = (Unop*) ftptr(&ftast, uaddr);
		unop->type = EUNOP;
		unop->op = add;

		// Parse the following expression
		intptr nxt = 0;
		i++;
		ires = parse_unop(t + i, eoe, &nxt);
		if (ires < 0) {
			return -1;
		}
		i += ires;
		unop->expr = nxt;
		break;
	}
	case LPAREN: {
		const ETok eoeparen[] = {RPAREN, UNDEFINED};
		// Alloc paren
		intptr paddr = ftalloc(&ftast, sizeof(Paren));
		*expr = paddr;

		// Parse the expression
		i++;
		intptr res = -1;
		ires = parse_expression(t + i, eoeparen, &res);
		if (ires < 0) {
			return -1;
		}
		i += ires;

		// Save paren
		Paren *paren = (Paren*) ftptr(&ftast, paddr);
		paren->type = EPAREN;
		paren->expr = res;


		// Consume RPAREN
		i++;
		break;
	}
	default:
		fprintf(stderr, "ERR(%d): line %d, unexpected token <%s>\n", __LINE__, line, tokenstrs[t[i]]);
		return -1;
	}

	return i;
}

#define LEVEL 10

void
printtoklst(const ETok *t)
{
	int i = 0;
	while (t[i] != UNDEFINED) {
		fprintf(stderr, " %s ", tokenstrs[t[i]]);
		i++;
	}
	fprintf(stderr, "\n");
}

static const ETok tokbylvl[][LEVEL] = {
	{UNDEFINED},
	{MUL, DIV, MOD, UNDEFINED},
	{ADD, SUB, UNDEFINED},
	{LSHIFT, RSHIFT, UNDEFINED},
	{LT, LE, GT, GE, UNDEFINED},
	{EQUAL, NEQUAL, UNDEFINED},
	{BAND, UNDEFINED},
	{BXOR, UNDEFINED},
	{BOR, UNDEFINED},
	{LAND, UNDEFINED},
	{LOR, UNDEFINED},
};

int
parse_binop(const ETok *t, const ETok *eoe, intptr *expr, int lvl)
{
	int i = 0;
	int res = -1;
	intptr left = -1;
	intptr right = -1;

	if (lvl > 1) {
		res =  parse_binop(t + i, eoe, &left, lvl - 1);
	} else {
		res = parse_unop(t + i, eoe, &left);
	}

	if (res < 0) {
		return -1;
	}
	i += res;

	ISTOK((t[i]), (tokbylvl[lvl]), res);
	if (res == 0) {
		// Not a binop
		*expr = left;
		return i;
	}

	// Alloc a binop
	intptr baddr = ftalloc(&ftast, sizeof(Binop));
	Binop *binop = (Binop*) ftptr(&ftast, baddr);
	binop->type = EBINOP;
	binop->op = TOKTOOP(t[i]);

	// Save in the return variable
	*expr = baddr;

	// Parse the right
	i++;
	res = parse_binop(t + i, eoe, &right, lvl);
	if (res < 0) {
		return -1;
	}
	i += res;

	// Save the parsed expression
	binop->left = left;
	binop->right = right;

	return i;
}

int
parse_expression(const ETok *t, const ETok *eoe, intptr *expr)
{
	int i = 0;
	int res = -1;

	res = parse_binop(t, eoe, expr, LEVEL);
	if (res < 0) {
		return -1;
	}
	i += res;

	ISTOK((t[i]), eoe, res);
	if (res == 0) {
		fprintf(stderr, "parse_expression doesn't consume the all expression (eoe doesn't match with t)\n");
		fprintf(stderr, "Tok(%d): %s, Eoe:", t[i], tokenstrs[t[i]]);
		printtoklst(eoe);
		return -1;
	}
	return i;
}

int
parse_toplevel_decl(const ETok *t, intptr ident, intptr *stmt)
{
	int res = -1;
	int i = 0;
	int type = -1;
	int cst = -1;

	if (t[i] != COLON) {
		fprintf(stderr, "ERR: line %d, declaration: <IDENTIFIER> : <IDENTIFIER> : EXPR\n", line);
		return 0;
	}
	i++;

	if (t[i] == IDENTIFIER) {
		i++;
		type = i;
		i++;
	}

	if (t[i] == COLON) {
		cst = 1;
	} else if (t[i] == ASSIGN) {
		cst = 0;
	} else {
		fprintf(stderr, "ERR: line %d, declaration: <IDENTIFIER> : <IDENTIFIER> : EXPR", line);
		return 0;
	}
	i++;

	if (t[i] == STRUCT) {
		i++;
		res = parse_toplevel_struct(t + i, ident, stmt);
		if (res < 0) {
			fprintf(stderr, "ERR: error toplevel\n");
			return -1;
		}
		i += res;
	} else if (t[i] == ENUM) {
		i++;
		res = parse_toplevel_enum(t + i);
		if (res < 0) {
			fprintf(stderr, "ERR: error toplevel\n");
			return -1;
		}
		i += res;
	} else if (t[i] == FUN) {
		i++;
		res = parse_toplevel_fun(t + i, ident, stmt);
		if (res < 0) {
			fprintf(stderr, "ERR: error toplevel fun\n");
			return -1;
		}
		i += res;
	} else if (t[i] == INTERFACE) {
		i++;
		res = parse_toplevel_interface(t + i);
		if (res < 0) {
			fprintf(stderr, "ERR: error toplevel\n");
			return -1;
		}
		i += res;
	} else { /* Expression */
		const ETok eoe[3] = {SEMICOLON, NEWLINE, UNDEFINED};
		intptr expr = 0;

		res = parse_expression(t + i, eoe, &expr);
		if (res < 0) {
			fprintf(stderr, "ERR: error toplevel\n");
			return -1;
		}
		i += res;

		intptr daddr = ftalloc(&ftast, sizeof(Decl));
		*stmt = daddr;
		Decl *decl = (Decl*) ftptr(&ftast, daddr);

		decl->type = SDECL;
		decl->ident = ident;
		decl->expr = expr;
		decl->cst = cst;
		decl->itype = type;
	}

	// consume end of statement
	i++;

	return i;
}

int
parse_toplevel_import(const ETok *t, intptr *stmt)
{
	(void)t;
	(void)stmt;
	TODO("IMPORT");
	return -1;
}

int
parse_toplevel(const ETok *t)
{
	int i = 0;
	int ident = -1;
	int res = -1;
	intptr stmt;

	while (t[i] != EOI) {
		switch (t[i]) {
		case IDENTIFIER:
			i++;
			ident = t[i];
			i++;
			res = parse_toplevel_decl(t + i, ident, &stmt);
			break;
		case IMPORT:
			i++;
			res = parse_toplevel_import(t + i, &stmt);
			break;
		default:
			fprintf(stderr, "Unexpected token: <%s> toplevel\n", tokenstrs[t[i]]);
			return -1;
		}

		if (res < 0) {
			return -1;
		}
		i += res;
		printstmt(stderr, stmt);
		fprintf(stderr, "\n");
	}
	return 0;
}

