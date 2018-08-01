// services.c, 159

#include "spede.h"
#include "kernel_types.h"
#include "kernel_data.h" 
#include "services.h"
#include "tools.h"       
#include "proc.h"

//int tick_count = 0;

// to create process, alloc PID, PCB, and process stack
// build trapframe, initialize PCB, record PID to ready_pid_q (unless 0)
void NewProcService(func_p_t proc_p) {  // arg: where process code starts
  int pid;

  if (avail_pid_q.size == 0) {
    cons_printf("Kernel Panic: no more process!\n");
    return;
  }
  
  pid = DeQ(&avail_pid_q);
  MyBzero((char*)&pcb[pid], sizeof(pcb_t));
  MyBzero((char *)&proc_stack[pid], PROC_STACK_SIZE);
  pcb[pid].state = READY;
  if (pid != 0) {
    EnQ(pid, &ready_pid_q);
  }

  pcb[pid].trapframe_p = (trapframe_t *)&proc_stack[pid][PROC_STACK_SIZE];
  pcb[pid].trapframe_p--;

  pcb[pid].trapframe_p->efl = EF_DEFAULT_VALUE | EF_INTR;
  pcb[pid].trapframe_p->eip = (unsigned int) proc_p;
  pcb[pid].trapframe_p->cs = get_cs();
}

// count runtime of process and preempt it when reaching time limit
void TimerService(void) {
   int i;

   current_time++;

   outportb(0x20, 0x60); // dismiss timer (IRQ0)

   for (i = 0; i < PROC_NUM; i++) {
      if (pcb[i].state == SLEEP 
          && pcb[i].wake_time == current_time) {
        EnQ(i, &ready_pid_q);
        pcb[i].state = READY;
      }
   }

   if (run_pid == 0) return;

   pcb[run_pid].runtime++;
   if (pcb[run_pid].runtime == TIME_LIMIT) {
      pcb[run_pid].state = READY;
      EnQ(run_pid, &ready_pid_q);
      run_pid = -1;
   }

}

void SyscallService(trapframe_t *p) {
   switch(p->eax) {
      case SYS_GETPID:
        GetPidService(&p->ebx);
        break;
      case SYS_SLEEP:
        SleepService(p->ebx);
        break;
      case SYS_READ:
        ReadService(p->ebx, (char*)p->ecx, p->edx);
        break;
      case SYS_WRITE:
        WriteService(p->ebx, (char*)p->ecx, p->edx);
        break;
      case SYS_SEMWAIT:
        SemwaitService(p->ebx);
        break;
      case SYS_SEMPOST:
        SempostService(p->ebx);
        break;
   }
        
}

void GetPidService(int *p) {
   *p = run_pid;
}

void SleepService(int centi_sec) {
   pcb[run_pid].wake_time = current_time + centi_sec;
   pcb[run_pid].state = SLEEP;
   run_pid = -1;
}

void ReadService(int fileno, char *str, int len) {
   int which;
   if (fileno != TERM1 && fileno != TERM2) return;
   which = (fileno == TERM1) ? 0 : 1;
   EnQ(run_pid, &term[which].kb_wait_q);
   pcb[run_pid].state = WAIT;
   run_pid = -1;
}

void WriteService(int fileno, char *str, int len) {
   static unsigned short *vga_p = (unsigned short *)0xb8000; // top-left
   int i, which;
   if (fileno == STDOUT) {
      for (i = 0; i < len; i++) {
        *vga_p = str[i] + 0x0f00;
        vga_p++;

        if (vga_p >= (unsigned short *) 0xb8000+25*80) { // bottom-right
          while (vga_p --> ((unsigned short *)0xb8000)) {
            *vga_p = ' ';
          }
        }

      }
   } else if (fileno == TERM1 || fileno == TERM2) {
      which = (fileno == TERM1) ? 0 : 1;
      MyStrcpy((char*)term[which].dsp, (char*)str);
      EnQ(run_pid, &term[which].dsp_wait_q);
      pcb[run_pid].state = WAIT;
      run_pid = -1;
      DspService(which);
   } else {
      cons_printf("Kernel Panic: unsupported fileno!\n");
   }
}

void SemwaitService(int sem_num){
   if(sem_num == STDOUT) {
      if(video_sem.val > 0) {
	      video_sem.val--;
	    }
	    else{
	      EnQ(run_pid, &video_sem.wait_q);
	      pcb[run_pid].state=WAIT;
	      run_pid=-1;
	    }
   }
   else{
      cons_printf("Kernel Panic: no-such semaphore number!\n");
   }
}

void SempostService(int sem_num) {
   int i;
   if(sem_num == STDOUT){
      if(video_sem.wait_q.size == 0) {
	      video_sem.val++;
	    }
	    else{
	      i = DeQ(&video_sem.wait_q);
	      pcb[i].state = READY;
	      EnQ(i, &ready_pid_q);
	    }
   }
   else{
      cons_printf("Kernel Panic: no-such semaphore number!\n");
   }
}

void TermService(int which) {
  switch (inportb(term[which].status)) {
  case DSP_READY:
      DspService(which);
      break;
  case KB_READY:
      KbService(which);
      break;
  }
}

void DspService(int which) {
   int pid;

   if (term[which].dsp[0] == '\0') return; // nothing to do

   outportb(term[which].port, term[which].dsp[0]); // disp 1st char

   lshift_str((char*)&term[which].dsp);

   if ((term[which].dsp[0] == '\0') && (term[which].dsp_wait_q.size > 0)) {
      pid = DeQ(&term[which].dsp_wait_q);
      pcb[pid].state = READY;
      EnQ(pid, &ready_pid_q);
   }
}

void KbService(int which) {
   int pid;
   char c = inportb(term[which].port);
   outportb(term[which].port, c);
   if (c != '\r') {
      append_str(c, (char*)&term[which].kb);
      return;
   }
   outportb(term[which].port, '\n');
   if (term[which].kb_wait_q.size > 0) {
      pid = DeQ(&term[which].kb_wait_q);
      pcb[pid].state = READY;   // release process
      EnQ(pid, &ready_pid_q);
      MyStrcpy((char*)pcb[pid].trapframe_p->ecx, (char*)&term[which].kb);
   }
   term[which].kb[0] = '\0'; // reset terminal kb string
}

