#include <stdio.h>
#include <stdlib.h>

char buf[512];
int hd = -1;
int holdc = 0;

typedef enum {
	FOR,
	SEMICOLON,
} ETok;

typedef struct {
	ETok type;
} Tok;

char keywords[] = {
	[FOR] = "for",
};

void
forward(void)
{
	hold = 0;
}

char
peekc(void)
{
	static char c;
	ssize_t sz;

	/* return the same char until advance not used */
	if (holdc) {
		return c;
	}

	sz = read(input.fd, &c, sizeof(c));
	if (sz < 0) {
		perror("read");
		exit(1);
	}
	if (sz == 0)
		return EOF;

	holdc = 1;
	return c;
}

static int
skipwhite()
{
	char c = nextc();
	if (c != ' ' && c != '\t') {
		return 0;
	}
	do {
		advance();
		c = nextc();
	} while (c == ' ' || c == '\t');
	return 1;
}

Tok
peek()
{
	char c;
	static Tok tok;

	if (hold)
		return tok;

	memset(&tok, 0, sizeof(tok));

	c = nextc();

	/* skip whitespaces */
	if (c != EOF && c != '\0' && skipwhite()) {
		tok.type = SPC;
		hold = 1;
		/* printtok(stderr, tok); fprintf(stderr, "\n"); */
		return tok;
	}

	advance();

	switch (c) {
	case ';':
		tok.type = SEMICOLON;
		break;
	}
}

int
main(int argc, char *argv[])
{
}
