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

#define islangtype(_type) ((typeoffset <= _type) && (_type <= typeend))

extern int typeoffset;
extern int typeend;
extern int ident2langtype[NLANGTYPE * 3];
extern int langtype2ident[NLANGTYPE];
extern Symbols funsym;
extern Symbols identsym;
extern Symbols typesym;

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

typedef struct SymHierarchy SymHierarchy;
struct SymHierarchy {
	Symbols *top;
	SymHierarchy *others;
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

Bool
computeconstype(intptr expr, intptr *type, int *ptrlvl)
{
	UnknownExpr *unknown = (UnknownExpr*) ftptr(&ftast, expr);
	switch (*unknown) {
	case ECSTI:
		*type = langtype2ident[I32];
		*ptrlvl = 0;
		INFO("the type of ECSTI: <%s>", (char*) ftptr(&ftident, *type));
		return 1;
		break;
	case ECSTF:
		*type = langtype2ident[F32];
		*ptrlvl = 0;
		INFO("the type of ECSTF: <%s>", (char*) ftptr(&ftident, *type));
		return 1;
		break;
	case ECSTS:
		*type = langtype2ident[U8];
		*ptrlvl = 1;
		INFO("the type of ECSTS: <%s>", (char*) ftptr(&ftident, *type));
		return 1;
		break;
	case EBINOP: {
		EBinop *binop = (EBinop*) unknown;
		intptr type1;
		int ptrlvl1;

		intptr type2;
		int ptrlvl2;

		if (!(computeconstype(binop->left, &type1, &ptrlvl1)
		                && computeconstype(binop->right, &type2, &ptrlvl2))) {
			ERR("Error when computing the type of a const declaration binop.");
			return 0;
		}

		if (!promotion(type1, ptrlvl1, type2, ptrlvl2, &binop->type, &binop->ptrlvl)) {
			ERR("Error when promoting the type of a const declaration binop.");
			return 0;
		}

		*type = binop->type;
		*ptrlvl = binop->ptrlvl;
		break;
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
		case UOP_BXOR: {
			if (!computeconstype(unop->expr, &unop->type, &unop->ptrlvl)) {
				ERR("Error when computing an unop <%s>.", uopstrs[unop->op]);
				return 0;
			}

			if (*ptrlvl > 0) {
				ERR("Error the unop <%s> can't be used on a pointer.", uopstrs[unop->op]);
				return 0;
			}

			*type = unop->type;
			*ptrlvl = unop->ptrlvl;
			break;
		}
		case UOP_AT:
			if (!computeconstype(unop->expr, &unop->type, &unop->ptrlvl)) {
				ERR("Error when computing an unop <%s>.", uopstrs[unop->op]);
				return 0;
			}
			*type = unop->type;
			*ptrlvl = (++unop->ptrlvl);

			return 1;
		case UOP_SIZEOF:
			if (!computeconstype(unop->expr, &unop->type, &unop->ptrlvl)) {
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

		return 0;
		ERR("Unreachable unop op <%d>.", unop->op);
		break;
	}

	case EPAREN: {
		EParen *paren = (EParen*) unknown;
		if (!computeconstype(paren->expr, &paren->type, &paren->ptrlvl)) {
			ERR("Error when computing a paren type.");
			return 0;
		}

		*type = paren->type;
		*ptrlvl = paren->ptrlvl;
		return 1;
		break;
	}
	case ESTRUCT: {
		EStruct* st = (EStruct*) unknown;
		*type = st->ident;
		TODO("Handle Struct expression.");
		return 0;
		break;
	}
	default:
		ERR("Unexpected statement <%s> in a toplevel declaration.", exprstrs[*unknown]);
		return 0;
	}
	return 0;
}

Bool
insertglobalconst(Symbols *syms, SDecl *decl, intptr stmt)
{
	Symbol *sym = (Symbol*) ftptr(&ftsym, syms->array);
	for (int i = 0; i < syms->nsym; i++) {
		if (decl->ident == sym[i].ident) {
			ERR("Symbole <%s> is already declared.", (char*) ftptr(&ftident, decl->ident));
			return 0;
		}
	}

	sym[syms->nsym] = (Symbol) {
		.ident = decl->ident, .stmt = stmt
	};
	syms->nsym++;

	if (decl->type != -1) {
		TODO("Take into account the typing given by the user.");
	}

	if (computeconstype(decl->expr, &decl->type, &decl->ptrlvl)) {
		ERR("Error when computing a toplevel declaration");
		return 0;
	}

	sym[syms->nsym] = (Symbol) {
		.ident = decl->ident, .stmt = stmt
	};
	syms->nsym++;

	return 1;
}

Bool
searchtopdcl(Symbols *syms, intptr ident)
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
istypedefined(intptr type)
{
	return ((typeoffset <= type) && (type <= typeend))
	       || searchtopdcl(&typesym, type);
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

// Bool analyzefunfor();
// Bool analyzefunif();
// Bool analyzefunstmt(SFun *fun, UnknownStmt *stmt, SymHierarchy *sym)
// {
// 	TODO("analyzefunstmt");
// 	int nstmt = -1;
//
// 	switch (*stmt) {
// 	case SSEQ: {
// 		SSeq* seq = (SSeq*) stmt;
// 		UnknownStmt* stmt1 = ftptr(&ftast, seq->stmt);
// 		UnknownStmt* nxt = ftptr(&ftast, seq->nxt);
//
// 		switch (*stmt1) {
// 		case SFOR:
// 		case SIF:
// 			nstmt = sym->nstmt;
// 			analyzefunstmt(nxt);
// 			break;
// 		default:
// 			return analyzefunstmt(stmt1) && analyzefunstmt(nxt);
// 		}
// 	}
// 	case SDECL:
// 	case SIF:
// 	case SASSIGN:
// 	case SRETURN:
// 	case SFOR:
// 	case SCALL:
// 	default:
// 		ERR("Unexpected <%s> Statement in a function.", stmtstrs[*ptr]);
// 		return 0;
// 	}
// 	return 0;
// }
//
// Bool
// insertdcl(Symbols *syms, )
// {
// 	Symbol *sym = (Symbol*) ftptr(&ftsym, syms->array);
// 	for (int i = 0; i < syms->nsym; i++) {
// 		if (ident == sym[i].ident) {
// 			return 0;
// 		}
// 	}
// 	sym[syms->nsym] = (Symbol) {
// 		.ident = ident, .stmt = stmt
// 	};
// 	syms->nsym++;
//
// 	return 1;
// }

Bool
analyzefun(SFun *fun)
{
	Symbols localsym = {0};
	localsym.array = ftalloc(&ftsym, sizeof(Symbol));

	// Verify the params
	SMember *member = (SMember*) ftptr(&ftast, fun->params);
	for (int i = 0; i < fun->nparam; i++) {
		if (!istypedefined(member->type)) {
			ERR("Type <%s> used in the function <%s> but never declared", (char*) ftptr(&ftident, member->type), (char*) ftptr(&ftident, fun->ident));
			return 0;
		}
		member++;
	}

	if (!istypedefined(fun->type)) {
		ERR("Type <%s> used in the function <%s> but never declared", (char*) ftptr(&ftident, fun->type), (char*) ftptr(&ftident, fun->ident));
		return 0;
	}


	return 1;
}

