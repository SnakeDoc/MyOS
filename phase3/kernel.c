// kernel.c, 159
// OS bootstrap and kernel code for OS phase 1
//
// Team Name: CosmicDogs (Members: Brandon Her, Breeana Proffit, Jason Sipula)

#include "spede.h"         // given SPEDE stuff
#include "kernel_types.h"  // kernle data types
#include "entry.h"         // entries to kernel
#include "tools.h"         // small tool functions
#include "proc.h"          // process names such as IdleProc()
#include "services.h"      // service code

// kernel data are all declared here:
int run_pid;                       // currently running PID; if -1, none selected
pid_q_t ready_pid_q, avail_pid_q;  // avail PID and those ready to run
pcb_t pcb[PROC_NUM];               // Process Control Blocks
char proc_stack[PROC_NUM][PROC_STACK_SIZE]; // process runtime stacks
int current_time;
semaphore_t video_sem;

struct i386_gate *IDT_p;

void InitKernelData(void) {        // init kernel data
   int i;
   run_pid = -1;
   MyBzero((char*)ready_pid_q.q, Q_SIZE);
   MyBzero((char*)avail_pid_q.q, Q_SIZE);
   for (i = 0; i < PROC_NUM; i++) {
      EnQ(i, &avail_pid_q);
   }
}

void InitKernelControl(void) {     // init kernel control
   IDT_p = get_idt_base();
   cons_printf("IDT location at DRAM addr %x (%d).\n", &IDT_p, &IDT_p);

   fill_gate(&IDT_p[TIMER], (int)TimerEntry, get_cs(), ACC_INTR_GATE, 0);
   fill_gate(&IDT_p[SYSCALL], (int)SyscallEntry, get_cs(), ACC_INTR_GATE, 0);

   outportb(0x21, ~1);
}

void ProcScheduler(void) {              // choose run_pid to load/run
   int i;

   if (run_pid > 0) return;           // user cannot schedule process

   i = DeQ(&ready_pid_q);
   if (i == -1) {
      run_pid = 0;
   } else {
      run_pid = i;
   }

   pcb[run_pid].totaltime += pcb[run_pid].runtime;
   pcb[run_pid].runtime = 0;
}

int main(void) {  // OS bootstraps

   current_time = 0;
   
   video_sem.val = 1;
   video_sem.wait_q.size = 0;

   InitKernelData();
   InitKernelControl();

   NewProcService(&IdleProc);
   ProcScheduler();
   ProcLoader(pcb[run_pid].trapframe_p);

   return 0; // compiler needs for syntax altho this statement is never exec
}

void Kernel(trapframe_t *trapframe_p) {   // kernel code runs (100 times/second)
   char key;

   pcb[run_pid].trapframe_p = trapframe_p;

   switch(trapframe_p->intr_num) {
      case TIMER:
        TimerService();
        break;
      case SYSCALL:
        SyscallService(trapframe_p);
        break;
   }

   if (cons_kbhit()) {
      key = cons_getchar();
      if (key == 'n') {
        NewProcService(&UserProc);
      } else if (key == 'b') {
        breakpoint();
      }
   }

   ProcScheduler();
   ProcLoader(pcb[run_pid].trapframe_p);
}

