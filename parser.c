#include <assert.h>
#include "lib/token.h"

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

#define TODO(message) \
    do { \
        printf("TODO(%s): %s\n", TOSTRING(__LINE__), message); \
    } while (0)

static int line = 0;
static int eoi = 0;
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

#define ISEOE(_t, _eoe, _res)                  \
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

#define ISMONADICOP(_op) (_op == SUB || _op == LNOT || _op == BNOT || _op == BXOR || _op == AT || _op == SIZEOF)
#define ISBINOP(_op) (_op >= MUL && _op <= LNOT)
#define TOKTOOP(_t) ((_t) - MUL)


int
parse_toplevel_fun(ETok *t)
{
	(void)t;
	TODO("Toplevel fun");
	return 0;
}

int
parse_toplevel_interface(ETok *t)
{
	(void)t;
	TODO("Toplevel interface");
	return 0;
}

int
parse_toplevel_struct(ETok *t)
{
	TODO("Toplevel struct");
	fprintf(stderr, "INFO: declaration of a struct:\n");
	int i = 0;

	if (t[i] != LBRACES) {
		fprintf(stderr, "ERR: after 'struct' a '{' is expected\n");
		return 0;
	}
	i++;

	while (t[i] == NEWLINE) {
		i++;
		line++;
	}

	while (1) {
		int ident = -1;
		int type = -1;

		if (t[i] == IDENTIFIER) {
			i++;
			ident = t[i];
			i++;
		} else goto err;

		if (t[i] == COLON) {
			i++;
		}

		if (t[i] == IDENTIFIER) {
			i++;
			type = t[i];
			i++;
		} else goto err;

		fprintf(stderr, "INFO: member <%s> of type <%s>\n", (char*)ftptr(&ftident, ident), (char*)ftptr(&ftident, type));

		if (t[i] == NEWLINE || t[i] == SEMICOLON) {
			BURNSEPARATORS(t, i);
		} else {
			fprintf(stderr, "ERR: line %d, expects a separator <NEWLINE> | <SEMICOLON>\n", line);
			return 0;
		}

		if (t[i] == RBRACES) {
			i++;
			break;
		}
	}

	fprintf(stderr, "INFO: end of the struct:\n");
	return i;

	err:
	fprintf(stderr, "ERR: line %d, unexepcted Token <%s>, expects struct { <IDENTIFIER> ':' <IDENTIFIER> }\n", line, tokenstrs[t[i]]);
	return 0;
}

int
parse_toplevel_enum(ETok *t)
{
	(void)t;
	TODO("Toplevel enum");
	return 0;
}

typedef int intptr;
typedef enum {
	ENONE,
	ECSTI,
	ECSTF,
	ECSTS,
	EMEM,
	EBINOP,
	EUNOP,
	ECALL,
	ERETURN,
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
	[ERETURN] = "ERETURN",
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
	int nparam;
	intptr params;
} Call;

typedef struct {
	EExpr type;
	intptr expr;
} Return;

void
printexpr(FILE* fd, intptr expr)
{
	UnknownExpr* ptr = (UnknownExpr*) ftptr(&ftast, expr);
	switch(*ptr) {
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
	case ECALL:
	case ERETURN:
	default:
		assert(1 || "Unreachable enum");
	}
}

int
parse_call(ETok *t)
{
	const ETok eoe[3] = {COMMA, RPAREN, UNDEFINED};
	int i = 0;

	if (t[i] == LPAREN) {
	TODO("Function call");
		fprintf(stderr, "INFO: line %d, <IDENTIFIER> is a fun call\n", line);
		return 1;
	}

	return 0;
}

