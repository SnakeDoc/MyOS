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
    int my_pid, sleep_period;
    char str[] = "   ";
    my_pid = sys_getpid();
    sleep_period = my_pid * 50;
    str[0] = '0' + my_pid/10;
    str[1] = '0' + my_pid%10;
    
    while(1){
      sys_write(which, "\n\r", 2);      // get a new line
      sys_write(which, str, 3);         // to show my PID
      sys_write(which, "I am the child, ", 16);
      sys_sleep(sleep_period);             // sleep for .5 sec x PID
    }
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

  while(1) {
      sys_write(which, "\n\r", 2);      // get a new line
      sys_write(which, str, 3);         // to show my PID
      sys_write(which, "enter ", 6);    // and other msgs
      sys_write(which, "shell ", 6);
      sys_write(which, "command: ", 9);
      sys_read(which, cmd, BUFF_SIZE);  // here we read term KB
      sys_write(which, "You've entered: ", 16);
      sys_write(which, cmd, BUFF_SIZE); // verify what's read

      if(MyStrcmp(cmd, "fork") == 1) {
          fork_result = sys_fork();
          if (fork_result == -1) {
              cons_printf("OS failed to work\n");
          }
          else if (fork_result == 0) {
              ChildStuff(which);
          }
          else if (fork_result > 0) {
              sys_write(which, "\n\r", 2);
              sys_write(which, str, 3);
              sys_write(which, "UserProc: ", 10);
              sys_write(which, cmd, BUFF_SIZE);
          }
      }
      sys_sleep(centi_sec);             // sleep for .5 sec x PID
  }
}

