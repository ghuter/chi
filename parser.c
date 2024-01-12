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


int
parse_toplevel_fun(ETok *t)
{
	TODO("Toplevel fun");
	return 0;
}

int
parse_toplevel_interface(ETok *t)
{
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
	TODO("Toplevel enum");
	return 0;
}

typedef enum {
	ENONE,
	ECST,
	EMEM,
	EBINOP,
	EUNOP,
	ECALL,
	ERETURN,
	NEXPR,
} Expr;

char *exprstr[NEXPR] = {
	[ENONE]   = "ENONE",
	[ECST]    = "ECST",
	[EMEM]    = "EMEM",
	[EBINOP]  = "EBINOP",
	[EUNOP]   = "EUNOP",
	[ECALL]   = "ECALL",
	[ERETURN] = "ERETURN",
};

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
parse_expression(ETok *t, const ETok *eoe)
{
	TODO("Expression");
	fprintf(stderr, "INFO: t == %p\n", (void*)t);

	int i = 0;
	int ident = -1;
	int op = -1;
	Expr expr = ENONE;

	switch (t[i]) {
		case IDENTIFIER:
			i++;
			ident = t[i];
			expr = EMEM;
			i++;
			if (parse_call(t + i) != 0) {
				expr = ECALL;
			}
			break;

		// Cst
		case FLOAT:
		case INT:
		case LITERAL:
			i++;
			i++;
			expr = ECST;
			break;

		// Unop
		case SUB:    // fallthrough
		case LNOT:   // fallthrough
		case BNOT:   // fallthrough
		case BXOR:   // fallthrough
		case AT:     // fallthrough
		case SIZEOF: 
			i++;
			expr = EUNOP;
			i += parse_expression(t + i, eoe);
			break;
		default:
			fprintf(stderr, "ERR: line %d, unexpected token <%s>\n", line, tokenstrs[t[i]]);
			return 0;
	}

	// Here, we have parsed at least one expression.
	// We need to check if it's the end.
	int res = 0;

	ISEOE(t[i], eoe, res);
	if (res != 0) {
		fprintf(stderr, "INFO: line %d, expression <%s>\n", line, exprstr[expr]);
		return i;
	}

	if (ISBINOP(t[i])) {
		op = t[i];
		i++;
		fprintf(stderr, "INFO: line %d, expression <%s> followed by a binop\n", line, exprstr[expr]);
		fprintf(stderr, "---------------BINOP(%s, %s\n", tokenstrs[op], exprstr[expr]);
		parse_expression(t + i, eoe);
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
		fprintf(stderr, "ERR: line %d, declaration: <IDENTIFIER> : <IDENTIFIER> : EXPR", line);
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
		parse_expression(t + i, eoe);
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

