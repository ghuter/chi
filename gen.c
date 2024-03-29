#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "lib/fatarena.h"
#include "lib/token.h"
#include "parser.h"
#include "analyzer.h"
#include "gen.h"

#define CODEADD(...) do { size_t sz = sprintf(hd, __VA_ARGS__); hd += sz; } while(0)
#define TODO(message) \
    do { \
        fprintf(stderr, "TODO(%d): %s\n", __LINE__, message); \
    } while (0)

#define ERR(...) \
	do { \
	fprintf(stderr, "ERR(%d,%d): ", __LINE__, line); \
	fprintf(stderr, __VA_ARGS__); \
	fprintf(stderr, "\n"); \
	} while (0)

extern int ident2langtype[NLANGTYPE * 3];
extern int typeoffset;

extern FatArena ftast;
extern FatArena ftident;
extern FatArena ftimmed;
extern FatArena ftlit;

char *hd = 0;

static const char *optype2inst[OP_NUM][NLANGTYPE] = {
	[OP_MUL] = {
		[U8]   = "mulu8",
		[U16]  = "mulu16",
		[U32]  = "mulu32",
		[U64]  = "mulu64",
		[U128] = "mulu128",
		[I8]   = "muli8",
		[I16]  = "muli16",
		[I32]  = "muli32",
		[I64]  = "muli64",
		[I128] = "muli128",
		[F32]  = "mulf32",
		[F64]  = "mulf64",
	},
	[OP_DIV] = {
		[U8]   = "quou8",
		[U16]  = "quou16",
		[U32]  = "quou32",
		[U64]  = "quou64",
		[U128] = "quou128",
		[I8]   = "quoi8",
		[I16]  = "quoi16",
		[I32]  = "quoi32",
		[I64]  = "quoi64",
		[I128] = "quoi128",
		[F32]  = "quof32",
		[F64]  = "quof64",
	},
	[OP_MOD] = {
		[U8]   = "remu8",
		[U16]  = "remu16",
		[U32]  = "remu32",
		[U64]  = "remu64",
		[U128] = "remu128",
		[I8]   = "remi8",
		[I16]  = "remi16",
		[I32]  = "remi32",
		[I64]  = "remi64",
		[I128] = "remi128",
	},
	[OP_ADD] = {
		[U8]   = "addu8",
		[U16]  = "addu16",
		[U32]  = "addu32",
		[U64]  = "addu64",
		[U128] = "addu128",
		[I8]   = "addi8",
		[I16]  = "addi16",
		[I32]  = "addi32",
		[I64]  = "addi64",
		[I128] = "addi128",
		[F32]  = "addf32",
		[F64]  = "addf64",
	},
	[OP_SUB] = {
		[U8]   = "subu8",
		[U16]  = "subu16",
		[U32]  = "subu32",
		[U64]  = "subu64",
		[U128] = "subu128",
		[I8]   = "subi8",
		[I16]  = "subi16",
		[I32]  = "subi32",
		[I64]  = "subi64",
		[I128] = "subi128",
		[F32]  = "subf32",
		[F64]  = "subf64",
	},
	[OP_LSHIFT] = {
		[U8]   = "shlu8",
		[U16]  = "shlu16",
		[U32]  = "shlu32",
		[U64]  = "shlu64",
		[U128] = "shlu128",
		[I8]   = "shli8",
		[I16]  = "shli16",
		[I32]  = "shli32",
		[I64]  = "shli64",
		[I128] = "shli128",
		[F32]  = "shlf32",
		[F64]  = "shlf64",
	},
	[OP_RSHIFT] = {
		[U8]   = "shru8",
		[U16]  = "shru16",
		[U32]  = "shru32",
		[U64]  = "shru64",
		[U128] = "shru128",
		[I8]   = "shri8",
		[I16]  = "shri16",
		[I32]  = "shri32",
		[I64]  = "shri64",
		[I128] = "shri128",
		[F32]  = "shrf32",
		[F64]  = "shrf64",
	},
	[OP_LT] = {
		[U8]   = "ltu8",
		[U16]  = "ltu16",
		[U32]  = "ltu32",
		[U64]  = "ltu64",
		[U128] = "ltu128",
		[I8]   = "lti8",
		[I16]  = "lti16",
		[I32]  = "lti32",
		[I64]  = "lti64",
		[I128] = "lti128",
		[F32]  = "ltf32",
		[F64]  = "ltf64",
	},
	[OP_GT] = {
		[U8]   = "gtu8",
		[U16]  = "gtu16",
		[U32]  = "gtu32",
		[U64]  = "gtu64",
		[U128] = "gtu128",
		[I8]   = "gti8",
		[I16]  = "gti16",
		[I32]  = "gti32",
		[I64]  = "gti64",
		[I128] = "gti128",
		[F32]  = "gtf32",
		[F64]  = "gtf64",
	},
	[OP_LE] = {
		[U8]   = "ltequ8",
		[U16]  = "ltequ16",
		[U32]  = "ltequ32",
		[U64]  = "ltequ64",
		[U128] = "ltequ128",
		[I8]   = "lteqi8",
		[I16]  = "lteqi16",
		[I32]  = "lteqi32",
		[I64]  = "lteqi64",
		[I128] = "lteqi128",
		[F32]  = "lteqf32",
		[F64]  = "lteqf64",
	},
	[OP_GE] = {
		[U8]   = "gtequ8",
		[U16]  = "gtequ16",
		[U32]  = "gtequ32",
		[U64]  = "gtequ64",
		[U128] = "gtequ128",
		[I8]   = "gteqi8",
		[I16]  = "gteqi16",
		[I32]  = "gteqi32",
		[I64]  = "gteqi64",
		[I128] = "gteqi128",
		[F32]  = "gteqf32",
		[F64]  = "gteqf64",
	},
	[OP_EQUAL] = {
		[U8]   = "equ8",
		[U16]  = "equ16",
		[U32]  = "equ32",
		[U64]  = "equ64",
		[U128] = "equ128",
		[I8]   = "eqi8",
		[I16]  = "eqi16",
		[I32]  = "eqi32",
		[I64]  = "eqi64",
		[I128] = "eqi128",
		[F32]  = "eqf32",
		[F64]  = "eqf64",
	},
	[OP_NEQUAL] = {
		[U8]   = "nequ8",
		[U16]  = "nequ16",
		[U32]  = "nequ32",
		[U64]  = "nequ64",
		[U128] = "nequ128",
		[I8]   = "neqi8",
		[I16]  = "neqi16",
		[I32]  = "neqi32",
		[I64]  = "neqi64",
		[I128] = "neqi128",
		[F32]  = "neqf32",
		[F64]  = "neqf64",
	},
	[OP_BAND] = {
		[U8]   = "andu8",
		[U16]  = "andu16",
		[U32]  = "andu32",
		[U64]  = "andu64",
		[U128] = "andu128",
		[I8]   = "andi8",
		[I16]  = "andi16",
		[I32]  = "andi32",
		[I64]  = "andi64",
		[I128] = "andi128",
		[F32]  = "andf32",
		[F64]  = "andf64",
	},
	[OP_BXOR] = {
		[U8]   = "xoru8",
		[U16]  = "xoru16",
		[U32]  = "xoru32",
		[U64]  = "xoru64",
		[U128] = "xoru128",
		[I8]   = "xori8",
		[I16]  = "xori16",
		[I32]  = "xori32",
		[I64]  = "xori64",
		[I128] = "xori128",
		[F32]  = "xorf32",
		[F64]  = "xorf64",
	},
	[OP_BOR] = {
		[U8]   = "oru8",
		[U16]  = "oru16",
		[U32]  = "oru32",
		[U64]  = "oru64",
		[U128] = "oru128",
		[I8]   = "ori8",
		[I16]  = "ori16",
		[I32]  = "ori32",
		[I64]  = "ori64",
		[I128] = "ori128",
		[F32]  = "orf32",
		[F64]  = "orf64",
	},
	[OP_LAND] = {
		[U8]   = "&&",
		[U16]  = "&&",
		[U32]  = "&&",
		[U64]  = "&&",
		[U128] = "&&",
		[I8]   = "&&",
		[I16]  = "&&",
		[I32]  = "&&",
		[I64]  = "&&",
		[I128] = "&&",
		[F32]  = "&&",
		[F64]  = "&&",
	},
	[OP_LOR] = {
		[U8]   = "||",
		[U16]  = "||",
		[U32]  = "||",
		[U64]  = "||",
		[U128] = "||",
		[I8]   = "||",
		[I16]  = "||",
		[I32]  = "||",
		[I64]  = "||",
		[I128] = "||",
		[F32]  = "||",
		[F64]  = "||",
	},
	[OP_BNOT] = {
		[U8]   = "!",
		[U16]  = "!",
		[U32]  = "!",
		[U64]  = "!",
		[U128] = "!",
		[I8]   = "!",
		[I16]  = "!",
		[I32]  = "!",
		[I64]  = "!",
		[I128] = "!",
		[F32]  = "!",
		[F64]  = "!",
	},
	[OP_LNOT] = {
		[U8]   = "!",
		[U16]  = "!",
		[U32]  = "!",
		[U64]  = "!",
		[U128] = "!",
		[I8]   = "!",
		[I16]  = "!",
		[I32]  = "!",
		[I64]  = "!",
		[I128] = "!",
		[F32]  = "!",
		[F64]  = "!",
	},
};

