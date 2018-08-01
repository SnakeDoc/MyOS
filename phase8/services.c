// services.c, 159

#include "spede.h"
#include "kernel_types.h"
#include "kernel_data.h" 
#include "services.h"
#include "tools.h"       
#include "proc.h"

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
      case SYS_GETPPID:
        GetPPidService(&p->ebx);
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
      case SYS_FORK:
      	ForkService(&p->ebx);
        break;
      case SYS_SIGNAL:
        SignalService(p->ebx, (func_p_t)p->ecx);
        break;
      case SYS_EXIT:
        ExitService((int)p->ebx);
        break;
      case SYS_WAITCHILD:
        WaitchildService((int *)p->ebx, &p->ecx);
        break;
   }
        
}

void GetPidService(int *p) {
   *p = run_pid;
}

void GetPPidService(int *p) {
   *p = pcb[run_pid].ppid;
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
   if (c == CTRL_C) {
      if (term[which].kb_wait_q.size > 0) {
        
        // release process
        pid = DeQ(&term[which].kb_wait_q);
        pcb[pid].state = READY;
        EnQ(pid, &ready_pid_q);

        // if process has a handler for SIGINT
        if (signal_table[pid][SIGINT] != 0) {
          WrapperService(pid, signal_table[pid][SIGINT]);
        }
        else {
          outportb(term[which].port, '^');
        }
      }
      return;
   }
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

void ForkService(int *ebx_p) {
    int diff, pid;
    int *p;
    
    if(avail_pid_q.size == 0) {
        *ebx_p = -1;
	      cons_printf("Kernel Panic: Fork Error, no more Processes!\n");
	      return;
    }
    
    pid = DeQ(&avail_pid_q);
    *ebx_p = pid;
    EnQ(pid, &ready_pid_q);
    
    MyBzero((char*) &pcb[pid], sizeof(pcb_t));
    pcb[pid].state = READY;
    pcb[pid].ppid = run_pid;

    MyMemcpy((char*) &proc_stack[pid][0], (char*) &proc_stack[run_pid][0], PROC_STACK_SIZE);
    MyMemcpy((char*) &signal_table[pid][0], (char*) &signal_table[run_pid][0], SIG_NUM);

    diff = &proc_stack[pid][0] - &proc_stack[run_pid][0];

    pcb[pid].trapframe_p = (trapframe_t*)((int)pcb[run_pid].trapframe_p + diff);

    pcb[pid].trapframe_p->esp = pcb[pid].trapframe_p->esp + diff;
    pcb[pid].trapframe_p->ebp = pcb[pid].trapframe_p->ebp + diff;
    pcb[pid].trapframe_p->esi = pcb[pid].trapframe_p->esi + diff;
    pcb[pid].trapframe_p->edi = pcb[pid].trapframe_p->edi + diff;
    pcb[pid].trapframe_p->ebx = 0;
    
    p = (int*)pcb[pid].trapframe_p->ebp;
    while(*p != 0) {
        *p += diff;
        p = (int*)*p;
    }
}

void SignalService(int signal, func_p_t function) {
    signal_table[run_pid][signal] = function;
}

void WrapperService(int pid, func_p_t p) {
    trapframe_t tmp;
    int* q;

    MyMemcpy((char*)&tmp, (char*)pcb[pid].trapframe_p, sizeof(trapframe_t));

    q = (int*)((int)pcb[pid].trapframe_p + sizeof(trapframe_t) - 4);
    *q = (int)p;
    q--; 
    *q = tmp.eip;

    // lower trapframe location by 8 bytes
    pcb[pid].trapframe_p = (trapframe_t*)((int)pcb[pid].trapframe_p - 8);
    MyMemcpy((char*)pcb[pid].trapframe_p, (char*)&tmp, sizeof(trapframe_t));

    pcb[pid].trapframe_p->eip = (int)Wrapper;
}

void ExitService(int exit_code){	//as child calls sys_exit()
    int ppid, *p;
	
	  ppid = pcb[run_pid].ppid;

    if(pcb[ppid].state != WAITCHILD) {
	    pcb[run_pid].state = ZOMBIE;
		  run_pid = -1;
		  if(signal_table[ppid][SIGCHILD] != 0) {
			  WrapperService(ppid, (func_p_t)signal_table[ppid][SIGCHILD]);
      }
		  return;
	  }

	  pcb[ppid].state = READY;
    EnQ(ppid, &ready_pid_q);
	  //give parent child PID exiting
    pcb[ppid].trapframe_p->ecx = run_pid;
	  //give parent exit code
    p = (int*)pcb[ppid].trapframe_p->ebx;
    *p = (int)exit_code;
		
    // reclaim child resources
    pcb[run_pid].state = AVAIL;
	  EnQ(run_pid, &avail_pid_q);
	  MyBzero((char*) &pcb[run_pid], sizeof(pcb_t));
	  MyBzero((char *)&proc_stack[run_pid], PROC_STACK_SIZE);
    MyBzero((char *)&signal_table[run_pid][0], SIG_NUM);

    run_pid = -1;
}

void WaitchildService(int *exit_code_p, int *child_pid_p){ //parent requests
    int child_pid, i;

    child_pid = -1;

    for (i=0; i < PROC_STACK_SIZE; i++){
	      if(pcb[i].state == ZOMBIE && pcb[i].ppid == run_pid) {
			      child_pid = i;
			      break;
	  	  }
 	  }

	  if (child_pid == -1){
	      pcb[run_pid].state = WAITCHILD;
		    run_pid = -1;
		    return;
 	  }
	
	  //child pid
	  *child_pid_p = child_pid;
	  //exit code
    *exit_code_p = pcb[child_pid].trapframe_p->ebx;
	
    pcb[child_pid].state = AVAIL;
	  EnQ(child_pid, &avail_pid_q);
	  MyBzero((char*) &pcb[child_pid], sizeof(pcb_t));
	  MyBzero((char *)&proc_stack[child_pid], PROC_STACK_SIZE);
	  MyBzero((char *)&signal_table[child_pid][0], SIG_NUM);	   
}

