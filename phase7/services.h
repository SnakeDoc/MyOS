// services.h, 159

#ifndef _SERVICES_H_
#define _SERVICES_H_

#include "kernel_types.h"   // need definition of 'func_p_t' below

void NewProcService(func_p_t);
void TimerService(void);
void SyscallService(trapframe_t *p);
void GetPidService(int *p);
void GetPPidService(int *p);
void SleepService(int centi_sec);
void ReadService(int fileno, char *str, int len);
void WriteService(int fileno, char *str, int len);
void SemwaitService(int);
void SempostService(int);
void TermService(int which);
void DspService(int which);
void KbService(int which);
void ForkService(int *ebx_p);
void SignalService(int, func_p_t);
void WrapperService(int, func_p_t);

#endif