static const char* uop2str[UOP_NUM] = {
	[UOP_SUB]    = "-",
	[UOP_LNOT]   = "!",
	[UOP_BNOT]   = "!",
	[UOP_AT]     = "&",
	[UOP_DEREF]  = "*",
	[UOP_SIZEOF] = "sizeof",
};

static int getchildtype(intptr expr);
static void genexpr(intptr expr);
static void gentype(intptr type, int ptrlvl);
static void genfunprot(StmtArray pubsym, intptr stmt, char *prefix, char *sig_name);
static void genfun(StmtArray pubsym, intptr stmt, char *prefix, char *sig_name);
static void genstruct(intptr stmt, char *prefix);
static void genstmt(StmtArray pubsym, intptr stmt);
static void genmod(StmtArray pubsym, Symbols modsym[NMODSYM]);

static int
getchildtype(intptr expr)
{
	UnknownExpr* ptr = (UnknownExpr*) ftptr(&ftast, expr);
	int type = -1;

#define TYPEOF(X) do{ \
		X *e = (X*)ptr; \
		type = ident2langtype[e->type - typeoffset]; \
		} while(0)
	switch (*ptr) {
	case ECSTI:
		return I64;
	case ECSTF:
		return F64;
	case EUNOP: {
		TYPEOF(EUnop);
		break;
	}
	case ECALL: {
		TYPEOF(ECall);
		break;
	}
	case EACCESS: {
		TYPEOF(EAccess);
		break;
	}
	case ESUBSCR: {
		TYPEOF(ESubscr);
		break;
	}
	case EMEM: {
		TYPEOF(Mem);
		break;
	}
	default:
		fprintf(stderr, "default hit with: %d\n", *ptr);
		return -1;
	}
