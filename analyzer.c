#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>

#include "token.h"
#include "lib/fatarena.h"
#include "lib/map.h"
#include "lex.h"
#include "parser.h"
#include "analyzer.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define TODO(message)                                                   \
    do {                                                                \
        fprintf(stderr, "TODO(%s:%d): %s\n", __FILE__, __LINE__, message); \
    } while (0)

#define ERR(...) fprintf(stderr, "ERR(%s:%d): ", __FILE__, __LINE__); fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")

#define PRINTINFO 0
#if PRINTINFO == 0
#define INFO(...) do {} while (0)
#else
#define INFO(...) fprintf(stderr, "INFO(%d): ", __LINE__), fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#endif

#define TYPESTR(_type) (_type == -1 ? "?" : (char*) ftptr(&ftident, _type))
#define islangtype(_type) ((typeoffset <= _type) && (_type <= typeend))
#define identstr(_ident) ((char*) ftptr(&ftident, _ident))

extern int typeoffset;
extern int typeend;
extern int ident2langtype[NLANGTYPE * 3];
extern int langtype2ident[NLANGTYPE];
extern Symbols funsym;
extern Symbols identsym;
extern Symbols typesym;
extern Symbols signatures;
extern Symbols modsym[NMODSYM];
extern SymInfo *syminfo;
extern int nsym;


const char* langtypestrs[NLANGTYPE] = {
	[BOOL] = "bool",
	[U8]   = "u8",
	[U16]  = "u16",
	[U32]  = "u32",
	[U64]  = "u64",
	[U128] = "u128",
	[I8]   = "i8",
	[I16]  = "i16",
	[I32]  = "i32",
	[I64]  = "i64",
	[I128] = "i128",
	[F32]  = "f32",
	[F64]  = "f64",
};

static const int promotiontable[NLANGTYPE][NLANGTYPE] = {
	[BOOL] = {
		[BOOL] = BOOL,
		[U8]   = BOOL,
		[U16]  = BOOL,
		[U32]  = BOOL,
		[U64]  = BOOL,
		[U128] = BOOL,
		[I8]   = BOOL,
		[I16]  = BOOL,
		[I32]  = BOOL,
		[I64]  = BOOL,
		[I128] = BOOL,
		[F32]  = BOOL,
		[F64]  = BOOL,
	},
	[U8] = {
		[BOOL] = BOOL,
		[U8]   = U8,
		[U16]  = U16,
		[U32]  = U32,
		[U64]  = U64,
		[U128] = U128,
		[I8]   = I8,
		[I16]  = I16,
		[I32]  = I32,
		[I64]  = I64,
		[I128] = I128,
		[F32]  = F32,
		[F64]  = F64,
	},
	[U16] = {
		[BOOL] = BOOL,
		[U8]   = U16,
		[U16]  = U16,
		[U32]  = U32,
		[U64]  = U64,
		[U128] = U128,
		[I8]   = I16,
		[I16]  = I16,
		[I32]  = I32,
		[I64]  = I64,
		[I128] = I128,
		[F32]  = F32,
		[F64]  = F64,
	},
	[U32] = {
		[BOOL] = BOOL,
		[U8]   = U32,
		[U16]  = U32,
		[U32]  = U32,
		[U64]  = U64,
		[U128] = U128,
		[I8]   = I32,
		[I16]  = I32,
		[I32]  = I32,
		[I64]  = I64,
		[I128] = I128,
		[F32]  = F32,
		[F64]  = F64,
	},
	[U64] = {
		[BOOL] = BOOL,
		[U8]   = U64,
		[U16]  = U64,
		[U32]  = U64,
		[U64]  = U64,
		[U128] = U128,
		[I8]   = I64,
		[I16]  = I64,
		[I32]  = I64,
		[I64]  = I64,
		[I128] = I128,
		[F32]  = F32,
		[F64]  = F64,
	},
	[U128] = {
		[BOOL] = BOOL,
		[U8]   = U128,
		[U16]  = U128,
		[U32]  = U128,
		[U64]  = U128,
		[U128] = U128,
		[I8]   = I128,
		[I16]  = I128,
		[I32]  = I128,
		[I64]  = I128,
		[I128] = I128,
		[F32]  = F32,
		[F64]  = F64,
	},
	[I8] = {
		[BOOL] = BOOL,
		[U8]   = I8,
		[U16]  = I16,
		[U32]  = I32,
		[U64]  = I64,
		[U128] = I128,
		[I8]   = I8,
		[I16]  = I16,
		[I32]  = I32,
		[I64]  = I64,
		[I128] = I128,
		[F32]  = F32,
		[F64]  = F64,
	},
	[I16] = {
		[BOOL] = BOOL,
		[U8]   = I16,
		[U16]  = I16,
		[U32]  = I32,
		[U64]  = I64,
		[U128] = I128,
		[I8]   = I16,
		[I16]  = I16,
		[I32]  = I32,
		[I64]  = I64,
		[I128] = I128,
		[F32]  = F32,
		[F64]  = F64,
	},
	[I32] = {
		[BOOL] = BOOL,
		[U8]   = I32,
		[U16]  = I32,
		[U32]  = I32,
		[U64]  = I64,
		[U128] = I128,
		[I8]   = I32,
		[I16]  = I32,
		[I32]  = I32,
		[I64]  = I64,
		[I128] = I128,
		[F32]  = F32,
		[F64]  = F64,
	},
	[I64] = {
		[BOOL] = BOOL,
		[U8]   = I64,
		[U16]  = I64,
		[U32]  = I64,
		[U64]  = I64,
		[U128] = I128,
		[I8]   = I64,
		[I16]  = I64,
		[I32]  = I64,
		[I64]  = I64,
		[I128] = I128,
		[F32]  = F32,
		[F64]  = F64,
	},
	[I128] = {
		[BOOL] = BOOL,
		[U8]   = I128,
		[U16]  = I128,
		[U32]  = I128,
		[U64]  = I128,
		[U128] = I128,
		[I8]   = I128,
		[I16]  = I128,
		[I32]  = I128,
		[I64]  = I128,
		[I128] = I128,
		[F32]  = F32,
		[F64]  = F64,
	},
	[F32] = {
		[BOOL] = BOOL,
		[U8]   = F32,
		[U16]  = F32,
		[U32]  = F32,
		[U64]  = F32,
		[U128] = F32,
		[I8]   = F32,
		[I16]  = F32,
		[I32]  = F32,
		[I64]  = F32,
		[I128] = F32,
		[F32]  = F32,
		[F64]  = F64,
	},
	[F64] = {
		[BOOL] = BOOL,
		[U8]   = F64,
		[U16]  = F64,
		[U32]  = F64,
		[U64]  = F64,
		[U128] = F64,
		[I8]   = F64,
		[I16]  = F64,
		[I32]  = F64,
		[I64]  = F64,
		[I128] = F64,
		[F32]  = F64,
		[F64]  = F64,
	},
};

static Bool computeconstype(intptr expr, intptr *type, int *ptrlvl, intptr typeinfo);
static Bool analyzefunexpr(intptr expr, intptr *type, int *ptrlvl, intptr typeinfo, int nsym);

void
printsymbols(Symbols *syms)
{
	Symbol *sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		printstmt(stderr, sym[i].stmt);
		fprintf(stderr, "\n");
	}
}

