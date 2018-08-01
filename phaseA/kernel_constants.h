// kernel_constants.h, 159

#ifndef _KERNEL_CONSTANTS_H_
#define _KERNEL_CONSTANTS_H_

#define TIMER 32             // IDT entry #32 has code addr for timer intr (DOS IRQ0)

#define LOOP 166666         // handly loop limit exec asm("inb $0x80");
#define TIME_LIMIT 200       // max timer count, then rotate process
#define PROC_NUM 20          // max number of processes
#define Q_SIZE 20            // queuing capacity
#define PROC_STACK_SIZE 4096 // process runtime stack in bytes
#define SYSCALL 128
#define STDOUT 1

/**
 * System Calls
 */
#define SYS_EXIT 1
#define SYS_FORK 2
#define SYS_READ 3
#define SYS_WRITE 4
#define SYS_WAITCHILD 7
#define SYS_EXEC 11
#define SYS_GETPID 20
#define SYS_SIGNAL 48
#define SYS_GETPPID 64
#define SYS_SLEEP 162
#define SYS_SEMWAIT 300
#define SYS_SEMPOST 301

/**
 * Virtual Memory
 */
#define VM_START 0x20000000   // VM starts at addr 500MB
#define VM_END 0x5fffffff     // VM ends at addr 1.5GB
//#define VM_TF (VM_END + 1) - sizeof(trapframe_t) - sizeof(int[2])
#define VM_TF (0x60000000 - sizeof(trapframe_t) - sizeof(int[2]))

/**
 * Page Memory
 */
#define PAGE_BASE 0xe00000 // MyOS.dli ends at byte 14M-1, 0xdfffff
#define PAGE_NUM 20        // only 20 DRAM pages to experiment
#define PAGE_SIZE 4096     // each DRAM page is 4KB in size

/**
 * Signals
 */
#define SIG_NUM 32  // 32-bit OS has 32 different signals
#define SIGINT 2    // signal ctrl-C is 2
#define SIGCHILD 17

/**
 * Terminal Constants
 */
#define TERM1 35
#define TERM2 36
#define COM2_PORT 0x2f8
#define COM3_PORT 0x3e8
#define COM4_PORT 0x2e8
#define BUFF_SIZE 101
#define DSP_READY IIR_TXRDY // term display ready
#define KB_READY IIR_RXRDY  // term KB input arrives

/**
 * Shell Commands
 */
#define CMD_FORK "fork"
#define CMD_FORK_BACKGROUND "fork&"
#define CMD_FORK_BACKGROUND_SPC "fork &"
#define CTRL_C 3  // ASCII 3
#endif
