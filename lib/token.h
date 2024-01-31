#ifndef TOKEN_H
#define TOKEN_H

typedef enum {
	UNDEFINED = 0,

	EOI, // End-Of-Input
	INT,
	FLOAT,
	LITERAL,
	IDENTIFIER,
	NEWLINE,
	SEMICOLON,
	SPC,
	COLON,
	COMMA,
	DOT,
	LPAREN,
	RPAREN,
	LBRACKETS,
	RBRACKETS,
	LBRACES,
	RBRACES,

// OPERATORS
	MUL,
	DIV,
	MOD,
	ADD,
	SUB,
	LSHIFT,
	RSHIFT,
	LT,
	GT,
	LE,
	GE,
	EQUAL,
	NEQUAL,
	BAND,
	BXOR,
	BOR,
	LAND,
	LOR,
	BNOT,
	LNOT,
	ASSIGN,
	BAND_ASSIGN,
	BOR_ASSIGN,
	BXOR_ASSIGN,
	BNOT_ASSIGN,
	LAND_ASSIGN,
	LOR_ASSIGN,
	LSHIFT_ASSIGN,
	RSHIFT_ASSIGN,
	ADD_ASSIGN,
	SUB_ASSIGN,
	MUL_ASSIGN,
	DIV_ASSIGN,
	MOD_ASSIGN,
	AT,

	NTOK, // must be the last of "regular tokens"

	FOR,
	IF,
	ELSE,
	STRUCT,
	SIZEOF,
	RETURN,
	IN,
	IMPORT,
	ENUM,
	FUN,
	TRUE,
	FALSE,
	IMPL,
	SIGNATURE,
	SKELETON,
	DEFINE,
	NKEYWORDS, // must be the last "keyword"
} ETok;

typedef struct {
	ETok type;
} Tok;

extern char *keywords[];
extern char *tokenstrs[];

#endif /* TOKEN_H */