#undef TYPEOF

	return type;
}

static void
genexpr(intptr expr)
{
	UnknownExpr* ptr = (UnknownExpr*) ftptr(&ftast, expr);

	switch (*ptr) {
	case ENONE:
		break;
	case ECSTI: {
		Csti *csti = (Csti*) ptr;
		CODEADD("%"PRId64, *((int64_t*) ftptr(&ftimmed, csti->addr)));
		break;
	}
	case ECSTF: {
		Cstf *cstf = (Cstf*) ptr;
		CODEADD("%f", *((double*) ftptr(&ftimmed, cstf->addr)));
		break;
	}
	case ECSTS: {
		Csts *csts = (Csts*) ptr;
		CODEADD("\"%s\"", (char*) ftptr(&ftlit, csts->addr));
		break;
	}
	case EMEM: {
		Mem *mem = (Mem*) ptr;
		CODEADD("%s", (char*) ftptr(&ftident, mem->addr));
		break;
	}
	case EBINOP: {
		EBinop *binop = (EBinop*) ptr;
		Op op = binop->op;
		int type = ident2langtype[binop->type - typeoffset];
		fprintf(stderr, "op: %d, binop->type: %d, type: %d, str: %s\n",
		        op, binop->type, type, optype2inst[op][type]);
		// bool type: we want the actual operand types
		if (type == 0) {
			type = getchildtype(binop->left);
			if (type <= 0) {
				type = getchildtype(binop->right);
			}
		}
		if (type <= 0) {
			ERR("Invalid type for binop");
			assert(1 || "Invalid type for binop");
		}

		if (binop->ptrlvl > 0) {
			type = U64;
			CODEADD("(");
			gentype(binop->type, binop->ptrlvl);
			CODEADD(")");
		}

		fprintf(stderr, "op: %d, binop->type: %d, type: %d, str: %s\n",
		        op, binop->type, type, optype2inst[op][type]);

		CODEADD("%s(", optype2inst[op][type]);
		genexpr(binop->left);
		CODEADD(", ");
		genexpr(binop->right);
		CODEADD(")");
		break;
	}
	case EUNOP: {
		EUnop *unop = (EUnop*) ptr;
		CODEADD("%s(", uop2str[unop->op]);
		genexpr(unop->expr);
		CODEADD(")");
		break;
	}
	case ECALL: {
		ECall *call = (ECall*) ptr;
		UnknownExpr *ptrnext = (UnknownExpr*) ftptr(&ftast, call->expr);

		if (*ptrnext == EACCESS) {
			EAccess *ac = (EAccess*)ptrnext;
			genexpr(ac->expr);
			CODEADD("__%s", (char*) ftptr(&ftident, ac->ident));
		} else {
			genexpr(call->expr);
		}

		intptr *paramtab = (intptr*) ftptr(&ftast, call->params);
		CODEADD("(");
		for (int i = 0; i < call->nparam; i++) {
			if (i != 0) {
				CODEADD(", ");
			}
			genexpr(paramtab[i]);
		}
		CODEADD(")");
		break;
	}
	case EACCESS: {
		EAccess *ac = (EAccess*) ptr;
		genexpr(ac->expr);
		CODEADD(".%s", (char*) ftptr(&ftident, ac->ident));
		break;
	}
	case ESUBSCR: {
		ESubscr *sb = (ESubscr*) ptr;
		genexpr(sb->expr);
		CODEADD("[");
		genexpr(sb->idxexpr);
		CODEADD("]");
		break;
	}
	case ESTRUCT: {
		EStruct *st = (EStruct*) ptr;
		CODEADD("(struct %s) { ", (char*) ftptr(&ftident, st->ident));

		EElem *elem = (EElem*) ftptr(&ftast, st->elems);
		for (int i = 0; i < st->nelem; i++) {
			if (i != 0) {
				CODEADD(", ");
			}
			CODEADD(".%s = ", (char*) ftptr(&ftident, elem->ident));
			genexpr(elem->expr);
			elem++;
		}
		CODEADD("}");
		break;
	}
	case ETRUE: {
		CODEADD("1");
		break;
	}
	case EFALSE: {
		CODEADD("0");
		break;
	}
	case ECAST: {
		ECast *c = (ECast*) ptr;
		CODEADD("(");
		gentype(c->type, c->ptrlvl);
		CODEADD(")");
		genexpr(c->expr);
		break;
	}
	default:
		assert(1 || "Unreachable expr");
	}
}

