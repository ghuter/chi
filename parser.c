#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "lib/token.h"
#include "lib/fatarena.h"
#include "parser.h"
#include "lex.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define TODO(message) \
    do { \
        fprintf(stderr, "TODO(%s): %s\n", TOSTRING(__LINE__), message); \
    } while (0)

#define ERR(...) fprintf(stderr, "ERR(%d,%d): ", __LINE__, line); fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")

#define BURNSEPARATORS(_t, _i)             \
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

#define BURNNEWLINE(_t, _i)       \
do {                              \
	while (_t[_i] == NEWLINE) {   \
		_i++;                     \
		line++;                   \
	}                             \
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
	[SFOR]       = "SFOR",
	[SCALL]      = "SCALL",
	[SRETURN]    = "SRETURN",
};

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
const char *uopstrs[UOP_NUM] = {
	[UOP_SUB]    = "SUB",
	[UOP_LNOT]   = "LNOT",
	[UOP_BNOT]   = "BNOT",
	[UOP_BXOR]   = "BXOR",
	[UOP_AT]     = "AT",
	[UOP_SIZEOF] = "SIZEOF",
};

void printexpr(FILE *fd, intptr expr);
static int parse_expression(const ETok *t, const ETok *eoe, intptr *expr);
static int parse_fun_stmts(const ETok *t, intptr *stmt);
static int parse_call(const ETok *t, intptr ident, intptr *d);

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
	case SFOR: {
		For *_for = (For*) ptr;
		fprintf(fd, "%s(", stmtstrs[*ptr]);
		printexpr(fd, _for->expr1);
		fprintf(fd, ", ");
		printexpr(fd, _for->expr2);
		fprintf(fd, ", ");
		printexpr(fd, _for->expr3);
		fprintf(fd, ", ");
		printstmt(fd, _for->forstmt);
		fprintf(fd, ")");
		break;
	}
	case SCALL: {
		SCall *call = (SCall*) ptr;
		fprintf(fd, "%s(%s", stmtstrs[*ptr], (char*) ftptr(&ftident, call->ident));

		intptr *paramtab = (intptr*) ftptr(&ftast, call->params);
		for (int i = 0; i < call->nparam; i++) {
			fprintf(fd, ", ");
			printexpr(fd, paramtab[i]);
		}
		fprintf(fd, ")");
		break;
	}
	default:
		ERR(" Unreachable statement in printstmt.");
		assert(1 || "Unreachable stmt");
	}
}

void
printexpr(FILE* fd, intptr expr)
{
	UnknownExpr* ptr = (UnknownExpr*) ftptr(&ftast, expr);
	switch (*ptr) {
	case ENONE:
		fprintf(fd, "<NONE>");
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
		assert(1 || "Unreachable epxr");
	}
}

