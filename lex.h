#define POK(_v, _msg) if (!(_v)) {perror(_msg); exit(1);}

extern FatArena ftident;
extern FatArena ftimmed;
extern FatArena ftlit;

Tok	getnext(void);
void	printtok(FILE *o, Tok t);