static void
gentype(intptr type, int ptrlvl)
{
	char *t = (char*)ftptr(&ftident, type);
	// if type is not defined or generic, put "void" as type
	t = (type == -1 || (isupper(t[0]) && t[1] == '\0')) ? "void" : t;
	CODEADD("%s", t);
	while (ptrlvl-- > 0) {
		CODEADD("*");
	}
}

static void
genfunprot(StmtArray pubsym, intptr stmt, char *prefix, char *sig_name)
{
	UnknownStmt *ptr = (UnknownStmt*) ftptr(&ftast, stmt);
	if (stmt == -1) {
		return;
	}

	if (*ptr != SFUN && *ptr != SSIGN)
		return;

	SFun *fun = (SFun*) ptr;
	static const EStmt funkind[] = { SFUN, SNOP };
	char *fun_ident = (char*)ftptr(&ftident, fun->ident);

	if (!ispub(&pubsym, stmt, funkind) && strcmp(fun_ident, "main"))
		CODEADD("static ");
	gentype(fun->type, fun->ptrlvl);
	CODEADD("\n");
	if (prefix)
		CODEADD("%s__", prefix);
	CODEADD("%s(", fun_ident);

	if (sig_name) {
		CODEADD("%s__Modules __M", sig_name);
		if (fun->nparam > 0)
			CODEADD(", ");
	}

	SMember* members = (SMember*) ftptr(&ftast, fun->params);
	for (int i = 0; i < fun->nparam; i++) {
		if (i != 0) {
			CODEADD(", ");
		}
		gentype(members->type, members->ptrlvl);
		CODEADD(" ");
		CODEADD("%s", (char*) ftptr(&ftident, members->ident));
		members++;
	}
	CODEADD(");\n");
}