void
printsymbolsinfo(int nsym)
{
	for (int i = nsym - 1; i >= 0; i--) {
		if (syminfo[i].cst) {
			fprintf(stderr, "const ");
		}
		fprintf(stderr, "%s : ", identstr(syminfo[i].ident));
		int ptrlvl = syminfo[i].ptrlvl;
		while (ptrlvl-- > 0) {
			fprintf(stderr, "^");
		}

		char *type = TYPESTR(syminfo[i].type);
		fprintf(stderr, "%s, ", type);
	}

	if (nsym > 0) {
		fprintf(stderr, "\n");
	}
}

static Symbol*
searchtopdcl(Symbols *syms, intptr ident)
{
	Symbol *sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		if (ident == sym[i].ident) {
			return sym + i;
		}
	}

	return NULL;
}

static Bool
existstopdcl(Symbols *syms, intptr ident)
{
	Symbol *sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		if (ident == sym[i].ident) {
			return 1;
		}
	}
	return 0;
}

static Bool
insertsyminfo(int nsym, int block, SDecl *decl)
{
	for (int i = nsym; i > block; i--) {
		if (syminfo[i].ident == decl->ident) {
			ERR("Re-declaration of <%s>.", identstr(decl->ident));
			return 0;
		}
	}

	syminfo[nsym].ident = decl->ident;
	syminfo[nsym].type = decl->type;
	syminfo[nsym].ptrlvl = decl->ptrlvl;
	syminfo[nsym].cst = decl->cst;

	return 1;
}

static SymInfo*
searchsyminfo(int nsym, intptr ident)
{
	for (int i = nsym - 1; i >= 0; i--) {
		if (syminfo[i].ident == ident) {
			return syminfo + i;
		}
	}

	return NULL;
}


static SMember*
searchmember(intptr members, int nmember, intptr ident)
{
	SMember *member = (SMember*) ftptr(&ftast, members);
	for (int i = 0; i < nmember; i++) {
		if (member[i].ident == ident) {
			return member + i;
		}
	}
	return NULL;
}


static Bool
promotion(intptr type1, int ptrlvl1, intptr type2, int ptrlvl2, int *rtype, intptr *rptrlvl)
{
	// pointer + any langtype
	if (ptrlvl1 > 0 && ptrlvl2 == 0) {
		if (!islangtype(type2)) {
			ERR("No operation can be performed between <%s> and <%s>.", (char*) ftptr(&ftident, type1), (char*) ftptr(&ftident, type2));
			return 0;
		}

		// type 2 is a langtype
		LangType langtype = ident2langtype[type2];
		if (langtype == F32 || langtype == F64) {
			ERR("No operation can be performed between <%s> and <%s>.", (char*) ftptr(&ftident, type1), (char*) ftptr(&ftident, type2));
			return 0;
		}

		*rtype = type1;
		*rptrlvl = ptrlvl1;
		return 1;
	}

	// pointer + any langtype
	if (ptrlvl2 > 0 && ptrlvl1 == 0) {
		if (!islangtype(type1)) {
			ERR("No operation can be performed between <%s> and <%s>.", (char*) ftptr(&ftident, type1), (char*) ftptr(&ftident, type2));
			return 0;
		}

		// type 1 is a langtype
		LangType langtype = ident2langtype[type1];
		if (langtype == F32 || langtype == F64) {
			ERR("No operation can be performed between <%s> and <%s>.", (char*) ftptr(&ftident, type1), (char*) ftptr(&ftident, type2));
			return 0;
		}

		*rtype = type2;
		*rptrlvl = ptrlvl2;
		return 1;
	}

	if (ptrlvl1 > 0 && ptrlvl2 > 0) {
		ERR("No operation can be performed between pointers.");
		return 0;
	}

	if (!islangtype(type1) || !islangtype(type2)) {
		ERR("No operation can be performed between pointers.");
		return 0;
	}

	// Both are langtype.
	LangType ltype1 = ident2langtype[type1 - typeoffset];
	LangType ltype2 = ident2langtype[type2 - typeoffset];
	LangType newtype = promotiontable[ltype1][ltype2];

	*rtype = langtype2ident[newtype];
	*rptrlvl = 0;

	return 1;
}

SMember*
getstructfield(SStruct *st, intptr ident)
{
	SMember* m = (SMember*) ftptr(&ftast, st->members);
	for (int i = 0; i < st->nmember; i++) {
		if (m->ident == ident) {
			return m;
		}
		m++;
	}

	ERR("The struct <%s> as no member <%s>", identstr(st->ident), identstr(ident));
	return NULL;
}

static Bool
computeconstype(intptr expr, intptr *type, int *ptrlvl, intptr typeinfo)
{
	UnknownExpr *unknown = (UnknownExpr*) ftptr(&ftast, expr);
	switch (*unknown) {
	case ECSTI:
		*type = langtype2ident[I32];
		if (typeinfo > 0) {
			if (!islangtype(typeinfo)) {
				ERR("The type <%s> provided by the user can't match the statement: <%s>.", identstr(typeinfo), stmtstrs[*unknown]);
				return 0;
			}
			*type = typeinfo;
		}
		*ptrlvl = 0;
		return 1;
	case ECSTF:
		*type = langtype2ident[F32];
		if (typeinfo > 0) {
			if (typeinfo != langtype2ident[F32] && typeinfo != langtype2ident[F64]) {
				ERR("The type <%s> provided by the user can't match the statement: <%s>.", identstr(typeinfo), stmtstrs[*unknown]);
				return 0;
			}
			*type = typeinfo;
		}
		*ptrlvl = 0;
		return 1;
	case ECSTS:
		*type = langtype2ident[U8];
		*ptrlvl = 1;
		return 1;
	case ETRUE:
	case EFALSE:
		*type = langtype2ident[BOOL];
		*ptrlvl = 0;
		return 1;
	case ESTRUCT: {
		EStruct* st = (EStruct*) unknown;
		*type = st->ident;

		Symbol *sym = searchtopdcl(&typesym, st->ident);
		if (sym == NULL) {
			ERR("The struct <%s> is used but never declared.", identstr(st->ident));
			return 0;
		}
		SStruct *sdecl = (SStruct*) ftptr(&ftast, sym->stmt);

		EElem *elem = (EElem*) ftptr(&ftast, st->elems);
		for (int i = 0; i < st->nelem; i++) {
			SMember* m = getstructfield(sdecl, elem->ident);
			if (m == NULL) {
				ERR("Error, unknown identifier <%s> in the struct <%s>", identstr(elem->ident), identstr(sdecl->ident));
				return 0;
			}

			if (!computeconstype(elem->expr, &elem->type, &elem->ptrlvl, m->type)) {
				ERR("Error when computing the type of a struct expression.");
				return 0;
			}

			if (m->type != elem->type || m->ptrlvl != elem->ptrlvl) {
				ERR("The struct field <%s.%s> needs <%s> but it found <%s>", identstr(st->ident), identstr(elem->ident), identstr(m->type), identstr(elem->type));
				return 0;
			}
			elem++;
		}
		return 1;
	}
	default:
		ERR("Unexpected statement <%s> in a toplevel declaration.", exprstrs[*unknown]);
		return 0;
	}
	ERR("programming error: Unreachable.");
	return 0;
}

static Bool
istypedefined(intptr type)
{
	return ((typeoffset <= type) && (type <= typeend))
	       || existstopdcl(&typesym, type);
}

