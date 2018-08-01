// entry.S, entries to kernel services, build process trapframe

#include "kernel_constants.h"
#include <spede/machine/asmacros.h> // do not use/include "spede.h"

.comm kernel_stack, 8192, 1 // declare kernel stack space 8192 x 1 byte
.text

// loads the trapframe of the selected process in order to run it
ENTRY(ProcLoader)
   movl 4(%esp), %eax
   mov %eax, %esp
   popa
   addl $4, %esp // skip intr_num 4 bytes
   iret

ENTRY(TimerEntry)
   pushl $TIMER
   jmp EnterKernel

ENTRY(SyscallEntry)
   pushl $SYSCALL
   jmp EnterKernel

// build proc frame, and go into kernel
EnterKernel:                  // will have efl, cs, eip pushed by hardware
   pusha
   movl %esp, %ecx
   movw $0x10, %ax                 // set kernel data segment registers
   mov %ax, %ds                    // set kernel data segment registers
   mov %ax, %es                    // set kernel data segment registers
   leal kernel_stack + 8192, %esp  // point esp to the kernel stack
   pushl %ecx
   cld
   call CNAME(Kernel)