static void
genfun(StmtArray pubsym, intptr stmt, char *prefix, char *sig_name)
{
	UnknownStmt *ptr = (UnknownStmt*) ftptr(&ftast, stmt);
	if (stmt == -1) {
		return;
	}

	if (*ptr != SFUN)
		return;

	SFun *fun = (SFun*) ptr;
	genfunprot(pubsym, stmt, prefix, sig_name);
	hd -= 2; // remove ";\n" from generated prototype
	CODEADD("\n{\n");
	genstmt(pubsym, fun->stmt);
	CODEADD("}\n");
}

static void
genstruct(intptr stmt, char *prefix)
{
	UnknownStmt *ptr = (UnknownStmt*) ftptr(&ftast, stmt);
	if (stmt == -1) {
		return;
	}
	if (*ptr != SSTRUCT)
		return;
	SStruct *s = (SStruct*) ptr;
	char *ident = (char*) ftptr(&ftident, s->ident);

	if (prefix) {
		CODEADD("typedef struct %s__%s %s__%s;\n", prefix, ident, prefix, ident);
		CODEADD("struct %s__%s {\n", prefix, ident);
	} else {
		CODEADD("typedef struct %s %s;\n", ident, ident);
		CODEADD("struct %s {\n", ident);
	}

	SMember* members = (SMember*) ftptr(&ftast, s->members);
	for (int i = 0; i < s->nmember; i++) {
		CODEADD("\t");
		gentype(members->type, members->ptrlvl);
		CODEADD(" ");
		CODEADD("%s", (char*) ftptr(&ftident, members->ident));
		CODEADD(";\n");
		members++;
	}
	CODEADD("};\n");
}

