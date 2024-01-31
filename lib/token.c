#include "token.h"

char *keywords[NKEYWORDS] = {
	[FOR]       = "for",
	[IF]        = "if",
	[ELSE]      = "else",
	[STRUCT]    = "struct",
	[SIZEOF]    = "sizeof",
	[RETURN]    = "return",
	[IN]        = "in",
	[IMPORT]    = "import",
	[ENUM]      = "enum",
	[FUN]       = "fun",
	[TRUE]      = "true",
	[FALSE]     = "false",
	[IMPL]      = "impl",
	[SIGNATURE] = "signature",
	[SKELETON]  = "skeleton",
	[DEFINE]    = "define",
};

char *tokenstrs[NKEYWORDS] = {
	[EOI]           = "EOI",
	[FOR]           = "FOR",
	[IF]            = "IF",
	[ELSE]          = "ELSE",
	[STRUCT]        = "STRUCT",
	[SIZEOF]        = "SIZEOF",
	[RETURN]        = "RETURN",
	[IN]            = "IN",
	[IMPORT]        = "IMPORT",
	[ENUM]          = "ENUM",
	[FUN]           = "FUN",
	[TRUE]          = "TRUE",
	[FALSE]         = "FALSE",
	[IMPL]          = "IMPL",
	[SIGNATURE]     = "SIGNATURE",
	[SKELETON]      = "SKELETON",
	[DEFINE]        = "DEFINE",

	[INT]           = "INT",
	[FLOAT]         = "FLOAT",
	[LITERAL]       = "LITERAL",
	[IDENTIFIER]    = "IDENTIFIER",
	[NEWLINE]       = "NEWLINE",
	[SEMICOLON]     = "SEMICOLON",
	[SPC]           = "SPC",
	[COLON]         = "COLON",
	[COMMA]         = "COMMA",
	[DOT]           = "DOT",
	[LPAREN]        = "LPAREN",
	[RPAREN]        = "RPAREN",
	[LBRACKETS]     = "LBRACKETS",
	[RBRACKETS]     = "RBRACKETS",
	[LBRACES]       = "LBRACES",
	[RBRACES]       = "RBRACES",

	[MUL]           = "MUL",
	[DIV]           = "DIV",
	[MOD]           = "MOD",
	[ADD]           = "ADD",
	[SUB]           = "SUB",
	[LSHIFT]        = "LSHIFT",
	[RSHIFT]        = "RSHIFT",
	[LT]            = "LT",
	[GT]            = "GT",
	[LE]            = "LE",
	[GE]            = "GE",
	[EQUAL]         = "EQUAL",
	[NEQUAL]        = "NEQUAL",
	[BAND]          = "BAND",
	[BXOR]          = "BXOR",
	[BOR]           = "BOR",
	[LAND]          = "LAND",
	[LOR]           = "LOR",
	[BNOT]          = "BNOT",
	[LNOT]          = "LNOT",

	[ASSIGN]        = "ASSIGN",
	[BAND_ASSIGN]   = "BAND_ASSIGN",
	[BOR_ASSIGN]    = "BOR_ASSIGN",
	[BXOR_ASSIGN]   = "BXOR_ASSIGN",
	[BNOT_ASSIGN]   = "BNOT_ASSIGN",
	[LAND_ASSIGN]   = "LAND_ASSIGN",
	[LOR_ASSIGN]    = "LOR_ASSIGN",
	[LSHIFT_ASSIGN] = "LSHIFT_ASSIGN",
	[RSHIFT_ASSIGN] = "RSHIFT_ASSIGN",
	[ADD_ASSIGN]    = "ADD_ASSIGN",
	[SUB_ASSIGN]    = "SUB_ASSIGN",
	[MUL_ASSIGN]    = "MUL_ASSIGN",
	[DIV_ASSIGN]    = "DIV_ASSIGN",
	[MOD_ASSIGN]    = "MOD_ASSIGN",

	[AT]            = "AT",

	[UNDEFINED]     = "UNDEFINED",
};