Bool
analyzetype(SStruct *stmt)
{
	SMember *members = (SMember*) ftptr(&ftast, stmt->members);
	for (int i = 0; i < stmt->nmember; i++) {
		intptr type = members[i].type;
		if (!istypedefined(type)) {
			ERR("Type <%s> used in <%s> but never declared", (char*) ftptr(&ftident, type), (char*) ftptr(&ftident, stmt->ident));
			return 0;
		}
	}
	return 1;
}

Bool
analyzeglobalcst(SDecl *decl, int nelem)
{
	intptr type = -1;
	int ptrlvl = 0;

	if (!computeconstype(decl->expr, &type, &ptrlvl, decl->type)) {
		ERR("Error when computing a toplevel declaration");
		return 0;
	}

	if (decl->type != -1 && (type != decl->type || ptrlvl != decl->ptrlvl)) {
		ERR("The type provided by the user doesn't match the computed one.");
		ERR("<%s  ptrlvl(%d)> != <%s ptrlvl(%d)>", identstr(decl->type), decl->ptrlvl, identstr(type), ptrlvl);
		return 0;
	}

	decl->type = type;
	decl->ptrlvl = ptrlvl;

	syminfo[nelem].type = decl->type;
	syminfo[nelem].ptrlvl = decl->ptrlvl;
	syminfo[nelem].ident = decl->ident;
	syminfo[nelem].cst = decl->cst;

	return 1;
}

static Bool
analyzebinop(EBinop *binop, intptr *type, int *ptrlvl, intptr typeinfo, int nsym)
{
	intptr type1 = -1;
	int ptrlvl1 = 0;

	intptr type2 = -1;
	int ptrlvl2 = 0;

	switch (binop->op) {
	// accepts all kind of system type.
	case OP_MUL:
	case OP_DIV:
	case OP_MOD:
	case OP_ADD:
	case OP_SUB:
		INFO("Normal ops");

		if (!analyzefunexpr(binop->left, &type1, &ptrlvl1, typeinfo, nsym)) {
			ERR("Error when computing the type of a const declaration binop.");
			return 0;
		}

		if (!analyzefunexpr(binop->right, &type2, &ptrlvl2, typeinfo, nsym)) {
			ERR("Error when computing the type of a const declaration binop.");
			return 0;
		}

		if (type1 == langtype2ident[BOOL] || type2 == langtype2ident[BOOL]) {
			ERR("Incompatible op <%s> with expressions left: <%s ptrlvl(%d)>, right: <%s ptrlvl(%d)>.", opstrs[binop->op], identstr(type1), ptrlvl1, identstr(type2), ptrlvl2);
			return 0;
		}

		if (!promotion(type1, ptrlvl1, type2, ptrlvl2, &binop->type, &binop->ptrlvl)) {
			ERR("Error when promoting the type of a const declaration binop.");
			return 0;
		}
		assert(binop->type != 0);

		*type = binop->type;
		*ptrlvl = binop->ptrlvl;
		INFO("ptrlvl: %d", *ptrlvl);
		return 1;

	// makes boolean, ensure that both have the same type.
	case OP_LT:
	case OP_GT:
	case OP_LE:
	case OP_GE:
	case OP_EQUAL:
	case OP_NEQUAL:
	case OP_LAND:
	case OP_LOR:
	case OP_BNOT:
	case OP_LNOT:
		INFO("compare ops");
		if (!analyzefunexpr(binop->left, &type1, &ptrlvl1, -1, nsym)) {
			ERR("Error when computing the type of a const declaration binop.");
			return 0;
		}

		if (!analyzefunexpr(binop->right, &type2, &ptrlvl2, -1, nsym)) {
			ERR("Error when computing the type of a const declaration binop.");
			return 0;
		}

		if (!islangtype(type1) || !islangtype(type2)) {
			ERR("Error, binops can only be done on the language types.");
			TODO("Verify if it's a ptr...");
			return 0;
		}

		if (type1 != type2 || ptrlvl1 != ptrlvl2) {
			ERR("Impossible to compare two things with a different types.");
			ERR("<%s  ptrlvl(%d)> != <%s ptrlvl(%d)>", identstr(type1), ptrlvl1, identstr(type2), ptrlvl2);
			return 0;
		}

		binop->type = langtype2ident[BOOL];
		binop->ptrlvl = 0;

		*type = binop->type;
		*ptrlvl = binop->ptrlvl;
		INFO("ptrlvl: %d", *ptrlvl);
		return 1;

	// bit ops
	case OP_LSHIFT:
	case OP_RSHIFT:
	case OP_BAND:
	case OP_BXOR:
	case OP_BOR:
		INFO("Bit ops");
		if (!analyzefunexpr(binop->left, &type1, &ptrlvl1, typeinfo, nsym)) {
			ERR("Error when computing the type of a const declaration binop.");
			return 0;
		}

		if (!analyzefunexpr(binop->right, &type2, &ptrlvl2, typeinfo, nsym)) {
			ERR("Error when computing the type of a const declaration binop.");
			return 0;
		}

		if (!promotion(type1, ptrlvl1, type2, ptrlvl2, &binop->type, &binop->ptrlvl)) {
			ERR("Error when promoting the type of a const declaration binop.");
			return 0;
		}

		if (!islangtype(binop->type)) {
			ERR("Error, binops can only be done on the language types.");
			TODO("Verify if it's a ptr...");
			return 0;
		}

		LangType ltype = ident2langtype[binop->type - typeoffset];
		if (ltype == F32 || ltype == F64 || ltype == BOOL) {
			ERR("Incompatible op <%s> with expressions left: <%s ptrlvl(%d)>, right: <%s ptrlvl(%d)>.", opstrs[binop->op], identstr(type1), ptrlvl1, identstr(type2), ptrlvl2);
			return 0;
		}

		*type = binop->type;
		*ptrlvl = binop->ptrlvl;
		INFO("ptrlvl: %d", *ptrlvl);
		return 1;


	default:
		ERR("Unreachable op <%s>.", opstrs[binop->op]);
		return 0;
	}

	ERR("Unreachable code.");
	return 0;
}

static Bool
analyzeunop(EUnop *unop, intptr *type, int *ptrlvl, intptr typeinfo, int nsym)
{
	switch (unop->op) {
	case UOP_SUB:
	//fallthrough
	case UOP_LNOT:
	//fallthrough
	case UOP_BNOT: {
		if (!analyzefunexpr(unop->expr, &unop->type, &unop->ptrlvl, typeinfo, nsym)) {
			ERR("Error when computing an unop <%s>.", uopstrs[unop->op]);
			return 0;
		}

		if (*ptrlvl > 0) {
			ERR("Error the unop <%s> can't be used on a pointer.", uopstrs[unop->op]);
			return 0;
		}

		*type = unop->type;
		*ptrlvl = unop->ptrlvl;
		return 1;
	}
	case UOP_DEREF:
		if (!analyzefunexpr(unop->expr, &unop->type, &unop->ptrlvl, typeinfo, nsym)) {
			ERR("Error when computing an unop <%s>.", uopstrs[unop->op]);
			return 0;
		}

		if (unop->ptrlvl < 1) {
			ERR("Trying to deref a non ptr.");
			return 0;
		}

		*type = unop->type;
		*ptrlvl = (--unop->ptrlvl);

		return 1;
	case UOP_AT:
		if (!analyzefunexpr(unop->expr, &unop->type, &unop->ptrlvl, typeinfo, nsym)) {
			ERR("Error when computing an unop <%s>.", uopstrs[unop->op]);
			return 0;
		}
		*type = unop->type;
		*ptrlvl = (++unop->ptrlvl);

		return 1;
	case UOP_SIZEOF:
		if (!analyzefunexpr(unop->expr, &unop->type, &unop->ptrlvl, typeinfo, nsym)) {
			ERR("Error when computing an unop <%s>.", uopstrs[unop->op]);
			return 0;
		}
		*type = unop->type;
		*ptrlvl = unop->ptrlvl;

		return 1;
	default:
		ERR("Unreachable unop op <%d>.", unop->op);
		return 0;
	}

	ERR("Unreachable code.");
	return 0;
}

