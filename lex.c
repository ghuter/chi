#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFSZ 512

int hold = 0;
int holdc = 0;
int fd = 0;

typedef enum {
	UNDEFINED = 0,

	EOI, // End-Of-Input
	IDENTIFIER,
	NEWLINE,
	SEMICOLON,
	SPC,
	NTOK, // must be the last of "regular tokens"

	FOR,
	NKEYWORDS, // must be the last "keyword"
} ETok;

typedef struct {
	ETok type;
} Tok;

char *keywords[] = {
	[FOR] = "for",
};

char *tokenstrs[] = {
	[EOI]        = "EOI",
	[FOR]        = "FOR",
	[IDENTIFIER] = "IDENTIFIER",
	[NEWLINE]    = "NEWLINE",
	[SEMICOLON]  = "SEMICOLON",
	[SPC]        = "SPC",
	[UNDEFINED]  = "UNDEFINED",
};

void
forward(void)
{
	holdc = 0;
}

char
peekc(void)
{
	static char buf[BUFSZ];
	static char c;
	static int hd = -1;
	ssize_t sz;

	/* return the same char until forward() is not called */
	if (holdc) {
		if (c == 0) {
			/* fprintf(stderr, "peekc(): returning EOF\n"); */
			return EOF;
		}
		/* fprintf(stderr, "peekc(): hold: returning '%c'\n", c); */
		return c;
	}
	holdc = 1;

	if (hd < 0 || hd == BUFSZ) {
		hd = 0;
		sz = read(fd, buf, sizeof(buf));
		if (sz < 0) {
			perror("read");
			exit(1);
		}
		if (sz == 0) {
			/* fprintf(stderr, "peekc(): returning EOF\n"); */
			return EOF;
		}
	}
	c = buf[hd];

	if (c == 0) {
		/* fprintf(stderr, "peekc(): returning EOF\n"); */
		return EOF;
	}

	hd++;
	/* fprintf(stderr, "peekc(): returning '%c'\n", c); */
	return c;
}

static int
skipwhite()
{
	char c = peekc();
	if (c != ' ' && c != '\t') {
		return 0;
	}
	do {
		forward();
		c = peekc();
	} while (c == ' ' || c == '\t');
	return 1;
}

void
printtok(FILE *o, Tok t)
{
	fprintf(o, "%s(%d)\n", tokenstrs[t.type], t.type);
}

Tok
peek()
{
	static Tok tok;
	char buf[64];
	char c;

	if (hold)
		return tok;
	hold = 1;

	memset(&tok, 0, sizeof(tok));

	c = peekc();
	forward();

	switch (c) {
	case EOF:
		tok.type = EOI;
		break;
	case ' ': // fallthrough
	case '\t':
		tok.type = SPC;
		do {
			forward();
			c = peekc();
		} while (c == ' ' || c == '\t');
		break;
	case ';':
		tok.type = SEMICOLON;
		break;
	case '\n':
		tok.type = NEWLINE;
		break;
	default: {
			int len = 0;
			while (9) {
				buf[len++] = c;
				c = peekc();

				if (c == EOF)
					break;
				// word character
				if (strchr(" '\t\n#;&|^$`{}()<>=", c))
					break;

				forward();
			}
			buf[len++] = 0;

			int i;
			for (i = NTOK + 1; i < NKEYWORDS; i++) {
				/* fprintf(stderr, "(i == %d) comparing '%s' to '%s'..\n", i, buf, keywords[i]); */
				if (strcmp(buf, keywords[i]) == 0) {
					tok.type = i;
					break;
				}
			}
			if (i == NKEYWORDS) {
				tok.type = IDENTIFIER;
				// TODO(ghuter): save the buffer somewhere, link it to the token
			}
		} break;
	}

	return tok;
}

void
tok_forward(void)
{
	hold = 0;
}

Tok
getnext(void)
{
	/* fprintf(stderr, "gettin next token..\n"); */
	Tok t = peek();
	tok_forward();
	return t;
}

int
main(int argc, char *argv[])
{
	Tok t;
	do {
		t = getnext();
		printtok(stdout, t);
	} while (t.type != EOI);
}
