#include "lib/token.h"

#define TODO(_str) printf("TODO: " _str "\n")

static int line = 0;
static int eoi = 0;
FatArena fttok = {0};

#define burnseparators(_t, _i)                 \
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

#define ismonadicop(_op) (_op == SUB || _op == LNOT || _op == BNOT || _op == BXOR || _op == AT || _op == SIZEOF)

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
			burnseparators(t, i);
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

int
parse_expression(ETok *t)
{
	TODO("Expression");
	int i = 0;

	if (t[i] == IDENTIFIER)
		TODO("IDENTIFIER");
	if (t[i] == FLOAT)
		TODO("FLOAT");
	if (t[i] == INT)
		TODO("INT");
	if (t[i] == LITERAL)
		TODO("LITERAL");
	if (ismonadicop(t[i]))
		TODO("MONADICOP");

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
		parse_expression(t + i);
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

	fprintf(stderr, "INFO: line %d, declaration <%s> : <%s>", line, (char*) ftptr(&ftident, ident), ((type > 0) ? (char*) ftptr(&ftident, type) : "GUESS" ));
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