static Bool
analyzestruct(EStruct *st, intptr *type, int nsym)
{
	*type = st->ident;

	Symbol *sym = searchtopdcl(&typesym, st->ident);
	if (sym == NULL) {
		ERR("The struct <%s> is used but never declared.", identstr(st->ident));
		return 0;
	}
	SStruct *sdecl = (SStruct*) ftptr(&ftast, sym->stmt);

	EElem *elem = (EElem*) ftptr(&ftast, st->elems);
	for (int i = 0; i < st->nelem; i++) {
		SMember* m = getstructfield(sdecl, elem->ident);
		if (m == NULL) {
			ERR("Error, unknown identifier <%s> in the struct <%s>", identstr(elem->ident), identstr(sdecl->ident));
			return 0;
		}

		if (!analyzefunexpr(elem->expr, &elem->type, &elem->ptrlvl, m->type, nsym)) {
			ERR("Error when computing the type of a struct expression.");
			return 0;
		}

		if (m->type != elem->type || m->ptrlvl != elem->ptrlvl) {
			ERR("The struct field <%s.%s> needs <%s> but it found <%s>", identstr(st->ident), identstr(elem->ident), identstr(m->type), identstr(elem->type));
			return 0;
		}
		elem++;
	}
	return 1;
}

static Bool
analyzecall(ECall *call, intptr *type, int *ptrlvl, int nsym)
{
	Mem* expr = (Mem*) ftptr(&ftast, call->expr);

	if (expr->kind != EMEM) {
		TODO("Function pointer isn't available yet.");
		return 0;
	}

	Symbol *sym = searchtopdcl(&signatures, expr->addr);
	if (sym == NULL) {
		ERR("The function <%s> is called but never declared.", identstr(expr->addr));
		return 0;
	}
	SFun *stmt = (SFun*) ftptr(&ftast, sym->stmt);
	assert(stmt->kind == SFUN);

	call->type = stmt->type;
	call->ptrlvl = stmt->ptrlvl;

	*type = stmt->type;
	*ptrlvl = stmt->ptrlvl;

	if (stmt->nparam != call->nparam) {
		ERR("The number of param (%d) in the function call of <%s> doesn't match with the signature (%d).", call->nparam, identstr(expr->addr), stmt->nparam);
		return 0;
	}

	intptr *exprs = (intptr*) ftptr(&ftast, call->params);
	SMember *params = (SMember*) ftptr(&ftast, stmt->params);
	for (int i = 0; i < call->nparam; i++) {
		intptr type;
		int ptrlvl;
		if (!analyzefunexpr(exprs[i], &type, &ptrlvl, params[i].type, nsym)) {
			ERR("Error when parsing the expression in a fun call expression.");
			return 0;
		}

		if (type != params[i].type || ptrlvl != params[i].ptrlvl) {
			ERR("The param type <%s of <%s> doesn't match with the signature.", identstr(params[i].ident), identstr(expr->addr));
			return 0;
		}
	}
	INFO("OK for ECALL");
	return 1;
}

static Bool
analyzeaccess(EAccess *ac, intptr *type, int *ptrlvl, int nsym)
{
	INFO("analyzeaccess");
	Bool res = 0;

	intptr structtype = -1;
	int structptrlvl = 0;

	res = analyzefunexpr(ac->expr, &structtype, &structptrlvl, -1, nsym);
	if (res == 0) {
		ERR("Error when analyzefunexpr.");
		return 0;
	}

	if (islangtype(structtype)) {
		ERR("There is no field in the langtype.");
		return 0;
	}

	if (structptrlvl != 0) {
		ERR("Accessing of the field of a struct pointer is forbidden. Derefence it before.");
		return 0;
	}

	Symbol *s = searchtopdcl(&typesym, structtype);
	assert(s != NULL);

	SStruct* st = (SStruct*) ftptr(&ftast, s->stmt);
	assert(st != NULL);

	SMember *sm = searchmember(st->members, st->nmember, ac->ident);
	if (sm == NULL) {
		ERR("Access to a field that doesn't exist <%s.%s>.", identstr(st->ident), identstr(ac->ident));
		return 0;
	}

	ac->type = sm->type;
	ac->ptrlvl = sm->ptrlvl;
	*type = ac->type;
	*ptrlvl = ac->ptrlvl;

	return 1;
}

static Bool
analyzefunexpr(intptr expr, intptr *type, int *ptrlvl, intptr typeinfo, int nsym)
{
	int res = 0;
	UnknownExpr *unknown = (UnknownExpr*) ftptr(&ftast, expr);
	switch (*unknown) {
	case ECSTI:
		*type = langtype2ident[I32];
		if (typeinfo > 0) {
			if (!islangtype(typeinfo)) {
				ERR("The type <%s> provided by the user can't match the expression type: <%s>.", identstr(typeinfo), exprstrs[*unknown]);
				return 0;
			}
			if (typeinfo == langtype2ident[F32] || typeinfo == langtype2ident[F64] || typeinfo == langtype2ident[BOOL]) {
				ERR("The type <%s> provided by the user can't match the expression type: <%s>.", identstr(typeinfo), exprstrs[*unknown]);
				return 0;
			}
			*type = typeinfo;
		}
		*ptrlvl = 0;
		return 1;
	case ECSTF:
		*type = langtype2ident[F32];
		if (typeinfo > 0) {
			if (typeinfo != langtype2ident[F32] && typeinfo != langtype2ident[F64]) {
				ERR("The type <%s> provided by the user can't match the expression type: <%s>.", identstr(typeinfo), exprstrs[*unknown]);
				return 0;
			}
			*type = typeinfo;
		}
		*ptrlvl = 0;
		return 1;
	case ECSTS:
		*type = langtype2ident[U8];
		*ptrlvl = 1;
		return 1;
	case ETRUE:
	case EFALSE:
		*type = langtype2ident[BOOL];
		*ptrlvl = 0;
		return 1;
	case EBINOP: {
		EBinop *binop = (EBinop*) unknown;
		res = analyzebinop(binop, type, ptrlvl, typeinfo, nsym);
		binop->ptrlvl = *ptrlvl;
		binop->type = *type;
		return res;
	}
	case EUNOP: {
		EUnop *unop = (EUnop*) unknown;
		res = analyzeunop(unop, type, ptrlvl, typeinfo, nsym);
		unop->ptrlvl = *ptrlvl;
		unop->type = *type;
		return res;
	}
	case ESTRUCT: {
		EStruct* st = (EStruct*) unknown;
		*ptrlvl = 0;
		return analyzestruct(st, type, nsym);
	}
	case ECALL: {
		ECall *call = (ECall*) unknown;
		return analyzecall(call, type, ptrlvl, nsym);
	}
	case EMEM: {
		Mem *mem = (Mem*) unknown;
		SymInfo *sym = searchsyminfo(nsym, mem->addr);

		if (sym == NULL) {
			ERR("Use of the identifier <%s> which has never been declared.", identstr(mem->addr));
			return 0;
		}

		mem->type = sym->type;
		mem->ptrlvl = sym->ptrlvl;
		*type = sym->type;
		*ptrlvl = sym->ptrlvl;
		return 1;
	}
	case EACCESS: {
		EAccess *ac = (EAccess*) unknown;
		return analyzeaccess(ac, type, ptrlvl, nsym);
	}

	default:
		ERR("Unexpected statement <%s> in a toplevel declaration.", exprstrs[*unknown]);
		return 0;
	}
	ERR("programming error: Unreachable.");
	return 0;
}

