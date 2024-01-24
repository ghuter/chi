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

#define TODO(message) \
    do { \
        fprintf(stderr, "TODO(%s): %s\n", TOSTRING(__LINE__), message); \
    } while (0)

#define ERR(...) fprintf(stderr, "ERR(%d): ", __LINE__); fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#define INFO(...) fprintf(stderr, "INFO(%d): ", __LINE__); fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")

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
extern SymInfo *syminfo;

char* langtypestr[NLANGTYPE] = {
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
	[U8] = {
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
		fprintf(stderr, "%s : ", identstr(syminfo[i].ident));
		int ptrlvl = syminfo[i].ptrlvl;
		while (ptrlvl-- > 0) {
			fprintf(stderr, "^");
		}

		char *type = TYPESTR(syminfo[i].type);
		fprintf(stderr, "%s, ", type);
	}
	fprintf(stderr, "\n");
}

int
searchtopdcl(Symbols *syms, intptr ident)
{
	Symbol *sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		if (ident == sym[i].ident) {
			return i;
		}
	}

	return -1;
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

	*rtype = langtype2ident[promotiontable[ltype1][ltype2]];
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
	case EBINOP: {
		EBinop *binop = (EBinop*) unknown;
		intptr type1;
		int ptrlvl1;

		intptr type2;
		int ptrlvl2;

		if (!computeconstype(binop->left, &type1, &ptrlvl1, typeinfo)) {
			ERR("Error when computing the type of a const declaration binop.");
			return 0;
		}

		if (!computeconstype(binop->right, &type2, &ptrlvl2, typeinfo)) {
			ERR("Error when computing the type of a const declaration binop.");
			return 0;
		}

		if (!promotion(type1, ptrlvl1, type2, ptrlvl2, &binop->type, &binop->ptrlvl)) {
			ERR("Error when promoting the type of a const declaration binop.");
			return 0;
		}

		*type = binop->type;
		*ptrlvl = binop->ptrlvl;

		return 1;
	}
	case EUNOP: {
		EUnop *unop = (EUnop*) unknown;
		switch (unop->op) {
		case UOP_SUB:
		//fallthrough
		case UOP_LNOT:
		//fallthrough
		case UOP_BNOT:
			//fallthrough
		{
			if (!computeconstype(unop->expr, &unop->type, &unop->ptrlvl, typeinfo)) {
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
			ERR("Unexpected op <%s> at the toplevel.", uopstrs[unop->op]);
			return 0;
		case UOP_AT:
			if (!computeconstype(unop->expr, &unop->type, &unop->ptrlvl, typeinfo)) {
				ERR("Error when computing an unop <%s>.", uopstrs[unop->op]);
				return 0;
			}
			*type = unop->type;
			*ptrlvl = (++unop->ptrlvl);

			return 1;
		case UOP_SIZEOF:
			if (!computeconstype(unop->expr, &unop->type, &unop->ptrlvl, typeinfo)) {
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

		ERR("Unreachable unop op <%d>.", unop->op);
		return 0;
	}

	case EPAREN: {
		EParen *paren = (EParen*) unknown;
		if (!computeconstype(paren->expr, &paren->type, &paren->ptrlvl, typeinfo)) {
			ERR("Error when computing a paren type.");
			return 0;
		}

		*type = paren->type;
		*ptrlvl = paren->ptrlvl;
		return 1;
	}
	case ESTRUCT: {
		EStruct* st = (EStruct*) unknown;
		*type = st->ident;

		int isym = searchtopdcl(&typesym, st->ident);
		if (isym < 0) {
			ERR("Error the struct <%s> is used by never declared.", identstr(st->ident));
			return 0;
		}

		Symbol *sym = (Symbol*) ftptr(&ftsym, typesym.array);
		SStruct *sdecl = (SStruct*) ftptr(&ftast, sym[isym].stmt);

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
	if (!computeconstype(decl->expr, &decl->type, &decl->ptrlvl, decl->type)) {
		ERR("Error when computing a toplevel declaration");
		return 0;
	}

	syminfo[nelem].type = decl->type;
	syminfo[nelem].ptrlvl = decl->ptrlvl;
	syminfo[nelem].ident = decl->ident;

	return 1;
}

Bool
analyzefunexpr(intptr expr, intptr *type, int *ptrlvl, intptr typeinfo, int nsym)
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
	case EBINOP: {
		EBinop *binop = (EBinop*) unknown;
		intptr type1 = -1;
		int ptrlvl1 = -1;

		intptr type2 = -1;
		int ptrlvl2 = -1;

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

		*type = binop->type;
		*ptrlvl = binop->ptrlvl;
		return 1;
	}
	case EUNOP: {
		EUnop *unop = (EUnop*) unknown;
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

		ERR("Unreachable unop op <%d>.", unop->op);
		return 0;
	}
	case EPAREN: {
		EParen *paren = (EParen*) unknown;
		if (!analyzefunexpr(paren->expr, &paren->type, &paren->ptrlvl, typeinfo, nsym)) {
			ERR("Error when computing a paren type.");
			return 0;
		}

		*type = paren->type;
		*ptrlvl = paren->ptrlvl;
		return 1;
	}
	case ESTRUCT: {
		EStruct* st = (EStruct*) unknown;
		*type = st->ident;

		int isym = searchtopdcl(&typesym, st->ident);
		if (isym < 0) {
			ERR("Error the struct <%s> is used by never declared.", identstr(st->ident));
			return 0;
		}

		Symbol *sym = (Symbol*) ftptr(&ftsym, typesym.array);
		SStruct *sdecl = (SStruct*) ftptr(&ftast, sym[isym].stmt);

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
	case ECALL: {
		ECall *call = (ECall*) unknown;
		Mem* expr = (Mem*) ftptr(&ftast, call->expr);

		if (expr->kind != EMEM) {
			TODO("Function pointer isn't available yet.");
			return 0;
		}

		int idx = searchtopdcl(&funsym, expr->addr);
		if (idx == -1) {
			ERR("The function <%s> is called but never declared.", identstr(expr->addr));
			return 0;
		}

		Symbol *sym = (Symbol*) ftptr(&ftsym, funsym.array);
		SFun *stmt = (SFun*) ftptr(&ftast, sym[idx].stmt);
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
		TODO("OK for ECALL");
		return 1;
	}
	case EMEM: {
		Mem *mem = (Mem*) unknown;
		SymInfo *sym = searchsyminfo(nsym, mem->addr);

		if (sym == NULL) {
			ERR("Use of the identifier <%s> which has never been declared.", identstr(mem->addr));
		}

		*type = sym->type;
		*ptrlvl = sym->ptrlvl;
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
analyzefunstmt(const SFun *fun, int *nsym, int block, intptr stmt)
{
	UnknownStmt *unknown = (UnknownStmt*) ftptr(&ftast, stmt);
	switch (*unknown) {
	case SSEQ: {
		TODO("SSEQ");

		SSeq* seq = (SSeq*) unknown;

		ERR("seq->stmt: %d", seq->stmt);
		if (!analyzefunstmt(fun, nsym, block, seq->stmt)) {
			ERR("Error when analyzing a seq in a function.");
			return 0;
		}

		ERR("seq->next: %d", seq->nxt);
		if (!analyzefunstmt(fun, nsym, block, seq->nxt)) {
			ERR("Error when analyzing a seq in a function.");
			return 0;
		}

		return 1;
	}
	case SDECL: {
		TODO("SDECL");

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
		TODO("IF");
		intptr type;
		int ptrlvl;

		SIf* _if = (SIf*)unknown;
		if (!analyzefunexpr(_if->cond, &type, &ptrlvl, -1, *nsym)) {
			ERR("Error when analyzing the condition of a if.");
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
		TODO("SRETURN");
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
		TODO("OK ret");

		return 1;
	}
	case SFOR: {
		TODO("SFOR");

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

		newblock = newnsym;
		if (!analyzefunstmt(fun, &newnsym, newblock, f->forstmt)) {
			ERR("Error when analyzing the body stmt of a for.");
			return 0;
		}
		return 1;
	}
	case SCALL: {
		TODO("SCALL");

		SCall *call = (SCall*) unknown;

		int idx = searchtopdcl(&funsym, call->ident);
		if (idx == -1) {
			ERR("The function <%s> is called but never declared.", identstr(call->ident));
			return 0;
		}

		Symbol *sym = (Symbol*) ftptr(&ftsym, funsym.array);
		SFun *stmt = (SFun*) ftptr(&ftast, sym[idx].stmt);
		assert(stmt->kind == SFUN);

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
		TODO("SASSIGN");
		SAssign *a = (SAssign*) unknown;

		SymInfo *sym = searchsyminfo(*nsym, a->ident);
		if (sym == NULL) {
			ERR("Use of the identifier <%s> which has never been declared.", identstr(a->ident));
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
	default:
		ERR("Unexpected <%s> Statement in a function.", stmtstrs[*unknown]);
		return 0;
	}
	return 0;
}


Bool
analyzefun(SFun *fun, int nsym)
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

	int newnsym = nsym;
	if (!analyzefunstmt(fun, &newnsym, newnsym, fun->stmt)) {
		ERR("Error when analyzing the fun statements");
		return 0;
	}

	return 1;
}