static void
genstmt(StmtArray pubsym, intptr stmt)
{
	UnknownStmt *ptr = (UnknownStmt*) ftptr(&ftast, stmt);
	if (stmt == -1) {
		return;
	}

	switch (*ptr) {
	case SNOP:
		break;
	case SSTRUCT: {
		genstruct(stmt, 0);
		break;
	}
	case SDECL: {
		SDecl *decl = (SDecl*) ptr;
		if (decl->type == -1) {
			ERR("Missing type information.");
		}
		gentype(decl->type, decl->ptrlvl);
		CODEADD(" %s = ", (char*) ftptr(&ftident, decl->ident));
		genexpr(decl->expr);
		CODEADD(";\n");
		break;
	}
	case SFUN: {
		genfun(pubsym, stmt, 0, 0);
		break;
	}
	case SRETURN: {
		SReturn *ret = (SReturn*) ptr;
		CODEADD("return ");
		genexpr(ret->expr);
		CODEADD(";\n");
		break;
	}
	case SASSIGN: {
		SAssign *assign = (SAssign*) ptr;
		CODEADD("%s = ", (char*) ftptr(&ftident, assign->ident));
		genexpr(assign->expr);
		CODEADD(";\n");
		break;
	}
	case SIF: {
		SIf *_if = (SIf*) ptr;
		CODEADD("if (");
		genexpr(_if->cond);
		CODEADD(") {\n");
		genstmt(pubsym, _if->ifstmt);

		if (_if->elsestmt > 0) {
			CODEADD("} else {\n");
			genstmt(pubsym, _if->elsestmt);
		}
		CODEADD("}\n");
		break;
	}
	case SSEQ: {
		SSeq *seq = (SSeq*) ptr;
		genstmt(pubsym, seq->stmt);
		// kinda not needed, but makes sure there is a semicolon..
		/* CODEADD(";\n"); */
		genstmt(pubsym, seq->nxt);
		break;
	}
	case SFOR: {
		SFor *_for = (SFor*) ptr;
		CODEADD("for (\n");
		genstmt(pubsym, _for->stmt1);
		genexpr(_for->expr);
		CODEADD(";\n");
		genstmt(pubsym, _for->stmt2);
		hd -= 2; // remove ";\n"
		CODEADD("\n) {\n");
		genstmt(pubsym, _for->forstmt);
		CODEADD("}\n");
		break;
	}
	case SCALL: {
		SCall *call = (SCall*) ptr;
		CODEADD("%s(", (char*) ftptr(&ftident, call->ident));

		intptr *paramtab = (intptr*) ftptr(&ftast, call->params);
		for (int i = 0; i < call->nparam; i++) {
			if (i != 0) {
				CODEADD(", ");
			}
			genexpr(paramtab[i]);
		}
		CODEADD(");\n");
		break;
	}
	case SSIGN: {
		SSign *sig = (SSign*) ptr;

		gentype(sig->type, sig->ptrlvl);
		CODEADD(" %s(", (char*)ftptr(&ftident, sig->ident));

		SMember* members = (SMember*) ftptr(&ftast, sig->params);
		for (int i = 0; i < sig->nparam; i++) {
			if (i != 0) {
				CODEADD(", ");
			}
			gentype(members->type, members->ptrlvl);
			CODEADD(" ");
			CODEADD("%s", (char*) ftptr(&ftident, members->ident));
			members++;
		}
		CODEADD(");\n");
		break;
	}
	case SIMPORT: {
		/* TODO("imports"); */
		// SImport *import = (SImport*) ptr;
		// fprintf(fd, "%s(%s)", stmtstrs[*ptr], (char*) ftptr(&ftident, import->ident));
		break;
	}
	case SACCESSMOD: {
		SAccessMod *access = (SAccessMod*)ptr;
		CODEADD("%s__", (char*)ftptr(&ftident, access->mod));
		genstmt(pubsym, access->stmt);
		break;
	}
	default:
		ERR(" Unreachable statement in genstmt: %s", stmtstrs[*ptr]);
		assert(1 || "Unreachable stmt");
	}
}