static Bool
analyzefunstmt(const SFun *fun, int *nsym, int block, intptr stmt)
{
	UnknownStmt *unknown = (UnknownStmt*) ftptr(&ftast, stmt);
	switch (*unknown) {
	case SSEQ: {
		INFO("SSEQ");

		SSeq* seq = (SSeq*) unknown;

		if (!analyzefunstmt(fun, nsym, block, seq->stmt)) {
			ERR("Error when analyzing a seq in a function.");
			return 0;
		}

		if (!analyzefunstmt(fun, nsym, block, seq->nxt)) {
			ERR("Error when analyzing a seq in a function.");
			return 0;
		}

		return 1;
	}
	case SDECL: {
		INFO("SDECL");

		intptr type;
		int ptrlvl;

		SDecl *decl = (SDecl*)unknown;
		if (!analyzefunexpr(decl->expr, &type, &ptrlvl, decl->type, *nsym)) {
			ERR("Error when analyzing a declaration in a function.");
			return 0;
		}

		if ((decl->type > 0) && (decl->type != type || decl->ptrlvl != ptrlvl)) {
			ERR("Incompatible type annotation with the infered type.");
			return 0;
		}

		decl->type = type;
		decl->ptrlvl = ptrlvl;

		if (!insertsyminfo(*nsym, block, decl)) {
			ERR("Error when adding a declaration in a function.");
			return 0;
		}
		*nsym += 1;

		return 1;
	}
	case SIF: {
		INFO("IF");
		intptr type;
		int ptrlvl;

		SIf* _if = (SIf*)unknown;
		if (!analyzefunexpr(_if->cond, &type, &ptrlvl, -1, *nsym)) {
			ERR("Error when analyzing the condition of a if.");
			return 0;
		}

		if (type != langtype2ident[BOOL] ||  ptrlvl != 0) {
			ERR("Error the if expression must be a bool expression.");
			return 0;
		}

		int newnsym = *nsym;
		if (!analyzefunstmt(fun, &newnsym, newnsym, _if->ifstmt)) {
			ERR("Error when analyzing the if stmt of a if.");
			return 0;
		}

		newnsym = *nsym;
		if (_if->elsestmt != -1 && !analyzefunstmt(fun, &newnsym, newnsym, _if->elsestmt)) {
			ERR("Error when analyzing the else stmt of a if.");
			return 0;
		}
		return 1;
	}
	case SRETURN: {
		INFO("SRETURN");
		SReturn *ret = (SReturn*) unknown;
		intptr type = -1;
		int ptrlvl = -1;

		if (!analyzefunexpr(ret->expr, &type, &ptrlvl, fun->type, *nsym)) {
			ERR("Error when analyzing the return statement.");
			return 0;
		}

		if (fun->type != type || fun->ptrlvl != ptrlvl) {
			ERR("Error the return expression <%s: ptrlvl %d> isn't the same as the function return type <%s: ptrlvl %d>.", identstr(type), ptrlvl, identstr(fun->type), fun->ptrlvl);
			return 0;
		}
		INFO("OK ret");

		return 1;
	}
	case SFOR: {
		INFO("SFOR");

		intptr type;
		int ptrlvl;

		SFor *f = (SFor*)unknown;

		int newnsym = *nsym;
		int newblock = newnsym;

		if (!analyzefunstmt(fun, &newnsym, newblock, f->stmt1)) {
			ERR("Error when analyzing the first stmt of a for.");
			return 0;
		}

		if (!analyzefunstmt(fun, &newnsym, newblock, f->stmt2)) {
			ERR("Error when analyzing the second stmt of a for.");
			return 0;
		}

		if (!analyzefunexpr(f->expr, &type, &ptrlvl, -1, newnsym)) {
			ERR("Error when analyzing the condition of a if.");
			return 0;
		}

		if (type != langtype2ident[BOOL] ||  ptrlvl != 0) {
			ERR("Error the for expression must be a bool expression.");
			return 0;
		}

		newblock = newnsym;
		if (!analyzefunstmt(fun, &newnsym, newblock, f->forstmt)) {
			ERR("Error when analyzing the body stmt of a for.");
			return 0;
		}
		return 1;
	}
	case SCALL: {
		INFO("SCALL");

		SCall *call = (SCall*) unknown;

		Symbol *sym = searchtopdcl(&signatures, call->ident);
		if (sym == NULL) {
			ERR("The function <%s> is called but never declared.", identstr(call->ident));
			return 0;
		}
		SFun *stmt = (SFun*) ftptr(&ftast, sym->stmt);
		assert(stmt->kind == SFUN || stmt->kind == SSIGN);

		if (stmt->nparam != call->nparam) {
			ERR("The number of param (%d) in the function call of <%s> doesn't match with the signature (%d).", call->nparam, identstr(call->ident), stmt->nparam);
			return 0;
		}

		intptr *exprs = (intptr*) ftptr(&ftast, call->params);
		SMember *params = (SMember*) ftptr(&ftast, stmt->params);
		for (int i = 0; i < call->nparam; i++) {
			intptr type;
			int ptrlvl;
			if (!analyzefunexpr(exprs[i], &type, &ptrlvl, params[i].type, *nsym)) {
				ERR("Error when parsing the expression in a fun call expression.");
				return 0;
			}

			if (type != params[i].type || ptrlvl != params[i].ptrlvl) {
				ERR("The param type <%s of <%s> doesn't match with the signature.", identstr(params[i].ident), identstr(call->ident));
				return 0;
			}
		}
		return 1;
	}
	case SASSIGN: {
		INFO("SASSIGN");
		SAssign *a = (SAssign*) unknown;

		SymInfo *sym = searchsyminfo(*nsym, a->ident);
		if (sym == NULL) {
			ERR("Use of the identifier <%s> which has never been declared.", identstr(a->ident));
			return 0;
		}

		if (sym->cst) {
			ERR("Trying to change the value of the <%s> constant.", identstr(sym->ident));
			return 0;
		}

		intptr type = -1;
		int ptrlvl = -1;
		if (!analyzefunexpr(a->expr, &type, &ptrlvl, sym->type, *nsym)) {
			ERR("Error when parsing the expression in an assign.");
			return 0;
		}

		if (type != sym->type || ptrlvl != sym->ptrlvl) {
			ERR("Assign in the var <%s> of the type <%s : ptrlvl(%d)> an expression of the type <%s : ptrlvl(%d)>.", identstr(a->ident), identstr(sym->type), sym->ptrlvl, identstr(type), ptrlvl);
			return 0;
		}

		return 1;
	}
	case SEXPRASSIGN: {
		INFO("SEXPRASSIGN");
		SExprAssign *a = (SExprAssign*) unknown;

		UnknownExpr *left = (UnknownExpr*) ftptr(&ftast, a->left);
		if (*left != ESUBSCR && *left != EACCESS && *left != EMEM) {
			ERR("Unexpected expression <%s> at the left expression of an assign.", exprstrs[*left]);
			return 0;
		}

		intptr ltype = -1;
		int lptrlvl = -1;
		if (!analyzefunexpr(a->left, &ltype, &lptrlvl, -1, *nsym)) {
			ERR("Error when parsing the expression in an assign.");
			return 0;
		}

		intptr rtype = -1;
		int rptrlvl = -1;
		if (!analyzefunexpr(a->right, &rtype, &rptrlvl, ltype, *nsym)) {
			ERR("Error when parsing the expression in an assign.");
			return 0;
		}

		if (ltype != rtype || lptrlvl != rptrlvl) {
			ERR("Assign: left is <%s : ptrlvl(%d)> and right <%s : ptrlvl(%d)>.", identstr(ltype), lptrlvl, identstr(rtype), rptrlvl);
			return 0;
		}

		return 1;
	}
	default:
		ERR("Unexpected <%s> Statement in a function.", stmtstrs[*unknown]);
		return 0;
	}
	return 0;
}


