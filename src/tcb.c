#include "os.h"

volatile TCB* current_tcb = NULL;

static void _trampoline(void){
    sei();
    if(current_tcb && current_tcb->thread_fn){
        current_tcb->thread_fn(current_tcb->thread_arg);
    }
    cli();
    ((TCB*)current_tcb)->status = Terminated;
    schedule();
}

void TCB_create(TCB* tcb, Pointer stack_top, ThreadFn thread_fn, uint32_t thread_arg){

    tcb->thread_fn = thread_fn;
    tcb->thread_arg = thread_arg;
    tcb->next = NULL;
    tcb->prev = NULL;
    tcb->status = Ready;

    uint8_t* sp = stack_top;

    *sp-- = (uint8_t)(((uint16_t)_trampoline) & 0xFF);
    *sp-- = (uint8_t)((((uint16_t)_trampoline) >> 8) & 0xFF);

    for (uint8_t r = 2; r <= 17; ++r) *sp-- = 0x00;
    *sp-- = 0x00;
    *sp-- = 0x00; 

    tcb->sp_save_ptr = sp;
}

TCB* TCBList_dequeue(TCBList* list){
    TCB* t = list->first;
    if(!t) return NULL;
    
    if (list->size==1) {
        list->first = list->last = NULL;
    }
    else {
        TCB* next=t->next;
        list->first=next;
        next->prev=NULL;
    }

    --list->size;
    t->next=NULL;
    t->prev=NULL;
    return t;
}

uint8_t TCBList_enqueue(TCBList* list, TCB* tcb){
    if(tcb->prev!=NULL || tcb->next!=NULL)
        return 0;
    
    if (!list->size) {
        list->first=tcb;
        list->last=tcb;
        tcb->prev=NULL;
        tcb->next=NULL;
    }
    else {
        list->last->next=tcb;
        tcb->prev=list->last;
        tcb->next=NULL;
        list->last=tcb;
    }
    ++list->size;
    return 1;
}