static int
parse_param(const ETok *t, Fun* fun)
{
	int i = 0;
	if (t[i] != LPAREN) {
		ERR("ERR: Unexpected token <%s> when parsing the functions param.\n Expected `fun` `(`<param> `)`", tokenstrs[t[i]]);
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
			ERR("Unexpected token <%s> when parsing the functions param.\n|> Expected a param: <IDENTIFIER> `:` <IDENTIFIER>", tokenstrs[t[i]]);
			return -1;
		}
		i++;
		ident = t[i];
		i++;

		if (t[i] != COLON) {
			ERR("Unexpected token <%s> when parsing the functions param.\n|> Expected a param: <IDENTIFIER> `:` <IDENTIFIER>", tokenstrs[t[i]]);
			return -1;
		}
		i++;

		if (t[i] != IDENTIFIER) {
			ERR("Unexpected token <%s> when parsing the functions param.\n|> Expected a param: <IDENTIFIER> `:` <IDENTIFIER>", tokenstrs[t[i]]);
			return -1;
		}
		i++;
		type = t[i];
		i++;

		member->type = type;
		member->ident = ident;

		if (t[i] != COMMA && t[i] != RPAREN) {
			ERR("Unexpected token <%s> when parsing the functions param.\n|> Expected a `,` or `)`.", tokenstrs[t[i]]);
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

static int
parse_fun_stmt(const ETok *t, intptr *stmt)
{
	const ETok eoe[] = {SEMICOLON, RBRACES, UNDEFINED};
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
			ERR("parse expression in a function.");
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
				ERR("parse expression in a function.");
				return -1;
			}
			i += res;

			return i;
		}
		
		if (t[i] == LPAREN) {
			intptr caddr = -1;
			res = parse_call(t+i, ident, &caddr);
			if (res < 0) {
				ERR("Error when paring a call statement in a function.");
				return -1;
			}
			i += res;

			*stmt = caddr;
			SCall *call = (SCall*) ftptr(&ftast, caddr);
			call->type = SCALL;
			printf("CURRENT(%d) parse fun call: %s\n", i, tokenstrs[t[i]]);
			printf("CURRENT(%d) parse fun call: %s\n", i, tokenstrs[t[i+1]]);
			return i;
		}

		int cst = -1;
		int itype = -1;

		if (t[i] != COLON) {
			ERR("Unexpected token <%s> at the beginning of a statement.\n|> Expects `:`", tokenstrs[t[i]]);
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
			ERR("Unexpected token <%s> in a declaration statement.", tokenstrs[t[i]]);
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
			ERR("parsing expression in a declaration statement");
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
			ERR("parsing expression in a declaration statement");
			return -1;
		}
		i += res;

		intptr ifstmt = -1;
		intptr elsestmt = -1;
		res = parse_fun_stmts(t + i, &ifstmt);
		if (res < 0) {
			ERR("parsing statements in a if statement");
			return -1;
		}
		i += res;

		if (t[i] != RBRACES) {
			ERR("Unexpected token <%s>, expects a `}`", tokenstrs[t[i]]);
		}
		// Consume the `}` of the if
		i++;

		if (t[i] == ELSE) {
			i++;
			res = parse_fun_stmts(t + i, &elsestmt);
			if (res < 0) {
				ERR("parsing statements in a else statement");
				return -1;
			}
			i += res;

			if (t[i] != RBRACES) {
				ERR("Unexpected token <%s>, expects a `}`", tokenstrs[t[i]]);
			}
			// Consume the `}` of the else
			i++;
		}

		_if->ifstmt = ifstmt;
		_if->elsestmt = elsestmt;

		return i;
		break;
	}
	case FOR: {
		i++;
		const ETok eoe1[] = {SEMICOLON, UNDEFINED};
		const ETok eoe2[] = {LBRACES, UNDEFINED};
		intptr expr1, expr2, expr3, forstmt;
		res = parse_expression(t + i, eoe1, &expr1);
		if (res < 0) {
			ERR("Error when parsing the first expression of a forloop. Exepects: `for` `expr`; `expr` `;` `expr` `{` stmt `}`");
			return -1;
		}
		i += res;
		i++;

		res = parse_expression(t + i, eoe1, &expr2);
		if (res < 0) {
			ERR("Error when parsing the second expression of a forloop. Exepects: `for` `expr`; `expr` `;` `expr` `{` stmt `}`");
			return -1;
		}
		i += res;
		i++;

		res = parse_expression(t + i, eoe2, &expr3);
		if (res < 0) {
			ERR("Error when parsing the third expression of a forloop. Exepects: `for` `expr`; `expr` `;` `expr` `{` stmt `}`");
			return -1;
		}
		i += res;
		printf("CURRENT: %s\n", tokenstrs[t[i]]);

		res = parse_fun_stmts(t + i, &forstmt);
		if (res < 0) {
			ERR("Error when parsing statements in a forloop.");
			return -1;
		}
		i += res;
		// Consume the `}` of the forloop
		i++;

		intptr faddr = ftalloc(&ftast, sizeof(For));
		*stmt=faddr;

		For *_for = (For*) ftptr(&ftast, faddr);
		_for->type = SFOR;
		_for->forstmt = forstmt;
		_for->expr1 = expr1;
		_for->expr2 = expr2;
		_for->expr3 = expr3;

		return i;
	}
	default:
		ERR("Unexepcted token <%s> in at the beginning of a statement.", tokenstrs[t[i]]);
	}

	ERR("Unreachable when parsing statement");
	return -1;
}

