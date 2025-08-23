#pragma once
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stddef.h>

typedef uint8_t* Pointer;
typedef void (* ThreadFn)(uint32_t thread_args);

typedef enum { Running=0x0, Terminated=0x1, Ready=0x2, Blocked=0x3 } ThreadStatus;


// TCB - Thread Control Block
typedef struct TCB {
    Pointer sp_save_ptr;
    ThreadFn thread_fn;
    uint32_t thread_arg;
    struct TCB* next;
    struct TCB* prev;
    Pointer stack_bottom;
    uint32_t stack_size;
    ThreadStatus status;
} TCB;


// TCBList - doubly-linked list of TCBs
typedef struct {
  struct TCB* first;
  struct TCB* last;
  uint8_t size;
} TCBList;


// GLOBALS
extern volatile TCB* current_tcb;
extern TCBList running_queue;
extern TCB idle_tcb;

// OPS ON TCBList
TCB*  TCBList_dequeue(TCBList* list);
uint8_t TCBList_enqueue(TCBList* list, TCB* tcb);

// TCB and OS functions
void TCB_create(TCB* tcb, Pointer stack_top, ThreadFn thread_fn, uint32_t thread_arg);
void startSchedule(void);
void schedule(void);
void timerStart(void);
void os_yield(void);

void archContextSwitch(TCB* old_tcb, TCB* new_tcb);
void archFirstThreadRestore(TCB* new_tcb);

// UART
void serial_init(uint32_t baud);
int  serial_putc(char c);
int  serial_getc(void);
int  serial_write(const char* s);

// printf helper (optional)
void printf_init_uart(void);