static Bool
inserttopdcl(Symbols *syms, intptr ident, intptr stmt)
{
	Symbol *sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		if (ident == sym[i].ident) {
			return 0;
		}
	}
	sym[syms->nsym] = (Symbol) {
		.ident = ident, .stmt = stmt
	};
	syms->nsym++;

	return 1;
}

Bool
analyzefun(SFun *fun, intptr stmt, int nsym)
{
	// Verify the params
	SMember *member = (SMember*) ftptr(&ftast, fun->params);
	for (int i = 0; i < fun->nparam; i++) {
		if (!istypedefined(member->type)) {
			ERR("Param %d: <%s> used in the function <%s> but never declared", i, (char*) ftptr(&ftident, member->type), (char*) ftptr(&ftident, fun->ident));
			return 0;
		}
		syminfo[nsym].type = member->type;
		syminfo[nsym].ptrlvl = member->ptrlvl;
		syminfo[nsym].ident = member->ident;
		nsym++;
		member++;
	}

	if (fun->type >= 0 && !istypedefined(fun->type)) {
		ERR("Return type <%s> used in the function <%s> but never declared", (char*) ftptr(&ftident, fun->type), (char*) ftptr(&ftident, fun->ident));
		return 0;
	}

	switch (fun->kind) {
	case SSIGN:
		return 1;
	case SFUN:
		assert(inserttopdcl(&funsym, fun->ident, stmt) == 1);
		break;
	default:
		assert(0 && "Unreachable case");
		return 0;
	}

	int newnsym = nsym;
	if (!analyzefunstmt(fun, &newnsym, newnsym, fun->stmt)) {
		ERR("Error when analyzing the fun statements");
		return 0;
	}

	return 1;
}

Symbol*
searchmod(intptr ident, EModSym mod)
{
	Symbols s = modsym[mod];
	Symbol *ss = (Symbol*) ftptr(&ftsym, s.array);
	for (int i = 0; i < s.nsym; i++) {
		if (ss[i].ident == ident) {
			return ss + i;
		}
	}
	return NULL;
}

static Bool
issignatureequivalent(SSignature *s, SModSign *sign)
{
	SGenerics *g1 = (SGenerics*) ftptr(&ftast, s->generics);
	SGenerics *g2 = (SGenerics*) ftptr(&ftast, sign->generics);

	if (g1->ngen == g2->ngen) {
		return 1;
	}
	return 0;
}

static Bool
existgenerics(SGenerics *g, intptr ident)
{
	if (g == NULL) {
		return 0;
	}

	intptr *gens = (intptr*) ftptr(&ftast, g->generics);
	for (int i = 0; i < g->ngen; i++) {
		if (gens[i] == ident) {
			return 1;
		}
	}
	return 0;
}

static Bool
g1containsg2(SGenerics *g1, SGenerics *g2)
{
	if (g1 == NULL) {
		ERR("No generic provided.");
		return 0;
	}

	intptr *gens = (intptr*) ftptr(&ftast, g2->generics);
	for (int i = 0; i < g2->ngen; i++) {
		if (!existgenerics(g1, gens[i])) {
			ERR("%s not found.", identstr(gens[i]));
			return 0;
		}
	}
	return 1;
}

static Bool
sign_is_generic_defined(SGenerics *g1, SSignature *s)
{
	Symbol *sym = searchmod(s->signature, MODSIGN);
	// Verify if a signature is defined.
	if (sym == NULL) {
		ERR("The signature %s is used but it has never been defined.", identstr(s->signature));
		return 0;
	}

	// Verify if the existing signature is equivalent with the used one.
	SModSign *sign = (SModSign*) ftptr(&ftast, sym->stmt);
	assert(sign->kind == SMODSIGN);

	if (!issignatureequivalent(s, sign)) {
		ERR("The signature %s is used isn't compatible with the defined one.", identstr(s->signature));
		return 0;
	}

	// Verify if the generics of the signatures are declared.
	if (s->generics != -1) {
		SGenerics *g2 = (SGenerics*) ftptr(&ftast, s->generics);
		if (!g1containsg2(g1, g2)) {
			ERR("The signature %s use a non defined generic.", identstr(s->signature));
			return 0;
		}
	}

	return 1;
}

static Bool
sign_are_generics_defined(intptr generics, intptr signatures)
{
	// No signatures
	if (signatures == -1) {
		return 1;
	}

	// No generics
	SGenerics *g = NULL;
	if (generics != -1) {
		g = (SGenerics*) ftptr(&ftast, generics);
	}

	// Generics and signatures
	SSignatures *s = (SSignatures*) ftptr(&ftast, signatures);
	SSignature *signs = (SSignature*) ftptr(&ftast, s->signs);
	for (int i = 0; i < s->nsign; i++) {
		if (!sign_is_generic_defined(g, signs + i)) {
			ERR("Usage of a undefined signature in the definition of a signature.");
			return 0;
		}
	}

	return 1;
}

static Bool
sign_is_type_exist(SModSign *s, SGenerics *g, intptr ident, int ptrlvl, Bool verifyptrlvl)
{
	(void) s;

	if (istypedefined(ident)) {
		return 1;
	}

	if (existgenerics(g, ident)) {
		if (!verifyptrlvl) {
			return 1;
		}
		return ptrlvl > 0;
	}

	TODO("Verify if the struct is defined in the module.");
	return 0;
}

