// tools.h, 159

#ifndef _TOOLS_H_
#define _TOOLS_H_

#include "kernel_constants.h" // needed for PAGE_BASE and PAGE_SIZE
#include "kernel_types.h" // need definition of 'pid_q_t' below

#define page_addr(X) (PAGE_BASE + X * PAGE_SIZE)

void EnQ(int, pid_q_t *);
int DeQ(pid_q_t *);
void MyBzero(char *, int);
void MyStrcpy(char *dst, char *src);
void lshift_str(char *str);
void append_str(char c, char *str);
int MyStrcmp(char *s1, char *s2);
void MyMemcpy(char *dst, char *src, int bytes);

#endif