static void
genmod(StmtArray pubsym, Symbols modsym[NMODSYM])
{
	Symbols *syms = 0;
	Symbol *sym = 0;
	intptr *stmt = 0;

	CODEADD("// --- SIGNATURES\n");
	syms = &modsym[MODSIGN];
	sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		SModSign *s = (SModSign*) ftptr(&ftast, sym[i].stmt);
		char *sig_name = (char*)ftptr(&ftident, s->ident);

		CODEADD("// signature's data types first\n");
		stmt = (intptr*)ftptr(&ftast, s->stmts);
		for (int j = 0; j < s->nstmt; j++) {
			UnknownStmt *ptr = (UnknownStmt*)ftptr(&ftast, stmt[j]);
			if (*ptr != SSTRUCT)
				continue;
			genstruct(stmt[j], sig_name);
		}
		CODEADD("\n");

		CODEADD("// signature's arg table\n");
		if (s->signatures != -1) {
			CODEADD("typedef struct {\n");
			SSignatures *sigs = (SSignatures*)ftptr(&ftast, s->signatures);
			SSignature *sig = (SSignature*)ftptr(&ftast, sigs->signs);
			for (int j = 0; j < sigs->nsign; j++) {
				char *sig_ident = (char*)ftptr(&ftident, sig[j].signature);
				char *param_ident = (char*)ftptr(&ftident, sig[j].ident);
				CODEADD("\t%s %s;\n", sig_ident, param_ident);
			}
			CODEADD("} %s__Modules;\n", sig_name);
		} else {
			CODEADD("typedef %s__Modules void;\n", sig_name);
		}
		CODEADD("\n");

		CODEADD("// signature's function pointers\n");
		stmt = (intptr*)ftptr(&ftast, s->stmts);
		for (int j = 0; j < s->nstmt; j++) {
			UnknownStmt *ptr = (UnknownStmt*)ftptr(&ftast, stmt[j]);
			if (*ptr != SSIGN)
				continue;
			SFun *fun = (SFun*)ptr;

			CODEADD("typedef ");
			gentype(fun->type, fun->ptrlvl);
			CODEADD(" (*%s_fn)", (char*)ftptr(&ftident, fun->ident));

			CODEADD("(");
			CODEADD("%s__Modules __M", sig_name);
			if (fun->nparam > 0)
				CODEADD(", ");
			SMember* members = (SMember*)ftptr(&ftast, fun->params);
			for (int i = 0; i < fun->nparam; i++) {
				if (i != 0) {
					CODEADD(", ");
				}
				gentype(members->type, members->ptrlvl);
				CODEADD(" ");
				CODEADD("%s", (char*)ftptr(&ftident, members->ident));
				members++;
			}
			CODEADD(");\n");
		}
		CODEADD("\n");

		CODEADD("// signature's vtable\n");
		CODEADD("typedef struct {\n");
		stmt = (intptr*)ftptr(&ftast, s->stmts);
		for (int j = 0; j < s->nstmt; j++) {
			UnknownStmt *ptr = (UnknownStmt*)ftptr(&ftast, stmt[j]);
			if (*ptr != SSIGN)
				continue;
			SFun *fun = (SFun*)ptr;
			char *fn_ident = (char*)ftptr(&ftident, fun->ident);
			CODEADD("\t%s_fn %s;\n", fn_ident, fn_ident);
		}
		CODEADD("} %s;\n", (char*)ftptr(&ftident, s->ident));
		CODEADD("\n");
	}
	CODEADD("\n");

	CODEADD("// --- IMPLEMENTATIONS\n");
	syms = &modsym[MODIMPL];
	sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		SModImpl *s = (SModImpl*) ftptr(&ftast, sym[i].stmt);
		char *impl_name = (char*)ftptr(&ftident, s->ident);
		char *sig_name = (char*)ftptr(&ftident, s->signature);

		CODEADD("// implementation's arg table\n");
		if (s->modules != -1) {
			CODEADD("typedef struct {\n");
			SSignatures *sigs = (SSignatures*)ftptr(&ftast, s->modules);
			SSignature *sig = (SSignature*)ftptr(&ftast, sigs->signs);
			for (int j = 0; j < sigs->nsign; j++) {
				char *sig_ident = (char*)ftptr(&ftident, sig[j].signature);
				char *param_ident = (char*)ftptr(&ftident, sig[j].ident);
				CODEADD("\t%s %s;\n", sig_ident, param_ident);
			}
			CODEADD("} %s__Modules;\n", sig_name);
		} else {
			CODEADD("typedef %s__Modules void;\n", sig_name);
		}
		CODEADD("\n");

		CODEADD("// implementation's functions\n");
		stmt = (intptr*)ftptr(&ftast, s->stmts);
		for (int j = 0; j < s->nstmt; j++) {
			UnknownStmt *ptr = (UnknownStmt*)ftptr(&ftast, stmt[j]);
			// has to be a function
			if (*ptr != SFUN)
				continue;
			genfun(pubsym, stmt[j], impl_name, sig_name);
		}
		CODEADD("\n");

		CODEADD("// implementation's function table\n");
		CODEADD("%s %s = (%s) {\n", sig_name, impl_name, sig_name);
		stmt = (intptr*)ftptr(&ftast, s->stmts);
		for (int j = 0; j < s->nstmt; j++) {
			UnknownStmt *ptr = (UnknownStmt*)ftptr(&ftast, stmt[j]);
			// has to be a function
			if (*ptr != SFUN)
				continue;
			SFun *fun = (SFun*)ptr;
			char *fn_ident = (char*)ftptr(&ftident, fun->ident);
			CODEADD("\t.%s = %s__%s;\n", fn_ident, impl_name, fn_ident);
		}
		CODEADD("};\n");
		CODEADD("\n");
	}
	CODEADD("\n");

	CODEADD("// --- SKELETONS\n");
	syms = &modsym[MODSKEL];
	sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		SModSkel *s = (SModSkel*) ftptr(&ftast, sym[i].stmt);
		char *skel_name = (char*)ftptr(&ftident, s->ident);

		CODEADD("// skeleton's functions\n");
		stmt = (intptr*)ftptr(&ftast, s->stmts);
		for (int j = 0; j < s->nstmt; j++) {
			UnknownStmt *ptr = (UnknownStmt*)ftptr(&ftast, stmt[j]);
			// has to be a function
			if (*ptr != SFUN)
				continue;
			genfun(pubsym, stmt[j], skel_name, "FancyName");
		}
	}
	CODEADD("\n");

	CODEADD("// --- DEFINITIONS\n");
	syms = &modsym[MODDEF];
	sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		SModDef *s = (SModDef*)ftptr(&ftast, sym[i].stmt);
		/* char *impl_name = (char*)ftptr(&ftident, s->ident); */
		char *skel_name = (char*)ftptr(&ftident, s->skeleton);

		Symbol *sy = searchtopdcl(modsym + MODSKEL, s->skeleton);
		if (sy == NULL) {
			continue;
		}
		SModSkel *skel = (SModSkel*)ftptr(&ftast, sy->stmt);
		char *sig_name = (char*)ftptr(&ftident, skel->signature);

		CODEADD("%s__Modules %s = {\n", sig_name, skel_name);
		CODEADD("};");
	}
	CODEADD("\n");
}