static Bool
sign_analyze_stmt(SModSign *sign, SGenerics *g, intptr stmt)
{
	UnknownStmt *unknown = (UnknownStmt*) ftptr(&ftast, stmt);
	switch (*unknown) {
	case SSTRUCT: {
		SStruct *s = (SStruct*) unknown;
		// Verify members
		if (s->nmember > 0) {
			SMember *m = (SMember*) ftptr(&ftast, s->members);
			for (int i = 0; i < s->nmember; i++) {
				if (!sign_is_type_exist(sign, g, m[i].type, m[i].ptrlvl, 1)) {
					ERR("Using of <%s ptrlvl(%d)> which is undefined or a generic but not a pointer.", identstr(m[i].type), m[i].ptrlvl);
					ERR("In the struct <%s>.", identstr(s->ident));
					return 0;
				}
			}
		}
		break;
	}
	case SSIGN: {
		SSign *s = (SSign*) unknown;
		// Verify return type
		if (!sign_is_type_exist(sign, g, s->type, s->ptrlvl, 1)) {
			ERR("Using an undefined type <%s>.", identstr(s->type));
			ERR("In the fun <%s>.", identstr(s->ident));
			return 0;
		}

		// Verify params
		if (s->nparam > 0) {
			SMember *m = (SMember*) ftptr(&ftast, s->params);
			for (int i = 0; i < s->nparam; i++) {
				if (!sign_is_type_exist(sign, g, m[i].type, m[i].ptrlvl, 1)) {
					ERR("Using of <%s ptrlvl(%d)> which is undefined or a generic but not a pointer.", identstr(m[i].type), m[i].ptrlvl);
					ERR("In the fun <%s>.", identstr(s->ident));
					return 0;
				}
			}
		}

		break;
	}
	default:
		ERR("Unexpected stmt <%s> in a module signature.", stmtstrs[*unknown]);
		return 0;
	}

	return 1;
}

Bool
analyzemodsign(SModSign *sign)
{
	// Verify the generics.
	if (!sign_are_generics_defined(sign->generics, sign->signatures)) {
		ERR("The module signature <%s> isn't correct.", identstr(sign->ident));
		return 0;
	}

	// Verify that there is only SSign or SStruct + analyze.
	intptr* stmt = (intptr*) ftptr(&ftast, sign->stmts);
	for (int i = 0; i < sign->nstmt; i++) {
		SGenerics *g = NULL;
		if (sign->generics != -1) {
			g = (SGenerics*) ftptr(&ftast, sign->generics);
		}

		if (!sign_analyze_stmt(sign, g, stmt[i])) {
			ERR("The stmts of the module signature <%s> isn't correct.", identstr(sign->ident));
			return 0;
		}
	}

	return 1;
}

static intptr
searchreal(intptr convtab, int nconv, intptr generic)
{
	SConv *c = (SConv*) ftptr(&ftast, convtab);

	for (int i = 0; i < nconv; i++) {
		if (c[i].gen == generic) {
			return c[i].real;
		}
	}

	return -1;
}

static Bool
getmodinfo(intptr module, intptr *generics, intptr *signature)
{
	Symbol *s = NULL;

	s = searchtopdcl(modsym + MODIMPL, module);
	if (s != NULL) {
		SModImpl *i = (SModImpl*) ftptr(&ftast, s->stmt);
		assert(i->kind == SMODIMPL);
		*generics = i->generics;
		*signature = i->signature;
		return 1;
	}

	s = searchtopdcl(modsym + MODDEF, module) ;
	if (s != NULL) {
		SModDef *d = (SModDef*) ftptr(&ftast, s->stmt);
		assert(d->kind == SMODDEF);

		Symbol *skel = searchtopdcl(modsym + MODSKEL, d->skeleton);
		if (skel == NULL) {
			ERR("The module <%s> define an undefined skeleton <%s>.", identstr(d->ident), identstr(d->skeleton));
			return 0;
		}
		SModSkel *sk = (SModSkel*) ftptr(&ftast, skel->stmt);
		assert(sk->kind == SMODSKEL);

		*generics = d->generics;
		*signature = sk->signature;
		return 1;
	}

	ERR("The module <%s> is undefined.", identstr(module));
	return 0;
}

static Bool
impl_verify_generics(SModImpl *impl, SModSign *sign)
{
	SGenerics *g1 = NULL;
	SGenerics *g2 = NULL;

	if (impl->generics != -1) {
		g1 = (SGenerics*) ftptr(&ftast, impl->generics);
	}

	if (sign->generics != -1) {
		g2 = (SGenerics*) ftptr(&ftast, sign->generics);
	}

	if ((g1 == NULL || g2 == NULL) && g1 != g2) {
		ERR("Incompatible generics of <%s> with its signature <%s>.", identstr(impl->ident), identstr(impl->signature));
		return 0;
	}

	if (g1 != NULL) {
		if (g1->ngen != g2->ngen) {
			ERR("Incompatible generics of <%s> with its signature <%s>.", identstr(impl->ident), identstr(impl->signature));
			return 0;
		}

		// Build the conversion table, gentype -> real type.
		intptr *t1 = (intptr*) ftptr(&ftast, g1->generics);
		intptr *t2 = (intptr*) ftptr(&ftast, g2->generics);

		{
			int i = 0;
			impl->convtab = ftalloc(&ftast, sizeof(SConv));
			impl->nconv = g1->ngen;
			SConv *c = (SConv*) ftptr(&ftast, impl->convtab);

			for (; i < g1->ngen; i++) {
				if (!istypedefined(t1[i])) {
					ERR("In <%s> the type <%s> is used but never defined.", identstr(impl->ident), identstr(t1[i]));
					return 0;
				}

				c[i].real = t1[i];
				c[i].gen = t2[i];
				(void) ftalloc(&ftast, sizeof(SConv));
			}
		}
	}
	return 1;
}

static Bool
impl_verify_module(SModImpl *impl, SModSign *sign)
{
	SSignatures *s1 = NULL;
	SSignatures *s2 = NULL;

	if (impl->modules != -1) {
		s1 = (SSignatures*) ftptr(&ftast, impl->modules);
	}

	if (sign->signatures != -1) {
		s2 = (SSignatures*) ftptr(&ftast, sign->signatures);
	}

	if ((s1 == NULL || s2 == NULL) && s1 != s2) {
		ERR("Incompatible modules of <%s> with its signature <%s>.", identstr(impl->ident), identstr(impl->signature));
		return 0;
	}

	if (s1 != NULL) {
		TODO("Accept the module params in an implementation.");
		if (s1->nsign != s2->nsign) {
			ERR("Incompatible modules of <%s> with its signature <%s>.", identstr(impl->ident), identstr(impl->signature));
			return 0;
		}

		SSignature *ss1 = (SSignature*) ftptr(&ftast, s1->signs);
		SSignature *ss2 = (SSignature*) ftptr(&ftast, s2->signs);
		for (int i = 0; i < s1->nsign; i++) {
			if (ss1->ident != ss2->ident) {
				ERR("The %d module params are incompatible <%s> != <%s>", i, identstr(ss1->ident), identstr(ss2->ident));
				ERR("Incompatible module params of <%s> with its signature <%s>.", identstr(impl->ident), identstr(impl->signature));
				return 0;
			}

			intptr mgen = -1;
			intptr msign = -1;
			// search module...
			if (!getmodinfo(ss1[i].signature, &mgen, &msign)) {
				ERR("Implementing <%s> with the undefined module <%s>.", identstr(impl->ident), identstr(ss1[i].signature));
				return 0;
			}

			// verify module signature
			if (ss2[i].signature != msign) {
				ERR("In <%s> using <%s: %s> as the signature type of <%s> but <%s> is required.", identstr(impl->ident), identstr(ss1[i].ident), identstr(ss1[i].signature), identstr(msign), identstr(ss2[i].signature));
				return 0;
			}
			
			// verify module generics
			SGenerics *ss2gen = NULL;
			if (ss2[i].generics != -1) {
				ss2gen = (SGenerics*) ftptr(&ftast, ss2[i].generics);
			}

			SGenerics *mgengen = NULL;
			if (mgen != -1) {
				mgengen = (SGenerics*) ftptr(&ftast, mgen);
			}

			if ((ss2gen == NULL || mgengen == NULL) && ss2gen != mgengen) {
				ERR("In <%s> using <%s: %s> as the signature type of <%s> but generics can't match signatures", identstr(impl->ident), identstr(ss1[i].ident), identstr(ss1[i].signature), identstr(msign));
				return 0;
			}

			if (ss2gen != NULL) {
				if (ss2gen->ngen != mgengen->ngen) {
					ERR("In <%s> using <%s: %s> as the signature type of <%s> but generics can't match signatures", identstr(impl->ident), identstr(ss1[i].ident), identstr(ss1[i].signature), identstr(msign));
					return 0;
				}

				intptr *t1 = (intptr*) ftptr(&ftast, mgengen->generics);
				intptr *t2 = (intptr*) ftptr(&ftast, ss2gen->generics);
				for (int j = 0; j < ss2gen->ngen; j++) {
					if (!(searchreal(impl->convtab, impl->nconv, t2[j]) == t1[j])) {
						ERR("In <%s> using <%s: %s> as the signature type of <%s> but generics can't match signatures", identstr(impl->ident), identstr(ss1[i].ident), identstr(ss1[i].signature), identstr(msign));
						return 0;
					}
				}
			}
		}
	}
	return 1;
}

