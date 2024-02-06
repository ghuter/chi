#ifndef MARSHAL_H
#define MARSHAL_H

/*
  generates header from `modsym` to `fd`
*/
void marshal(int fd, Symbols modsym[NMODSYM]);

/*
  fill `modsym` from header file `fd`
*/
void unmarshal(int fd, Symbols modsym[NMODSYM]);

#endif