size_t
gen(char *code, Symbols typesym, Symbols identsym, Symbols signatures, Symbols funsym, Symbols modsym[NMODSYM], StmtArray pubsym)
{
	hd = code;

#if 1
	FILE *fp = 0;
	size_t fsz = 0;
	fp = fopen("lib/c0.h", "r");
	if (!fp) {
		ERR(" Cannot open file: lib/c0.h");
		assert(1 || "Failed to open file 'lib/c0.h'");
	}
	fseek(fp, 0, SEEK_END);
	fsz = ftell(fp);
	rewind(fp);
	assert(fread(hd, 1, fsz, fp));
	hd += fsz;
	fclose(fp);
	CODEADD("\n");
#endif

	CODEADD("// --- TYPES\n");
	Symbol *sym = (Symbol*) ftptr(&ftsym, typesym.array);
	for (int i = 0; i < typesym.nsym; i++) {
		genstmt(pubsym, sym[i].stmt);
		CODEADD("\n");
	}
	CODEADD("\n");

	CODEADD("// --- CONSTANTS\n");
	Symbol *symd = (Symbol*) ftptr(&ftsym, identsym.array);
	for (int i = 0; i < identsym.nsym; i++) {
		genstmt(pubsym, symd[i].stmt);
	}
	CODEADD("\n");

	CODEADD("// --- FUNCTION PROTOTYPES\n");
	Symbol *sig = (Symbol*) ftptr(&ftsym, signatures.array);
	for (int i = 0; i < signatures.nsym; i++) {
		intptr stmt = sig[i].stmt;
		UnknownStmt *ptr = (UnknownStmt*) ftptr(&ftast, stmt);
		if (stmt == -1 || *ptr != SSIGN) {
			continue;
		}
		genfunprot(pubsym, stmt, 0, 0);
	}
	CODEADD("\n");

	genmod(pubsym, modsym);
	CODEADD("\n");

	CODEADD("// --- FUNCTIONS\n");
	Symbol *symf = (Symbol*) ftptr(&ftsym, funsym.array);
	for (int i = 0; i < funsym.nsym; i++) {
		genstmt(pubsym, symf[i].stmt);
		CODEADD("\n");
	}

	return hd - code;
}

