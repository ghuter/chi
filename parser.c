#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "lib/token.h"
#include "lib/fatarena.h"
#include "parser.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define TODO(message) \
    do { \
        fprintf(stderr, "TODO(%d): %s\n", __LINE__, message); \
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

#define SAVECST(_dest, _kind, _addr)             \
do {                                             \
	intptr _tmp = ftalloc(&ftast, sizeof(Csti)); \
	*_dest = _tmp;                               \
	Csti *_csti = (Csti *)ftptr(&ftast, _tmp);   \
	_csti->addr = _addr;                         \
	_csti->kind = _kind;                         \
} while (0)

#define PTRLVL(_t, _i, _type, _ptrlvl)                                           \
do {                                                                             \
	if (_t[_i] == BXOR) {                                                        \
		while (_t[_i] == BXOR) {                                                 \
			_ptrlvl++;                                                           \
			_i++;                                                                \
		}                                                                        \
		if (_t[_i] != IDENTIFIER) {                                              \
			ERR("Error when parsing a type, after n `^` expects <IDENTIFIER>."); \
			return -1;                                                           \
		}                                                                        \
		_i++;                                                                    \
		_type = _t[_i];                                                          \
		_i++;                                                                    \
	}                                                                            \
} while (0)


#define ISMONADICOP(_op) (_op == SUB || _op == LNOT || _op == BNOT || _op == BXOR || _op == AT || _op == SIZEOF)
#define ISBINOP(_op) (_op >= MUL && _op <= LNOT)
#define TOKTOOP(_t) ((_t) - MUL)
#define TOKTOASSIGN(_t) ((_t) - ASSIGN)
#define TYPESTR(_type) (_type == -1 ? "?" : (char*) ftptr(&ftident, _type))
#define identstr(_ident) ((char*) ftptr(&ftident, _ident))
#define ASSIGN2OP(_t) (assigntable[_t - assign_offset])

extern FatArena ftident;
extern FatArena ftimmed;
extern FatArena ftlit;

const char *stmtstrs[NSTATEMENT] = {
	[NOP]         = "NOP",
	[SSEQ]        = "SSEQ",
	[SSTRUCT]     = "SSTRUCT",
	[SENUM]       = "SENUM",
	[SINTERFACE]  = "SINTERFACE",
	[SFUN]        = "SFUN",
	[SDECL]       = "SDECL",
	[SIF]         = "SIF",
	[SELSE]       = "SELSE",
	[SASSIGN]     = "SASSIGN",
	[SFOR]        = "SFOR",
	[SCALL]       = "SCALL",
	[SRETURN]     = "SRETURN",
	[SIMPORT]     = "SIMPORT",
	[SEXPRASSIGN] = "SEXPRASSIGN",
};

const char *exprstrs[NEXPR] = {
	[ENONE]   = "ENONE",
	[ECSTI]   = "ECSTI",
	[ECSTF]   = "ECSTF",
	[ECSTS]   = "ECSTS",
	[ETRUE]   = "ETRUE",
	[EFALSE]  = "EFALSE",
	[EMEM]    = "EMEM",
	[EBINOP]  = "EBINOP",
	[EUNOP]   = "EUNOP",
	[ECALL]   = "ECALL",
	[EACCESS] = "EACCESS",
	[ESUBSCR] = "ESUBSCR",
	[ESTRUCT] = "ESTRUCT",
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
	[UOP_DEREF]  = "DEREF",
	[UOP_AT]     = "AT",
	[UOP_SIZEOF] = "SIZEOF",
};

static const int assign_offset = ASSIGN;
static const Op assigntable[MOD_ASSIGN - ASSIGN + 1] = {
	[ASSIGN - ASSIGN] = -1,
	[BAND_ASSIGN - ASSIGN] = OP_BAND,
	[BOR_ASSIGN - ASSIGN] = OP_BOR,
	[BXOR_ASSIGN - ASSIGN] = OP_BXOR,
	[BNOT_ASSIGN - ASSIGN] = OP_BNOT,
	[LAND_ASSIGN - ASSIGN] = OP_LAND,
	[LOR_ASSIGN - ASSIGN] = OP_LOR,
	[LSHIFT_ASSIGN - ASSIGN] = OP_LSHIFT,
	[RSHIFT_ASSIGN - ASSIGN] = OP_RSHIFT,
	[ADD_ASSIGN - ASSIGN] = OP_ADD,
	[SUB_ASSIGN - ASSIGN] = OP_SUB,
	[MUL_ASSIGN - ASSIGN] = OP_MUL,
	[DIV_ASSIGN - ASSIGN] = OP_DIV,
	[MOD_ASSIGN - ASSIGN] = OP_MOD,
};

void printexpr(FILE *fd, intptr expr);
static int parse_expression(const ETok *t, const ETok *eoe, intptr *expr);
static int parse_stmt_block(const ETok *t, intptr *stmt);
static int parse_call(const ETok *t, intptr ident, intptr *d);
static int parse_otherop(const ETok *t, const ETok *eoe, intptr *expr);
static int parse_fun_stmt(const ETok *t, const ETok *eoe, intptr *stmt);

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
		SStruct *s = (SStruct*) ptr;
		fprintf(fd, "%s(%s", stmtstrs[*ptr], (char*) ftptr(&ftident, s->ident));

		SMember* members = (SMember*) ftptr(&ftast, s->members);
		for (int i = 0; i < s->nmember; i++) {
			fprintf(fd, ", <%s : ", (char*) ftptr(&ftident, members->ident));
			int ptrlvl = members->ptrlvl;
			while (ptrlvl-- > 0) {
				fprintf(fd, "^");
			}
			fprintf(fd, "%s>", (char*) ftptr(&ftident, members->type));
			members++;
		}
		fprintf(fd, ")");
		break;
	}
	case SDECL: {
		SDecl *decl = (SDecl*) ptr;
		char *type = decl->type ==  -1 ? "?" : (char*) ftptr(&ftident, decl->type);
		if (decl->cst) {
			fprintf(fd, "const ");
		}
		fprintf(fd, "%s(%s, ", stmtstrs[*ptr], (char*) ftptr(&ftident, decl->ident));
		printexpr(fd, decl->expr);
		fprintf(fd, ") : ");
		int ptrlvl = decl->ptrlvl;
		while (ptrlvl-- > 0) {
			fprintf(fd, "^");
		}
		fprintf(fd, "%s", type);
		break;
	}
	case SFUN: {
		SFun *fun = (SFun*) ptr;
		char *ret = fun->type ==  -1 ? "void" : (char*) ftptr(&ftident, fun->type);
		fprintf(fd, "%s(%s", stmtstrs[*ptr], (char*)ftptr(&ftident, fun->ident));

		SMember* members = (SMember*) ftptr(&ftast, fun->params);
		for (int i = 0; i < fun->nparam; i++) {
			fprintf(fd, ", %s : ", (char*) ftptr(&ftident, members->ident));
			int ptrlvl = members->ptrlvl;
			while (ptrlvl-- > 0) {
				fprintf(fd, "^");
			}
			fprintf(fd, "%s", (char*) ftptr(&ftident, members->type));
			members++;
		}
		fprintf(fd, ", {");
		printstmt(fd, fun->stmt);
		fprintf(fd, "}");
		fprintf(fd, ") : ");

		int ptrlvl = fun->ptrlvl;
		while (ptrlvl-- > 0) {
			fprintf(fd, "^");
		}
		fprintf(fd, "%s", ret);

		return;
	}
	case SRETURN: {
		SReturn *ret = (SReturn*) ptr;
		fprintf(fd, "%s(", stmtstrs[*ptr]);
		printexpr(fd, ret->expr);
		fprintf(fd, ")");
		return;
	}
	case SASSIGN: {
		SAssign *assign = (SAssign*) ptr;
		fprintf(fd, "%s(%s, ", stmtstrs[*ptr], (char*) ftptr(&ftident, assign->ident));
		printexpr(fd, assign->expr);
		fprintf(fd, ")");
		return;
	}
	case SEXPRASSIGN: {
		SExprAssign *a = (SExprAssign*) ptr;
		fprintf(fd, "%s(", stmtstrs[*ptr]);
		printexpr(fd, a->left);
		fprintf(fd, ", ");
		printexpr(fd, a->right);
		fprintf(fd, ")");
		return;
	}
	case SIF: {
		SIf *_if = (SIf*) ptr;
		fprintf(fd, "%s(", stmtstrs[*ptr]);
		printexpr(fd, _if->cond);
		fprintf(fd, ", ");
		printstmt(fd, _if->ifstmt);

		if (_if->elsestmt > 0) {
			fprintf(fd, ", ELSE ");
			printstmt(fd, _if->elsestmt);
		}
		fprintf(fd, ")");
		return;
	}
	case SSEQ: {
		SSeq *seq = (SSeq*) ptr;
		fprintf(fd, "%s(", stmtstrs[*ptr]);
		printstmt(fd, seq->stmt);
		fprintf(fd, ", ");
		printstmt(fd, seq->nxt);
		fprintf(fd, ")");
		return;
	}
	case SFOR: {
		SFor *_for = (SFor*) ptr;
		fprintf(fd, "%s(", stmtstrs[*ptr]);
		printstmt(fd, _for->stmt1);
		fprintf(fd, ", ");
		printexpr(fd, _for->expr);
		fprintf(fd, ", ");
		printstmt(fd, _for->stmt2);
		fprintf(fd, ", ");
		printstmt(fd, _for->forstmt);
		fprintf(fd, ")");
		return;
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
		return;
	}
	case SIMPORT: {
		SImport *import = (SImport*) ptr;
		fprintf(fd, "%s(%s)", stmtstrs[*ptr], (char*) ftptr(&ftident, import->ident));
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
	case ECSTI: {
		Csti *csti = (Csti*) ptr;
		fprintf(fd, "%s(%ld)", exprstrs[*ptr], *((int64_t*) ftptr(&ftimmed, csti->addr)));
		return;
	}
	case ECSTF: {
		Cstf *cstf = (Cstf*) ptr;
		fprintf(fd, "%s(%f)", exprstrs[*ptr], *((double*) ftptr(&ftimmed, cstf->addr)));
		return;
	}
	case ECSTS: {
		Csts *csts = (Csts*) ptr;
		fprintf(fd, "%s(%s)", exprstrs[*ptr], (char*) ftptr(&ftlit, csts->addr));
		return;
	}
	case ETRUE:
	case EFALSE:
		fprintf(fd, "%s", exprstrs[*ptr]);
		return;
	case EMEM: {
		Mem *mem = (Mem*) ptr;
		fprintf(fd, "%s(%s)", exprstrs[*ptr], (char*) ftptr(&ftident, mem->addr));
		return;
	}
	case EBINOP: {
		EBinop *binop = (EBinop*) ptr;
		char* type = TYPESTR(binop->type);
		fprintf(fd, "%s(%s, ", exprstrs[*ptr], opstrs[binop->op]);
		printexpr(fd, binop->left);
		fprintf(fd, ", ");
		printexpr(fd, binop->right);
		fprintf(fd, ") : ");
		int ptrlvl = binop->ptrlvl;
		while (ptrlvl -- > 0) {
			fprintf(fd, "^");
		}
		fprintf(fd, "%s", type);
		return;
	}
	case EUNOP: {
		EUnop *unop = (EUnop*) ptr;
		char* type = TYPESTR(unop->type);
		fprintf(fd, "%s(%s, ", exprstrs[*ptr], uopstrs[unop->op]);
		printexpr(fd, unop->expr);
		fprintf(fd, ") : ");
		int ptrlvl = unop->ptrlvl;
		while (ptrlvl -- > 0) {
			fprintf(fd, "^");
		}
		fprintf(fd, "%s", type);
		return;
	}
	case ECALL: {
		ECall *call = (ECall*) ptr;
		char* type = TYPESTR(call->type);
		intptr *paramtab = (intptr*) ftptr(&ftast, call->params);

		fprintf(fd, "%s(", exprstrs[*ptr]);
		printexpr(fd, call->expr);
		for (int i = 0; i < call->nparam; i++) {
			fprintf(fd, ", ");
			printexpr(fd, paramtab[i]);
		}
		fprintf(fd, ") : ");
		int ptrlvl = call->ptrlvl;
		while (ptrlvl -- > 0) {
			fprintf(fd, "^");
		}
		fprintf(fd, "%s", type);
		return;
	}
	case EACCESS: {
		EAccess *ac = (EAccess*) ptr;
		char* type = TYPESTR(ac->type);
		fprintf(fd, "%s(", exprstrs[*ptr]);
		printexpr(fd, ac->expr);
		fprintf(fd, ".");
		fprintf(fd, "%s", (char*) ftptr(&ftident, ac->ident));
		fprintf(fd, ") : ");
		int ptrlvl = ac->ptrlvl;
		while (ptrlvl -- > 0) {
			fprintf(fd, "^");
		}
		fprintf(fd, "%s", type);
		return;
	}
	case ESUBSCR: {
		ESubscr *sb = (ESubscr*) ptr;
		char* type = TYPESTR(sb->type);
		fprintf(fd, "%s(", exprstrs[*ptr]);
		printexpr(fd, sb->expr);
		fprintf(fd, "[");
		printexpr(fd, sb->idxexpr);
		fprintf(fd, "]) : ");
		int ptrlvl = sb->ptrlvl;
		while (ptrlvl -- > 0) {
			fprintf(fd, "^");
		}
		fprintf(fd, "%s", type);
		return;
	}
	case ESTRUCT: {
		EStruct *st = (EStruct*) ptr;
		fprintf(fd, "%s(%s { ", exprstrs[*ptr], (char*) ftptr(&ftident, st->ident));

		EElem *elem = (EElem*) ftptr(&ftast, st->elems);
		for (int i = 0; i < st->nelem; i++) {
			fprintf(fd, "<%s : ", (char*) ftptr(&ftident, elem->ident));
			printexpr(fd, elem->expr);
			fprintf(fd, "> ");
			elem++;
		}
		fprintf(fd, "})");
		return;
	}
	default:
		assert(1 || "Unreachable epxr");
	}
}