static int
parse_fun_stmts(const ETok *t, intptr *stmt)
{
	TODO("fun stmts");

	int i = 0;
	int res = -1;

	if (t[i] != LBRACES) {
		ERR("Unexpected token <%s>, `{` is expected for a group of statement.", tokenstrs[t[i]]);
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
			ERR("parsing fun statements");
			return -1;
		}
		i += res;

		printf("CURRENT(%d): %s\n", i, tokenstrs[t[i]]);
		switch(t[i]) {
		case SEMICOLON:
			i++;
			if (t[i] == RBRACES) {
				break;
			}

			intptr addr = ftalloc(&ftast, sizeof(Seq));
			seq->nxt = addr;
			seq = (Seq*) ftptr(&ftast, addr);
			seq->type = SSEQ;
			seq->stmt = -1;
			seq->nxt = -1;
			break;

		case RBRACES:
			// Case right after a block
			if (t[i-1] == RBRACES) {
				break;
			}

			ERR("Unexpected token <%s>, expects `;`.", tokenstrs[t[i]]);
			return -1;
			break;

		default:
			// Case right after a block
			if (t[i-1] == RBRACES) {
				intptr addr = ftalloc(&ftast, sizeof(Seq));
				seq->nxt = addr;
				seq = (Seq*) ftptr(&ftast, addr);
				seq->type = SSEQ;
				seq->stmt = -1;
				seq->nxt = -1;
				break;
			}

			ERR("Unexpected token <%s>, expects `}` or `;`.", tokenstrs[t[i]]);
			return -1;
		}
	}

	return i;
}

static int
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
	fun->stmt = -1;

	res = parse_param(t + i, fun);
	if (res < 0) {
		ERR("parsing params");
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
		ERR("parsing fun statements");
		return -1;
	}
	i += res;

	return i;
}

static int
parse_toplevel_interface(const ETok *t)
{
	(void)t;
	TODO("Toplevel interface");
	return -1;
}

static int
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
		ERR("after 'struct' a '{' is expected");
		return -1;
	}
	i++;

	BURNNEWLINE(t, i);

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

		BURNNEWLINE(t, i);

		if (t[i] != SEMICOLON) {
			ERR("Expects a separator <SEMICOLON>");
			return -1;
		}
		i++;

		BURNNEWLINE(t, i);

		if (t[i] == RBRACES) {
			break;
		}
	}

	return i;

err:
	ERR("Unexepcted Token <%s>, expects struct { <IDENTIFIER> ':' <IDENTIFIER> }", tokenstrs[t[i]]);
	return -1;
}

static int
parse_toplevel_enum(const ETok *t)
{
	(void)t;
	TODO("Toplevel enum");
	return -1;
}



static int
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
			fprintf(stderr, "The param separator in a function call must be a <COMMA>");
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
	ERR("Error in the parsing of a function param");
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
			ERR("error in the parsing of an expression");
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
		fprintf(stderr, "Unexpected token <%s>", tokenstrs[t[i]]);
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
		ERR("parse_expression doesn't consume the all expression (eoe doesn't match with t)");
		ERR("Tok(%d): %s, Eoe:", t[i], tokenstrs[t[i]]);
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
		ERR("Declaration: <IDENTIFIER> : <IDENTIFIER> : EXPR");
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
		ERR("Declaration: <IDENTIFIER> : <IDENTIFIER> : EXPR");
		return 0;
	}
	i++;

	if (t[i] == STRUCT) {
		i++;
		res = parse_toplevel_struct(t + i, ident, stmt);
		if (res < 0) {
			ERR("Error toplevel");
			return -1;
		}
		i += res;
	} else if (t[i] == ENUM) {
		i++;
		res = parse_toplevel_enum(t + i);
		if (res < 0) {
			ERR("Error toplevel");
			return -1;
		}
		i += res;
	} else if (t[i] == FUN) {
		i++;
		res = parse_toplevel_fun(t + i, ident, stmt);
		if (res < 0) {
			ERR("Error toplevel fun");
			return -1;
		}
		i += res;
	} else if (t[i] == INTERFACE) {
		i++;
		res = parse_toplevel_interface(t + i);
		if (res < 0) {
			ERR("Error toplevel");
			return -1;
		}
		i += res;
	} else { /* Expression */
		const ETok eoe[3] = {SEMICOLON, UNDEFINED};
		intptr expr = 0;

		res = parse_expression(t + i, eoe, &expr);
		if (res < 0) {
			ERR("Error toplevel");
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
			ERR("Unexpected token: <%s> toplevel", tokenstrs[t[i]]);
			return -1;
		}

		if (res < 0) {
			return -1;
		}
		i += res;
		printstmt(stderr, stmt);
		fprintf(stderr, "\n");
		BURNNEWLINE(t, i);
	}
	return 0;
}