static Bool
impl_analyzefun(intptr convtab, int nconv, SFun *impl, SSign *sign)
{
	if (impl->ident != sign->ident) {
		ERR("Expects the functions in the same order in the impl and in the signature.");
		return 0;
	}
	int signtype = istypedefined(sign->type) ? sign->type : searchreal(convtab, nconv, sign->type);
	assert(signtype > 0 && "Unreachable assert.");

	if (signtype != impl->type || impl->ptrlvl != sign->ptrlvl) {
		ERR("Incompatible return type in <%s>, signature <%s ptrlvl(%d)> impl <%s ptrlvl(%d)>.",
		 identstr(impl->ident), identstr(sign->type), sign->ptrlvl, identstr(impl->type), impl->ptrlvl);
		return 0;
	}

	if (sign->nparam != impl->nparam) {
		ERR("Incompatible number of param in <%s>.", identstr(impl->ident));
		return 0;
	}

	SMember *m1 = (SMember*) ftptr(&ftast, impl->params);
	SMember *m2 = (SMember*) ftptr(&ftast, sign->params);
	int localnsym = nsym;
	for (int i = 0; i < impl->nparam; i++) {
		if (m1[i].ident != m2[i].ident) {
			ERR("Incompatible param identifier in <%s>, <%s> != <%s>.", identstr(impl->ident), identstr(m1[i].ident), identstr(m2[i].ident));
			return 0;
		}

		int signtype = istypedefined(m2[i].type) ? m2[i].type : searchreal(convtab, nconv, m2[i].type);
		assert(signtype > 0 && "Unreachable assert.");
		if (signtype != m1[i].type || m1[i].ptrlvl != m2[i].ptrlvl) {
			ERR("Incompatible param type in <%s> of <%s>, signature <%s ptrlvl(%d)> impl <%s ptrlvl(%d)>.",
			 identstr(impl->ident), identstr(m1[i].ident), identstr(m2[i].type), m2[i].ptrlvl, identstr(m1[i].type), m1[i].ptrlvl);
			return 0;
		}
		syminfo[localnsym].type = m1[i].type;
		syminfo[localnsym].ptrlvl = m1[i].ptrlvl;
		syminfo[localnsym].ident = m1[i].ident;
		localnsym++;
	}

	if (!analyzefunstmt(impl, &localnsym, localnsym, impl->stmt)) {
		ERR("Error when analyzing fun stmt in an implementation.");
		return 0;
	}

	// Analyze function stmt
	return 1;
}

static Bool
impl_verify_stmts(SModImpl *impl, SModSign *sign)
{
	if (impl->nstmt > sign->nstmt) {
		ERR("There is more stmt in the the implementation than in the signature.");
		return 0;
	}

	intptr *stmt1 = (intptr*) ftptr(&ftast, impl->stmts);
	intptr *stmt2 = (intptr*) ftptr(&ftast, sign->stmts);

	int i2 = 0;
	for (int i1 = 0; i1 < impl->nstmt; i1++) {

		SSign *f2 = (SSign*) ftptr(&ftast, stmt2[i2]);
		while (f2->kind != SSIGN) {
			i2++;
			f2 = (SFun*) ftptr(&ftast, stmt2[i2]);
			if (i2 >= sign->nstmt) {
				ERR("Statements of the impl are incompatible with the statements of the signature.");
				return 0;
			}
		}

		SFun *f1 = (SFun*) ftptr(&ftast, stmt1[i1]);
		if (f1->kind != SFUN) {
			ERR("Unexpected statement <%s> in an impl.", stmtstrs[f1->kind]);
			return 0;
		}

		// Verify function
		if (!impl_analyzefun(impl->convtab, impl->nconv, f1, f2)) {
			ERR("Incompatible functions.");
			return 0;
		}
	}

	return 1;
}

Bool
analyzemodimpl(SModImpl *impl)
{
	// Search the module signature.
	Symbol *s = searchtopdcl(modsym + MODSIGN, impl->signature);
	if (s == NULL) {
		ERR("Impl of <%s> an undefined signature <%s>", identstr(impl->ident), identstr(impl->signature));
		return 0;
	}
	SModSign *sign = (SModSign*) ftptr(&ftast, s->stmt);
	assert(sign->kind == SMODSIGN && "MODSIGN Table must contain only mod signatures");

	// Verify the generics.
	if (!impl_verify_generics(impl, sign)) {
		ERR("Error when analyzing <%s>.", identstr(impl->ident));
		return 0;
	}

	// Verify the modules.
	if(!impl_verify_module(impl, sign)) {
		ERR("Error when analyzing <%s>.", identstr(impl->ident));
		return 0;
	}

	// Verify the stmts.
	if(!impl_verify_stmts(impl, sign)) {
		ERR("Error when analyzing <%s>.", identstr(impl->ident));
		return 0;
	}
	
	return 1;
}

Bool
analyzemodskeleton(SModSkel *skel)
{
	// Verify the existance of the signature.
	Symbol *s = searchtopdcl(modsym + MODSIGN, skel->signature);
	if (s == NULL) {
		ERR("The skeleton <%s> use an undefined signature <%s>.", identstr(skel->ident), identstr(skel->signature));
		return 0;
	}

	SModSign *sign = (SModSign*) ftptr(&ftast, s->stmt);
	assert(sign->kind == SMODSIGN);

	// verify stmts

	TODO("analyzemodskeleton");
	return 0;
}

Bool
analyzemodefine(SModDef *def)
{
	(void) def;
	TODO("analyzemodefine");
	return 0;
}