static int
parse_param(const ETok *t, SFun* fun)
{
	int i = 0;
	if (t[i] != LPAREN) {
		ERR("ERR: Unexpected token <%s> when parsing the functions param.\n Expected `fun` `(`<param> `)`", tokenstrs[t[i]]);
		return -1;
	}
	i++;


	intptr maddr = ftalloc(&ftast, sizeof(SMember));
	fun->params = maddr;
	fun->nparam = 0;
	SMember* member = (SMember*) ftptr(&ftast, maddr);

	while (t[i] != RPAREN) {
		int ident = -1;
		int type = -1;
		int ptrlvl = 0;
		fun->nparam++;

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

		PTRLVL(t, i, type, ptrlvl);
		if (ptrlvl == 0) {
			if (t[i] != IDENTIFIER) {
				ERR("Unexpected token <%s> when parsing the functions param.\n|> Expected a param: <IDENTIFIER> `:` <IDENTIFIER>", tokenstrs[t[i]]);
				return -1;
			}
			i++;
			type = t[i];
			i++;
		}

		member->type = type;
		member->ident = ident;
		member->ptrlvl = ptrlvl;

		if (t[i] != COMMA && t[i] != RPAREN) {
			ERR("Unexpected token <%s> when parsing the functions param.\n|> Expected a `,` or `)`.", tokenstrs[t[i]]);
			return -1;
		}

		if (t[i] == COMMA) {
			i++;
			member++;
			ftalloc(&ftast, sizeof(SMember));
		}
	}

	i++;
	return i;
}

