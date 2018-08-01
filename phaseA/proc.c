// proc.c, 159
// // all processes are coded here
// // processes do not use kernel data or code, must ask via service calls
//
#include "spede.h"       // cons_xxx below needs
#include "kernel_data.h" // run_pid needed below is OK
#include "proc.h"        // prototypes of processes
#include "tools.h"
#include "syscalls.h"

void IdleProc(void) {
   int i;
   unsigned short *p = (unsigned short *)0xb8000 + 79; // upper-right corner of display

   while(1) {
      *p = '0' + 0x0f00; // show '0' at upper-right corner
      for(i=0; i<LOOP/2; i++) asm("inb $0x80"); // delay .5 sec
      *p = ' ' + 0x0f00; // show '0' at upper-right corner
      for(i=0; i<LOOP/2; i++) asm("inb $0x80"); // delay, can be service
   }
}

void ChildStuff(int which){  //which terminal to display msg
    int i, my_pid, centi_sec;
    char str[] = "   ";
    my_pid = sys_getpid();
    centi_sec = my_pid * 50;
    str[0] = '0' + my_pid/10;
    str[1] = '0' + my_pid%10;

   for(i=0; i<3; i++){		  //loop 3 times and exit
	     sys_write(which, "\n\r", 2);		//next line
	     sys_write(which, "I'm the child, PID ", 19);
	     sys_write(which, str, 3);		//show my PID
	     sys_sleep(centi_sec);		//sleep for .5 sec x PID
   }
   sys_exit(100 - my_pid);			//exit, exit code to parent
}

void Ouch(void) { // signal handler
  int ppid, which;

  ppid = sys_getppid(); // follow parent
  if (ppid == 0) ppid = sys_getpid(); // no parent, use own PID

  which = ppid % 2 ? TERM1 : TERM2;
  sys_write(which, "Ouch, don't touch that! ", 24);
}

void ChildHandler(void){
  int which, child_pid, exit_code;
  char str[] = "   ";
  char str2[] = "   ";
  child_pid = sys_waitchild(&exit_code);	//block if immediately called
  str[0] = '0' + child_pid/10;
  str[1] = '0' + child_pid%10;

  str2[0] = '0' + exit_code/10;
  str2[1] = '0' + exit_code%10;
  
  which = sys_getpid() % 2 ? TERM1 : TERM2;
  sys_write(which, "\n\r", 2);
  sys_write(which, "The child PID ", 14);
  sys_write(which, str, 3);
  sys_write(which, "exited, code = ", 16);
  sys_write(which, str2, 3);
}

void UserProc(void) {
  int my_pid, centi_sec, which, fork_result;
  char str[] = "   ";
  char cmd[BUFF_SIZE];

  my_pid = sys_getpid();
  centi_sec = 50 * my_pid;
  str[0] = '0' + my_pid/10;
  str[1] = '0' + my_pid%10;

  which = (my_pid % 2)? TERM1 : TERM2;

  sys_signal(SIGINT, Ouch); // register handler to run when SIGINT occurs

  while(1) {
      sys_write(which, "\n\r", 2);      // get a new line
      sys_write(which, str, 3);         // to show my PID
      sys_write(which, "enter ", 6);    // and other msgs
      sys_write(which, "shell ", 6);
      sys_write(which, "command: ", 9);
      sys_read(which, cmd, BUFF_SIZE);  // here we read term KB
      sys_write(which, "You've entered: ", 16);
      sys_write(which, cmd, BUFF_SIZE); // verify what's read

      if(MyStrcmp(cmd, CMD_FORK) == 1) {
          fork_result = sys_fork();
          if (fork_result == -1) {
              sys_write(which, "\n\rUserProc: cannot fork!\n\r", 28);
          }
          else if (fork_result == 0) {
              sys_exec(ChildStuff, which);
          }
          else {
              ChildHandler();
          }
      }
      else if((MyStrcmp(cmd, CMD_FORK_BACKGROUND) == 1) 
         || (MyStrcmp(cmd, CMD_FORK_BACKGROUND_SPC) == 1)) {
          sys_signal(SIGCHILD, ChildHandler);	//register signal handler!
	        fork_result = sys_fork();
          if (fork_result == -1) {
	          sys_write(which, "\n\rUserProc: cannot fork!\n\r", 28);
	          sys_signal(SIGCHILD, '\0');
          }
          else if(fork_result == 0) {
	          sys_exec(ChildStuff, which);
	        }
     }
     sys_sleep(centi_sec);             // sleep for .5 sec x PID
   }
}

void Wrapper(func_p_t p) {  // arg implanted in stack
  asm("pusha");             // save regs
  p();                      // call user's signal handler
  asm("popa");              // pop back regs
  asm("mov %%ebp, %%esp; pop %%ebp; ret $4"::);	//lil complication
}

