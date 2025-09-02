#include "os.h"
#include <avr/io.h>

TCB idle_tcb;
static uint8_t idle_stack[128];

static void idle_fn(uint32_t arg __attribute__((unused))){
  while(1) { 
    // idle loop
  }
}

/* code dei task pronti da eseguire */
TCBList running_queue = {.first=NULL, .last=NULL, .size=0};

/* avvio scheduler */
void startSchedule(void){
    cli();
    /* se coda vuota crea idle */
    if(!running_queue.first) {
        TCB_create(&idle_tcb, idle_stack + sizeof(idle_stack) - 1, idle_fn, 0);
        TCBList_enqueue(&running_queue, &idle_tcb);
    }

    current_tcb = TCBList_dequeue(&running_queue);
    if(!current_tcb) {
        current_tcb = &idle_tcb;
    }
    ((TCB*)current_tcb)->status = Running;

    timerStart();

    sei();
    
    archFirstThreadRestore((TCB*)current_tcb);
}

/* corpo dello scheduler */
void schedule(void) {
    TCB* old = (TCB*)current_tcb;

    if(old && old->status == Running) {
        old->status = Ready;
        TCBList_enqueue(&running_queue, old);
    }

    current_tcb = TCBList_dequeue(&running_queue);
    if(!current_tcb) {
        current_tcb = &idle_tcb;
    }
    ((TCB*)current_tcb)->status = Running;

    if(old != current_tcb) {
        archContextSwitch(old, (TCB*)current_tcb);
    }
}

/* configurazione timer */
void timerStart(void){
    TCCR2A = (1<<WGM21);                     // modalità CTC
    TCCR2B = (1<<CS22) | (1<<CS20);          // prescaler 128
    OCR2A = 249;                             // valore di confronto per 2ms a 16MHz e prescaler 128
    TIMSK2 = (1<<OCIE2A);                    // abilita interrupt confronto
}

/* chiama scheduler ad ogni tick */
ISR(TIMER2_COMPA_vect){
    schedule();
}

/* funzioni semafori */
void mutex_init(OsMutex* m){
    m->owner = NULL;
    m->waitq.first = m->waitq.last = NULL;
    m->waitq.size = 0;
  }
  
  void mutex_lock(OsMutex* m){
    cli();
    if (m->owner == NULL){
      m->owner = (TCB*)current_tcb;
      sei();
      return;
    }
    ((TCB*)current_tcb)->status = Blocked;
    TCBList_enqueue(&m->waitq, (TCB*)current_tcb);
    schedule();
    sei();
  }
  
  void mutex_unlock(OsMutex* m){
    cli();
    if (m->owner != (TCB*)current_tcb){
      sei();
      return; /* or assert */
    }
    if (m->waitq.size){
      TCB* t = TCBList_dequeue(&m->waitq);
      m->owner = t;
      t->status = Ready;
      TCBList_enqueue(&running_queue, t);
    } else {
      m->owner = NULL;
    }
    sei();
  }