static int
parse_stmt_return(const ETok *t, const ETok *eoe, intptr *stmt)
{
	int i = 0;
	int res = -1;

	intptr saddr = ftalloc(&ftast, sizeof(SReturn));
	*stmt = saddr;
	SReturn *ret = (SReturn*) ftptr(&ftast, saddr);
	ret->kind = SRETURN;

	res = parse_expression(t + i, eoe, &ret->expr);
	if (res < 0) {
		ERR("parse expression in a function.");
		return -1;
	}
	i += res;

	ISTOK(t[i], eoe, res);
	if (!res) {
		ERR("The return doesn't finish with the correct token.");
		ERR("Tok(%d): %s, Eoe:", t[i], tokenstrs[t[i]]);
		return -1;
	}
	i++;

	return i;
}

static int
parse_stmt_assignexpr(const ETok *t, const ETok *eoe, intptr *stmt, intptr left)
{
	int i = 0;
	int res = -1;

	intptr aaddr = ftalloc(&ftast, sizeof(SExprAssign));
	*stmt = aaddr;

	SExprAssign *a = (SExprAssign*) ftptr(&ftast, aaddr);
	a->kind = SEXPRASSIGN;
	a->left = left;

	if (t[i] < ASSIGN || t[i] > MOD_ASSIGN) {
		ERR("Expects an assign but found <%s>.", tokenstrs[t[i]]);
		return -1;
	}
	int op = ASSIGN2OP(t[i]);
	i++;

	intptr *expr = &a->right;
	if (op != -1) {
		intptr baddr = ftalloc(&ftast, sizeof(EBinop));
		a->right = baddr;
		EBinop *b = (EBinop*) ftptr(&ftast, baddr);
		b->kind = EBINOP;
		b->op = op;
		b->type = -1;
		b->ptrlvl = 0;
		b->left = left;

		expr = &b->right;
	}

	res = parse_expression(t + i, eoe, expr);
	if (res < 0) {
		ERR("parse expression in a function.");
		return -1;
	}
	i += res;


	ISTOK(t[i], eoe, res);
	if (!res) {
		ERR("The assign doesn't finish with the correct token.");
		ERR("Tok(%d): %s, Eoe:", t[i], tokenstrs[t[i]]);
		return -1;
	}

	printstmt(stderr, *stmt);
	// consume `;`
	i++;
	return i;
}

