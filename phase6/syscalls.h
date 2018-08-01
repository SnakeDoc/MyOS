// syscalls.h
//

#ifndef __SYSCALLS_H__
#define __SYSCALLS_H__

int sys_getpid(void);
void sys_read(int, char*, int);
void sys_write(int, char*, int);
void sys_sleep(int);
void sys_semwait(int);
void sys_sempost(int);
int sys_fork(void);

#endif
