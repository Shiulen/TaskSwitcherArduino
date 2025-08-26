#include "os.h"
#include <avr/io.h>

TCB idle_tcb;
static uint8_t idle_stack[128];

static void idle_fn(uint32_t arg){
    while(1) {
        // idle loop
    }
}

TCBList running_queue = {.first=NULL, .last=NULL, .size=0};

void os_yield(void){
    cli();
    schedule();
    sei();
}

void startSchedule(void){
    cli();
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

void timerStart(void){
    TCCR2A = (1<<WGM21);
    TCCR2B = (1<<CS22);
    OCR2A = 249;
    TIMSK2 = (1<<OCIE2A);
}

ISR(TIMER2_COMPA_vect){
    schedule();
}