static int
parse_stmt_assign(const ETok *t, const ETok *eoe, intptr *stmt, intptr ident, int op)
{
	int i = 0;
	int res = -1;

	intptr aaddr = ftalloc(&ftast, sizeof(SAssign));
	*stmt = aaddr;

	SAssign *assign = (SAssign*) ftptr(&ftast, aaddr);
	assign->kind = SASSIGN;
	assign->ident = ident;

	intptr *expr = &assign->expr;
	if (op != -1) {
		intptr baddr = ftalloc(&ftast, sizeof(EBinop));
		assign->expr = baddr;
		EBinop *b = (EBinop*) ftptr(&ftast, baddr);
		b->kind = EBINOP;
		b->op = op;
		b->type = -1;
		b->ptrlvl = 0;

		intptr maddr = ftalloc(&ftast, sizeof(Mem));
		Mem *mem = (Mem*) ftptr(&ftast, maddr);
		mem->addr = ident;
		mem->kind = EMEM;
		b->left = maddr;

		expr = &b->right;
	}

	res = parse_expression(t + i, eoe, expr);
	if (res < 0) {
		ERR("parse expression in a function.");
		return -1;
	}
	i += res;


	ISTOK(t[i], eoe, res);
	if (!res) {
		ERR("The assign doesn't finish with the correct token.");
		ERR("Tok(%d): %s, Eoe:", t[i], tokenstrs[t[i]]);
		return -1;
	}

	// consume `;`
	i++;
	return i;
}

static int
parse_stmt_call(const ETok *t, const ETok *eoe, intptr *stmt, intptr ident)
{
	int i = 0;
	int res = -1;
	intptr caddr = -1;

	res = parse_call(t + i, ident, &caddr);
	if (res < 0) {
		ERR("Error when paring a call statement in a function.");
		return -1;
	}
	i += res;

	*stmt = caddr;
	SCall *call = (SCall*) ftptr(&ftast, caddr);
	call->kind = SCALL;

	ISTOK(t[i], eoe, res);
	if (!res) {
		ERR("The call declaration doesn't finish with the correct token.");
		ERR("Tok(%d): %s, Eoe:", t[i], tokenstrs[t[i]]);
		return -1;
	}

	// consume
	i++;
	return i;
}

