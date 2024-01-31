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
        fprintf(stderr, "TODO(%s): %s\n", TOSTRING(__LINE__), message); \
    } while (0)

#define ERR(...) fprintf(stderr, "ERR(%d): ", __LINE__); fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")

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
extern SymInfo *syminfo;

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

Bool computeconstype(intptr expr, intptr *type, int *ptrlvl, intptr typeinfo);
Bool analyzefunexpr(intptr expr, intptr *type, int *ptrlvl, intptr typeinfo, int nsym);

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

Symbol*
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

Bool
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

Bool
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

SymInfo*
searchsyminfo(int nsym, intptr ident)
{
	for (int i = nsym - 1; i >= 0; i--) {
		if (syminfo[i].ident == ident) {
			return syminfo + i;
		}
	}

	return NULL;
}


SMember*
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


Bool
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

Bool
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

Bool
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

Bool
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

Bool
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

Bool
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

Bool
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

Bool
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

Bool
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

Bool
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

