#include "os.h"
#include <util/delay.h>
#include <stdio.h>
#include <string.h>

#define STACKSZ 256

static TCB t1_tcb, t2_tcb, echo_tcb;
static uint8_t t1_stack[STACKSZ];
static uint8_t t2_stack[STACKSZ];
static uint8_t echo_stack[STACKSZ];

/* Task 1 */
static void t1_fn(uint32_t arg){
  (void)arg;
  const char* msg = "[t1] Hello from task 1\r\n";
  for(;;){
    serial_write_atomic((const uint8_t*)msg, (uint16_t)strlen(msg));
    _delay_ms(200);
  }
}

/* Task 2 */
static void t2_fn(uint32_t arg){
  (void)arg;
  const char* msg = "[t2] Greetings from task 2\r\n";
  for(;;){
    serial_write_atomic((const uint8_t*)msg, (uint16_t)strlen(msg));
    _delay_ms(350);
  }
}

/* Task Echo -> stampa quello che inserisci */
static void echo_fn(uint32_t arg){
  (void)arg;
  const uint8_t* msg = (const uint8_t*)"[echo] Echo pronto. Digita e invio.\r\n";
  serial_write_atomic(msg, sizeof(msg)-1);
  for(;;){
    uint8_t line[64];

    line[0] = '[';
    line[1] = 'e';
    line[2] = 'c';
    line[3] = 'h';
    line[4] = 'o';
    line[5] = ']';
    line[6] = ' ';

    uint16_t i = 7;
    for(;;){
      int c = serial_getc();
      if (c == '\r' || c == '\n' || i >= sizeof(line)-2){
        line[i++] = '\r';
        line[i++] = '\n';
        break;
      }
      line[i++] = (uint8_t)c;
    }
    serial_write_atomic(line, i);
  }
}

int main(void){
  serial_init(57600);

  TCB_create(&t1_tcb,   t1_stack + sizeof(t1_stack) - 1,   t1_fn,   0);
  TCB_create(&t2_tcb,   t2_stack + sizeof(t2_stack) - 1,   t2_fn,   0);
  TCB_create(&echo_tcb, echo_stack + sizeof(echo_stack) - 1, echo_fn, 0);

  TCBList_enqueue(&running_queue, &t1_tcb);
  TCBList_enqueue(&running_queue, &t2_tcb);
  TCBList_enqueue(&running_queue, &echo_tcb);

  startSchedule();
  for(;;);
}