static int
parse_stmt_decl(const ETok *t, const ETok *eoe, intptr *stmt, intptr ident)
{
	int i = 0;
	int res = -1;
	int cst = -1;
	int type = -1;
	int ptrlvl = 0;

	if (t[i] != COLON) {
		ERR("Unexpected token <%s> at the beginning of a statement.\n|> Expects `:`", tokenstrs[t[i]]);
		return -1;
	}
	i++;

	PTRLVL(t, i, type, ptrlvl);
	if (ptrlvl == 0 && t[i] == IDENTIFIER) {
		i++;
		type = t[i];
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

	intptr daddr = ftalloc(&ftast, sizeof(SDecl));
	*stmt = daddr;

	SDecl *decl = (SDecl*) ftptr(&ftast, daddr);
	decl->kind = SDECL;
	decl->cst = cst;
	decl->type = type;
	decl->ident = ident;
	decl->ptrlvl = ptrlvl;

	res = parse_expression(t + i, eoe, &decl->expr);
	if (res < 0) {
		ERR("parsing expression in a declaration statement");
		return -1;
	}
	i += res;

	// consume `;`
	i++;
	return i;
}


static int
parse_stmt_if(const ETok *t, intptr *stmt)
{
	const ETok eoe[] = {LBRACES, UNDEFINED};
	int i = 0;
	int res = -1;

	intptr iaddr = ftalloc(&ftast, sizeof(SIf));
	*stmt = iaddr;

	SIf *_if = (SIf*) ftptr(&ftast, iaddr);
	_if->kind = SIF;

	res = parse_expression(t + i, eoe, &_if->cond);
	if (res < 0) {
		ERR("parsing expression in a declaration statement");
		return -1;
	}
	i += res;

	intptr ifstmt = -1;
	intptr elsestmt = -1;
	res = parse_fun_stmt(t + i, eoe, &ifstmt);
	if (res < 0) {
		ERR("parsing statements in a if statement");
		return -1;
	}
	i += res;

	const ETok eoeelse[] = {SEMICOLON, UNDEFINED};
	if (t[i] == ELSE) {
		i++;
		res = parse_fun_stmt(t + i, eoeelse, &elsestmt);
		if (res < 0) {
			ERR("Error when parsing the else stmt.");
			return -1;
		}
		i += res;
	}

	_if->ifstmt = ifstmt;
	_if->elsestmt = elsestmt;

	return i;
}

static int
parse_stmt_for(const ETok *t, intptr *stmt)
{
	const ETok eoe[] = {SEMICOLON, UNDEFINED};
	const ETok eoe2[] = {SEMICOLON, LBRACES, UNDEFINED};
	int i = 0;
	int res = -1;
	intptr stmt1, stmt2, expr, forstmt = -1;

	res = parse_fun_stmt(t + i, eoe, &stmt1);
	if (res < 0) {
		ERR("Error when parsing the first statement of a forloop. Exepects: `for` `stmt`; `expr` `;` `stmt` `{` stmt `}`");
		return -1;
	}
	i += res;

	res = parse_expression(t + i, eoe, &expr);
	if (res < 0) {
		ERR("Error when parsing the expression of a forloop. Exepects: `for` `stmt`; `expr` `;` `stmt` `{` stmt `}`");
		return -1;
	}
	i += res;

	// Consume `;`
	if (t[i] != SEMICOLON) {
		ERR("Error when parsing the expression of a forloop. Exepects: `for` `stmt`; `expr` `;` `stmt` `{` stmt `}`");
		return -1;
	}
	i++;

	res = parse_fun_stmt(t + i, eoe2, &stmt2);
	if (res < 0) {
		ERR("Error when parsing the second statement of a forloop. Exepects: `for` `stmt`; `expr` `;` `stmt` `{` stmt `}`");
		return -1;
	}
	i += res;

	if (t[i - 1] == LBRACES) {
		// cancel the consume
		i--;
	}

	res = parse_fun_stmt(t + i, eoe, &forstmt);
	if (res < 0) {
		ERR("Error when parsing statements in a forloop.");
		return -1;
	}
	i += res;

	intptr faddr = ftalloc(&ftast, sizeof(SFor));
	*stmt = faddr;

	SFor *_for = (SFor*) ftptr(&ftast, faddr);
	_for->kind = SFOR;
	_for->forstmt = forstmt;
	_for->stmt1 = stmt1;
	_for->stmt2 = stmt2;
	_for->expr = expr;

	return i;
}

static int
parse_stmt_block(const ETok *t, intptr *stmt)
{
	const ETok eoe[] = {SEMICOLON, UNDEFINED};
	int i = 0;
	int res = -1;
	intptr retaddr;
	intptr *saveaddr = &retaddr;

	if (t[i] == RBRACES) {
		return -1;
	}

	for (;;) {
		res = parse_fun_stmt(t + i, eoe, saveaddr);
		if (res < 0) {
			ERR("parsing fun statements");
			return -1;
		}
		i += res;

		if (t[i] == RBRACES) {
			break;
		}

		intptr addr = ftalloc(&ftast, sizeof(SSeq));
		intptr cpy = *saveaddr;
		*saveaddr = addr;
		SSeq *seq = (SSeq*) ftptr(&ftast, addr);
		seq->kind = SSEQ;
		seq->stmt = cpy;
		seq->nxt = -1;
		saveaddr = &seq->nxt;
	}
	*stmt = retaddr;

	// consume `}`
	i++;
	return i;
}

static const ETok assigneoe[] = {
	ASSIGN, BAND_ASSIGN, BOR_ASSIGN,
	BXOR_ASSIGN, BNOT_ASSIGN, LAND_ASSIGN,
	LOR_ASSIGN, LSHIFT_ASSIGN, RSHIFT_ASSIGN,
	ADD_ASSIGN, SUB_ASSIGN, MUL_ASSIGN,
	DIV_ASSIGN, MOD_ASSIGN, UNDEFINED
};

static int
parse_fun_stmt(const ETok *t, const ETok *eoe, intptr *stmt)
{
	int i = 0;
	int res = -1;

	switch (t[i]) {
	case RETURN: {
		i++;
		res = parse_stmt_return(t + i, eoe, stmt);
		break;
	}
	case LPAREN:
	case BXOR: {
		intptr expr = -1;
		res = parse_expression(t + i, assigneoe, &expr);
		if (res < 0) {
			ERR("Error when parsing the left part of an assign.");
			return -1;
		}
		i += res;

		res = parse_stmt_assignexpr(t + i, eoe, stmt, expr);
		if (res < 0) {
			ERR("Error when parsing the left part of an assign.");
			return -1;
		}
		break;
	}
	case IDENTIFIER:
		i++;
		int ident = t[i];
		i++;

		if (t[i] >= ASSIGN && t[i] <= MOD_ASSIGN) {
			Op op = ASSIGN2OP(t[i]);
			i++;
			res = parse_stmt_assign(t + i, eoe, stmt, ident, op);
			break;
		}

		if (t[i] == LPAREN) {
			i++;
			res = parse_stmt_call(t + i, eoe, stmt, ident);
			break;
		}

		if (t[i] == DOT || t[i] == LBRACKETS) {
			intptr expr = -1;
			i -= 2;

			res = parse_expression(t + i, assigneoe, &expr);
			if (res < 0) {
				ERR("Error when parsing the left part of an assign.");
				return -1;
			}
			i += res;

			res = parse_stmt_assignexpr(t + i, eoe, stmt, expr);
			if (res < 0) {
				ERR("Error when parsing the left part of an assign.");
				return -1;
			}
			break;
		}

		res = parse_stmt_decl(t + i, eoe, stmt, ident);
		break;
	case IF: {
		i++;
		res = parse_stmt_if(t + i, stmt);
		break;
	}
	case FOR: {
		i++;
		res = parse_stmt_for(t + i, stmt);
		break;
	}
	case LBRACES: {
		i++;
		res = parse_stmt_block(t + i, stmt);
		break;
	}
	default:
		ERR("Unexepcted token <%s> in at the beginning of a statement.", tokenstrs[t[i]]);
	}

	if (res < 0) {
		ERR("Error in parse_fun_stmt.");
		return -1;
	}

	i += res;
	return i;
}


static int
parse_toplevel_fun(const ETok *t, intptr ident, intptr *stmt)
{
	const ETok eoe[] = {SEMICOLON, UNDEFINED};
	int i = 0;
	int res = -1;

	intptr saddr = ftalloc(&ftast, sizeof(SFun));
	*stmt = saddr;

	SFun *fun = (SFun*) ftptr(&ftast, saddr);
	fun->ident = ident;
	fun->kind = SFUN;
	fun->type = -1;
	fun->ptrlvl = 0;
	fun->stmt = -1;

	res = parse_param(t + i, fun);
	if (res < 0) {
		ERR("parsing params");
		return -1;
	}
	i += res;

	if (t[i] == IDENTIFIER) {
		i++;
		fun->type = t[i];
		i++;
	}

	res = parse_fun_stmt(t + i, eoe, &fun->stmt);
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
	int i = 0;

	intptr saddr = ftalloc(&ftast, sizeof(SStruct));
	*stmt = saddr;
	SStruct *s = (SStruct*) ftptr(&ftast, saddr);

	s->kind = SSTRUCT;
	s->ident = ident;
	s->nmember = 0;

	intptr current = ftalloc(&ftast, sizeof(SMember));
	s->members = current;

	if (t[i] != LBRACES) {
		ERR("after 'struct' a '{' is expected");
		return -1;
	}
	i++;

	while (1) {
		int ident = -1;
		int type = -1;
		int ptrlvl = 0;
		SMember *member = (SMember*)ftptr(&ftast, current);
		s->nmember++;

		if (t[i] != IDENTIFIER) {
			ERR("Unexepcted Token <%s>, expects struct { <IDENTIFIER> ':' <IDENTIFIER> }", tokenstrs[t[i]]);
			return -1;
		};
		i++;
		ident = t[i];
		i++;

		if (t[i] != COLON) {
			ERR("Unexepcted Token <%s>, expects struct { <IDENTIFIER> ':' <IDENTIFIER> }", tokenstrs[t[i]]);
			return -1;
		}
		i++;

		PTRLVL(t, i, type, ptrlvl);
		if (ptrlvl == 0) {
			if (t[i] != IDENTIFIER) {
				ERR("Unexepcted Token <%s>, expects struct { <IDENTIFIER> ':' <IDENTIFIER> }", tokenstrs[t[i]]);
				return -1;
			}
			i++;
			type = t[i];
			i++;
		}

		member->ident = ident;
		member->type = type;
		member->ptrlvl = ptrlvl;
		current = ftalloc(&ftast, sizeof(SMember));

		if (t[i] == RBRACES) {
			break;
		}

		if (t[i] != SEMICOLON) {
			ERR("Expects a separator <SEMICOLON>");
			return -1;
		}
		i++;

		if (t[i] == RBRACES) {
			break;
		}
	}

	// consume `}`
	return i + 1;
}

static int
parse_toplevel_enum(const ETok *t)
{
	(void)t;
	TODO("Toplevel enum");
	return -1;
}



static int
parse_call(const ETok *t, const intptr addr, intptr *d)
{
	const ETok eoe[3] = {COMMA, RPAREN, UNDEFINED};
	int i = 0;
	int res = -1;

	// Alloc and save a function expression
	intptr caddr = ftalloc(&ftast, sizeof(ECall));
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
			ERR("The param separator in a function call must be a <COMMA>");
			goto err;
		}
		if (t[i] == COMMA) {
			i++;
		}
	}

	// consume RPAREN
	i++;

	// Save the params
	ECall *call = (ECall*) ftptr(&ftast, caddr);
	call->kind = ECALL;
	call->expr = addr;
	call->nparam = nparam;
	call->params = ftalloc(&ftast, nparam * sizeof(intptr));
	call->type = -1;
	call->ptrlvl = 0;
	intptr *cparams = (intptr*) ftptr(&ftast, call->params);
	memcpy(cparams, params, sizeof(intptr) * nparam);

	return i;

