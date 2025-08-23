#include "os.h"
#include <stdio.h>
#include <util/delay.h>

#define STACK_SIZE 256

static TCB tcb1, tcb2, echo;
static uint8_t stack1[STACK_SIZE], stack2[STACK_SIZE], stack_echo[STACK_SIZE];

static void t1_fn(uint32_t arg) {
    (void)arg;
    while (1) {
        serial_write("Thread 1\r\n");
        _delay_ms(500);
    }
}

static void t2_fn(uint32_t arg) {
    (void)arg;
    while (1) {
        serial_write("Thread 2\r\n");
        _delay_ms(500);
    }
}

static void echo_fn(uint32_t arg) {
    (void)arg;
    serial_write("Echo thread started. Type something:\r\n");
    char c;
    while (1) {
        c = serial_getc();
        serial_putc(c);
    }
}

int main(void){
    serial_init(1150200);
    printf_init_uart();

    TCB_create(&tcb1, stack1 + STACK_SIZE - 1, t1_fn, 0);
    TCB_create(&tcb2, stack2 + STACK_SIZE - 1, t2_fn, 0);
    TCB_create(&echo, stack_echo + STACK_SIZE - 1, echo_fn, 0);

    TCBList_enqueue(&running_queue, &tcb1);
    TCBList_enqueue(&running_queue, &tcb2);
    TCBList_enqueue(&running_queue, &echo);

    startSchedule();
}