int
parse_expression(ETok *t, const ETok *eoe, intptr *d)
{
	TODO("Expression");
	fprintf(stderr, "INFO: t == %p\n", (void*)t);

	int i = 0;
	int addr = -1;
	int add = 0;
	EExpr type = ENONE;

	switch (t[i]) {
		case IDENTIFIER:
			type = EMEM;
			i++;
			addr = t[i];
			i++;
			if (parse_call(t + i) != 0) {
				type = ECALL;
			}
			break;

		// Cst
		case LITERAL: 
			add++;
			//fallthrough
		case FLOAT:
			add++;
			// fallthrough
		case INT: 
			type = ECSTI + add;
			i++;
			addr = t[i];
			i++;
			break;

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
			puts("UNOP");
			intptr uaddr = ftalloc(&ftast, sizeof(Unop));
			// Save unop
			Unop *unop = (Unop*) ftptr(&ftast, uaddr);
			unop->type = EUNOP;
			unop->op = add;

			// Parse the following expression
			intptr expr = 0;
			i++;
			i += parse_expression(t + i, eoe, &expr);
			unop->expr = expr;

			// Save destination
			*d = uaddr;
			return i;
			break;
				  }
		default:
			fprintf(stderr, "ERR: line %d, unexpected token <%s>\n", line, tokenstrs[t[i]]);
			return 0;
	}

	// Here, we have parsed at least one expression.
	// We need to check if it's the end.
	int res = 0;

	ISEOE(t[i], eoe, res);
	if (res != 0) {
		fprintf(stderr, "INFO: line %d, expression <%s>\n", line, exprstrs[type]);
		// Alloc Identifier / Const
		intptr maddr = ftalloc(&ftast, sizeof(Mem));
		Mem* mem = (Mem*) ftptr(&ftast, maddr);

		// Set Memory
		mem->type = type;
		mem->addr = addr;

		// Set destination
		*d = maddr;
		return i;
	}

	if (ISBINOP(t[i])) {
		// Alloc a binop
		intptr baddr = ftalloc(&ftast, sizeof(Binop));
		Binop *binop = (Binop*) ftptr(&ftast, baddr);
		binop->type = EBINOP;
		binop->op = TOKTOOP(t[i]);

		// Save in the return variable
		*d = baddr;
		
		// Save the parsed expression
		intptr laddr = ftalloc(&ftast, sizeof(Csti));
		Csti *ptr = (Csti*) ftptr(&ftast, laddr);
		ptr->type = type;
		ptr->addr = addr;
		binop->left = laddr;

		// Parse the following expression
		i++;
		intptr raddr = -1;
		fprintf(stderr, "INFO: line %d, expression <%s> followed by a binop\n", line, exprstrs[binop->type]);
		fprintf(stderr, "---------------BINOP(%s, %s\n", opstrs[binop->op], exprstrs[binop->type]);
		i += parse_expression(t + i, eoe, &raddr);

		// Save the following expression
		binop->right = raddr;

		fprintf(stderr,"---------------)\n");
		return i;
	}

	fprintf(stderr, "i: %d\n", i);
	fprintf(stderr, "ERR: line %d, the expression isn't followed by a binop, nor an end of expression, it's a <%s>\n", line, tokenstrs[t[i]]);
	return 0;
}

int
parse_toplevel_decl(ETok *t)
{
	int i = 2;
	int ident = t[1];
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
	}
	else if (t[i] == ASSIGN) {
		cst = 0;
	} else {
		fprintf(stderr, "ERR: line %d, declaration: <IDENTIFIER> : <IDENTIFIER> : EXPR", line);
		return 0;
	}
	i++;

	if (t[i] == STRUCT) {
		i++;
		parse_toplevel_struct(t + i);
	} else if (t[i] == ENUM) {
		i++;
		parse_toplevel_enum(t + i);
	} else if (t[i] == FUN) {
		i++;
		parse_toplevel_fun(t + i);
	} else if (t[i] == INTERFACE) {
		i++;
		parse_toplevel_interface(t + i);
	} else /* Expression */ {
		const ETok eoe[3] = {SEMICOLON, NEWLINE, UNDEFINED};
		intptr expr = 0;
		parse_expression(t + i, eoe, &expr);
		printexpr(stderr, expr);
		fprintf(stderr, "\n");
	}

	// parse expression
	while (1) {
		if (t[i] == NEWLINE) {
			line += 1;
			break;
		} else if (t[i] == SEMICOLON) {
			break;
		} else if (t[i] == EOI) {
			eoi = 1;
			break;
		}
		i++;
	}

	fprintf(stderr, "INFO: line %d, declaration <%s> : <%s>\n", line, (char*) ftptr(&ftident, ident), ((type > 0) ? (char*) ftptr(&ftident, type) : "GUESS" ));
	return i;
}

int
parse_toplevel_import(ETok *t)
{
	(void)t;
	TODO("IMPORT");
	return 0;
}

int
parse_toplevel(ETok *t)
{
	int i = 0;

	if (t[i] == IDENTIFIER)
		parse_toplevel_decl(t);
	else if (t[i] == IMPORT)
		parse_toplevel_import(t);

	return 0;
}