err:
	ERR("Error in the parsing of a function param");
	return -1;
}

static int
parse_struct_elem(const ETok *t, const intptr addr, intptr *expr)
{
	static const ETok eoe[] = {COMMA, RBRACES, UNDEFINED};
	int i = 0;
	int res = -1;

	if (t[i] != LBRACES) {
		return -1;
	}
	i++;

	// Alloc and save a function expression
	intptr saddr = ftalloc(&ftast, sizeof(EStruct));
	*expr = saddr;

	EElem elems[64];
	int nelem = 0;

	// Parse the elems
	while (t[i] != RBRACES) {
		if (t[i] != IDENTIFIER) {
			ERR("Error when parsing a struct element, expects: <IDENTIFIER> `:` `expr`.");
			return -1;
		}
		i++;
		elems[nelem].ident = t[i];
		i++;

		if (t[i] != COLON) {
			ERR("Error when parsing a struct element, expects: <IDENTIFIER> `:` `expr`.");
			return -1;
		}
		i++;

		res = parse_expression(t + i, eoe, &elems[nelem].expr);
		if (res < 0) {
			ERR("Error when parsing the expression of a struct element.");
			return -1;
		}
		i += res;
		nelem++;

		if (!(t[i] == COMMA || t[i] == RBRACES)) {
			ERR("Error when parsing the struct elements, the separator must be a <COMMA>");
			return -1;
		}
		if (t[i] == COMMA) {
			i++;
		}
	}

	// consume RBRACES
	i++;

	// Save the params
	EStruct *st = (EStruct*) ftptr(&ftast, saddr);
	st->kind = ESTRUCT;
	st->ident = addr;
	st->nelem = nelem;
	st->elems = ftalloc(&ftast, nelem * sizeof(EElem));
	EElem *selems = (EElem*) ftptr(&ftast, st->elems);
	memcpy(selems, elems, sizeof(EElem) * nelem);

	return i;
}

