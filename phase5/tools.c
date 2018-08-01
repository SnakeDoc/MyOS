// tools.c, 159

#include "spede.h"
#include "kernel_types.h"
#include "kernel_data.h"

// clear DRAM by setting each byte to zero
void MyBzero(char *p, int size) {
  while (size-- > 0) {
    *p++ = 0;
  }
}

// dequeue, return 1st element in array, and move all forward
// if queue empty, return -1
int DeQ(pid_q_t *p) {
   int i,  element = -1;

   if (p->size > 0) {

      element = p->q[0];

      for (i = 1; i < p->size; i++) {
        p->q[i - 1] = p->q[i];
      }
      p->q[p->size - 1] = 0;
      p->size--;
   }

   return element;
}

// enqueue element to next available position in array, 'size' is array index
void EnQ(int element, pid_q_t *p) {
   if (p->size == Q_SIZE) {
      cons_printf("Kernel Panic: queue is full, cannot EnQ!\n");
      return;
   }
   p->q[p->size] = element;
   p->size++;
}

// copies string 'src' to string 'dst'
// assumes 'dst' has enough space to store
//    and 'src' is delimited with a null char
void MyStrcpy(char *dst, char *src) {
    while ((*dst++ = *src++)); // double parens fix warning
}

// given a character string, move all characters
// in it toward the beginning by one position
void lshift_str(char *str) {
  int i;
  for (i = 1; i < BUFF_SIZE; i++) {
     str[i - 1] = str[i];
     if (str[i] == '\0') break;
  }
}

// given a character and a string, append
// the character to the string
void append_str(char c, char *str) {
  int i;
  for (i = 0; i < BUFF_SIZE; i++) { // prevents running into other memory
                                    // and potentially fatal writes if buffer full, 
    if (str[i] == '\0') {           // or no terminator in string
      str[i] = c; // found end of string, append
      str[i + 1] = '\0'; // add new null terminator
      return;
    }
  }
}

