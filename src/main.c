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

  static const uint8_t hello[] = "[echo] Echo pronto. Digita e invio.\r\n";
  serial_write_atomic(hello, sizeof(hello)-1);

  static const uint8_t prefix[] = "[echo] ";
  uint8_t line[64];
  uint8_t out[sizeof(prefix)-1 +64+2]; // prefisso+messaggio+\r\n

  for(;;){

    uint16_t i = 0;
    for(;;){
      int c = serial_getc();
      if (c == '\r' || c == '\n') break;
      if(i<64-2)  line[i++] = (uint8_t)c; // lascia spazio per \r\n
      else break;
    }

    if(i==0) continue; // niente da mandare

    uint16_t pos=0;
    for(uint8_t k=0; k<sizeof(prefix)-1; ++k)
      out[pos++] = prefix[k];
    for (uint16_t k=0; k<i; ++k)
      out[pos++] = line[k];
    out[pos++] = '\r';
    out[pos++] = '\n';

    serial_write_atomic(out, pos);
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