static int
parse_direct(const ETok *t, const ETok *eoe, intptr *expr)
{
	int i = 0;
	int add = 0;
	int res = -1;

	switch (t[i]) {
	case IDENTIFIER: {
		i++;
		EExpr kind = EMEM;
		intptr addr = t[i];
		i++;

		ISTOK(t[i], eoe, res);
		// case init a struct
		if (!res && t[i] == LBRACES) {
			res = parse_struct_elem(t + i, addr, expr);
			if (res < 0) {
				ERR("Error when parsing the elements in a struct init.");
				return -1;
			}
			i += res;
			return i;
		}

		// case IDENTIFIER
		SAVECST(expr, kind, addr);
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
		EExpr kind = ECSTI + add;
		intptr addr = t[i];
		i++;
		SAVECST(expr, kind, addr);
		break;
	}
	case FALSE:
		add ++;
	// fallthrough
	case TRUE: {
		i++;
		intptr baddr = ftalloc(&ftast, sizeof(UnknownExpr));
		UnknownExpr* b = (UnknownExpr*) ftptr(&ftast, baddr);
		*b = ETRUE + add;
		*expr = baddr;
		break;
	}
	case LPAREN: {
		i++;
		const ETok eoeparen[] = {RPAREN, UNDEFINED};

		// Parse the expression
		res = parse_expression(t + i, eoeparen, expr);
		if (res < 0) {
			ERR("Error when parsing expression `( expr `)`");
			return -1;
		}
		i += res;

		// Consume RPAREN
		i++;
		break;
	}

	default:
		ERR("Unexpected token <%s>", tokenstrs[t[i]]);
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

static int
parse_aftercall(const ETok *t, const ETok *eoe, intptr cexpr, intptr *expr)
{
	int i = 0;
	int res = -1;

	switch (t[i]) {
	case DOT: {
		i++;
		if (t[i] != IDENTIFIER) {
			ERR("After a <DOT> expects an identifier");
			return -1;
		}
		i++;
		intptr ident = t[i];
		i++;

		intptr aaddr = ftalloc(&ftast, sizeof(EAccess));
		*expr = aaddr;
		EAccess *ac = (EAccess*) ftptr(&ftast, aaddr);
		ac->kind = EACCESS;
		ac->expr = cexpr;
		ac->ident = ident;
		ac->type = -1;
		ac->ptrlvl = 0;

		res = parse_aftercall(t + i, eoe, aaddr, expr);
		if (res < 0) {
			ERR("Error when parsing an aftercall");
			return -1;
		}
		i += res;

		return i;
	}
	case LPAREN: {
		i++;
		intptr call = -1;
		res = parse_call(t + i, cexpr, &call);
		if (res < 0) {
			ERR("error in the parsing of an expression");
			return -1;
		}
		i += res;

		res = parse_aftercall(t + i, eoe, call, expr);
		if (res < 0) {
			ERR("Error when parsing an aftercall");
			return -1;
		}
		i += res;
		return i;
	}

	case LBRACKETS: {
		i++;
		const ETok alteoe[] = {RBRACKETS, UNDEFINED};

		intptr idxexpr = -1;
		res = parse_expression(t + i, alteoe, &idxexpr);
		if (res < 0) {
			ERR("error in the parsing of an array subscripting");
			return -1;
		}
		i += res;
		// Consume `]`
		i++;

		intptr saddr = ftalloc(&ftast, sizeof(ESubscr));
		ESubscr *sb = (ESubscr*) ftptr(&ftast, saddr);
		sb->kind = ESUBSCR;
		sb->idxexpr = idxexpr;
		sb->expr = cexpr;

		res = parse_aftercall(t + i, eoe, saddr, expr);
		if (res < 0) {
			ERR("Error when parsing an aftercall");
			return -1;
		}
		i += res;
		return i;
	}
	default:
		break;
	}
	*expr = cexpr;
	return i;
}

static int
parse_otherop(const ETok *t, const ETok *eoe, intptr *expr)
{
	int i = 0;
	int res = -1;
	intptr addr = -1;

	res = parse_direct(t + i, eoe, &addr);
	if (res < 0) {
		return -1;
	}
	i += res;

	switch (t[i]) {
	case DOT: {
		i++;
		if (t[i] != IDENTIFIER) {
			ERR("After a <DOT> expects an identifier");
			return -1;
		}
		i++;
		intptr ident = t[i];
		i++;

		intptr aaddr = ftalloc(&ftast, sizeof(EAccess));
		EAccess *ac = (EAccess*) ftptr(&ftast, aaddr);
		ac->kind = EACCESS;
		ac->expr = addr;
		ac->ident = ident;
		ac->type = -1;
		ac->ptrlvl = 0;

		res = parse_aftercall(t + i, eoe, aaddr, expr);
		if (res < 0) {
			ERR("Error when parsing aftercall");
			return -1;
		}
		i += res;

		return i;
	}
	case LBRACKETS: {
		i++;
		const ETok alteoe[] = {RBRACKETS, UNDEFINED};

		intptr idxexpr = -1;
		res = parse_expression(t + i, alteoe, &idxexpr);
		if (res < 0) {
			ERR("error in the parsing of an array subscripting");
			return -1;
		}
		i += res;
		// Consume `]`
		i++;

		intptr saddr = ftalloc(&ftast, sizeof(ESubscr));
		ESubscr *sb = (ESubscr*) ftptr(&ftast, saddr);
		sb->kind = ESUBSCR;
		sb->idxexpr = idxexpr;
		sb->expr = addr;

		res = parse_aftercall(t + i, eoe, saddr, expr);
		if (res < 0) {
			ERR("Error when parsing aftercall");
			return -1;
		}
		i += res;
		return i;
	}
	case LPAREN: {
		i++;
		intptr call = -1;
		res = parse_call(t + i, addr, &call);
		if (res < 0) {
			ERR("error in the parsing of an expression");
			return -1;
		}
		i += res;

		res = parse_aftercall(t + i, eoe, call, expr);
		if (res < 0) {
			ERR("Error when parsing aftercall");
			return -1;
		}
		i += res;
		return i;
	}
	default:
		*expr = addr;
		return i;
	}
}

static int
parse_unop(const ETok *t, const ETok *eoe, intptr *expr)
{
	int i = 0;
	int res = 0;
	int add = 0;

	switch (t[i]) {
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
		i++;
		// Alloc unop
		intptr uaddr = ftalloc(&ftast, sizeof(EUnop));
		*expr = uaddr;

		// Save unop
		EUnop *unop = (EUnop*) ftptr(&ftast, uaddr);
		unop->kind = EUNOP;
		unop->op = add;
		unop->type = -1;
		unop->ptrlvl = 0;

		// Parse the following expression
		intptr nxt = 0;
		res = parse_unop(t + i, eoe, &nxt);
		if (res < 0) {
			ERR("Error when parsing an unop");
			return -1;
		}
		i += res;
		unop->expr = nxt;
		return i;
		break;
	}
	default:
		res = parse_otherop(t + i, eoe, expr);
		if (res < 0) {
			ERR("Error when parsing an expression");
			return -1;
		}
		i += res;
		return i;
	}
	ERR("Unreachable.");
	return -1;
}

static int
parse_binop(const ETok *t, const ETok *eoe, intptr *expr, int lvl)
{
	int i = 0;
	int res = -1;
	intptr left = -1;
	intptr right = -1;

	if (lvl > 1) {
		res = parse_binop(t + i, eoe, &left, lvl - 1);
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
	intptr baddr = ftalloc(&ftast, sizeof(EBinop));
	EBinop *binop = (EBinop*) ftptr(&ftast, baddr);
	binop->kind = EBINOP;
	binop->ptrlvl = 0;
	binop->type = -1;
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

static int
parse_expression(const ETok *t, const ETok *eoe, intptr *expr)
{
	int i = 0;
	int res = -1;

	res = parse_binop(t, eoe, expr, LEVEL);
	if (res < 0) {
		ERR("parse_binop");
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

static int
parse_toplevel_decl(const ETok *t, intptr ident, intptr *stmt)
{
	int res = -1;
	int i = 0;
	int type = -1;
	int cst = -1;
	int ptrlvl = 0;

	if (t[i] != COLON) {
		ERR("Declaration: <IDENTIFIER> : [<IDENTIFIER>] : EXPR, but found <%s>", tokenstrs[t[i]]);
		return -1;
	}
	i++;

	PTRLVL(t, i, type, ptrlvl);
	if (ptrlvl == 0 && t[i] == IDENTIFIER) {
		i++;
		type = t[i];
		i++;
	}

	if (t[i] == COLON) {
		cst = 1;
	} else if (t[i] == ASSIGN) {
		cst = 0;
	} else {
		ERR("Declaration: <IDENTIFIER> : [<IDENTIFIER>] : EXPR, but found <%s>", tokenstrs[t[i]]);
		return -1;
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

		intptr daddr = ftalloc(&ftast, sizeof(SDecl));
		*stmt = daddr;
		SDecl *decl = (SDecl*) ftptr(&ftast, daddr);

		decl->kind = SDECL;
		decl->ident = ident;
		decl->expr = expr;
		decl->cst = cst;
		decl->type = type;
		decl->ptrlvl = ptrlvl;

		if (t[i] != SEMICOLON) {
			ERR("A top level declaration finish by a `;`.");
			return -1;
		}
		// consume `;`
		i++;

	}

	return i;
}

static int
parse_toplevel_import(const ETok *t, intptr *stmt)
{
	int i = 0;
	int ident = -1;

	if (t[i] != IDENTIFIER) {
		ERR("Exepects an <IDENTIFIER>: `import` <IDENTIFIER> `;`");
		return -1;
	}
	i++;
	ident = t[i];
	i++;

	if (t[i] != SEMICOLON) {
		ERR("Exepects an <SEMICOLON>: `import` <IDENTIFIER> `;`");
		return -1;
	}
	i++;

	intptr iaddr = ftalloc(&ftast, sizeof(SImport));
	*stmt = iaddr;
	SImport *import = (SImport*) ftptr(&ftast, iaddr);
	import->kind = SIMPORT;
	import->ident = ident;

	return i;
}

int
parse_toplevel(const ETok *t, intptr *stmt)
{
	int i = 0;
	int ident = -1;
	int res = -1;

	switch (t[i]) {
	case IDENTIFIER:
		i++;
		ident = t[i];
		i++;
		res = parse_toplevel_decl(t + i, ident, stmt);
		break;
	case IMPORT:
		i++;
		res = parse_toplevel_import(t + i, stmt);
		break;
	default:
		ERR("Unexpected token: <%s> toplevel", tokenstrs[t[i]]);
		return -1;
	}

	if (res < 0) {
		ERR("Toplevel error.");
		return -1;
	}
	i += res;

	return i;
}

