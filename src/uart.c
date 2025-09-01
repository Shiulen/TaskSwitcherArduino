#include "os.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <stdarg.h>

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#define RX_BUFSZ 128
#define TX_BUFSZ 128

/* Definizione ring buffer */
static volatile uint8_t rxbuf[RX_BUFSZ], txbuf[TX_BUFSZ];
static volatile uint8_t rx_head=0, rx_tail=0;
static volatile uint8_t tx_head=0, tx_tail=0;

/* code di attesa per i task bloccati su scrittura e lettura */
static TCBList uart_rx_wq = {0};
static TCBList uart_tx_wq = {0};

/* semafori per atomicità */
static OsMutex tx_mutex;
static OsMutex rx_mutex;

/* incrementa ring buffer */
static uint8_t rb_inc(uint8_t v, uint8_t sz){
  v++;
  if (v==sz) v=0;
  return v;
}

/* ritorna quanto spazio libero ho nel buffer tx */
static uint8_t rb_free_tx(void){
  uint8_t head = tx_head, tail = tx_tail;
  if (head >= tail) return (TX_BUFSZ - (head - tail) - 1);
  else return (tail - head - 1);
}

/* inizializziamo la seriale */
void serial_init(uint32_t baud){
  uint16_t ubrr = (uint16_t)(F_CPU/16UL/baud - 1);
  UBRR0H = (uint8_t)(ubrr>>8);
  UBRR0L = (uint8_t)ubrr;
  UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
  UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);

  mutex_init(&tx_mutex);
  mutex_init(&rx_mutex);
}

/* primitive base per la seriale */

int serial_putc(char c){
  cli();
  uint8_t next = rb_inc(tx_head, TX_BUFSZ);
  while (next == tx_tail){
    ((TCB*)current_tcb)->status = Blocked;
    TCBList_enqueue(&uart_tx_wq, (TCB*)current_tcb);
    schedule();
    next = rb_inc(tx_head, TX_BUFSZ);
  }
  txbuf[tx_head] = (uint8_t)c;
  tx_head = next;
  UCSR0B |= (1<<UDRIE0);
  sei();
  return 0;
}

int serial_getc(void){
  int c;
  cli();
  while (rx_head == rx_tail){
    ((TCB*)current_tcb)->status = Blocked;
    TCBList_enqueue(&uart_rx_wq, (TCB*)current_tcb);
    schedule();
  }
  c = rxbuf[rx_tail];
  rx_tail = rb_inc(rx_tail, RX_BUFSZ);
  sei();
  return c;
}

/* primitive atomice per la seriale */

int serial_write_atomic(const uint8_t* data, uint16_t len){
  if (!data || len==0) return 0;
  mutex_lock(&tx_mutex);
  uint16_t i = 0;
  while (i < len){
    cli();
    uint8_t free = rb_free_tx();
    sei();
    if (free == 0){
      cli();
      ((TCB*)current_tcb)->status = Blocked;
      TCBList_enqueue(&uart_tx_wq, (TCB*)current_tcb);
      schedule();
      sei();
      continue;
    }
    uint16_t chunk = len - i;
    if (chunk > free) chunk = free;

    for (uint16_t k=0; k<chunk; ++k){
      serial_putc((char)data[i+k]);
    }
    i += chunk;
  }
  mutex_unlock(&tx_mutex);
  return (int)len;
}

int serial_read_atomic(uint8_t* dst, uint16_t len){
  if (!dst || len==0) return 0;
  mutex_lock(&rx_mutex);
  uint16_t i = 0;
  while (i < len){
    int c = serial_getc();
    dst[i++] = (uint8_t)c;
  }
  mutex_unlock(&rx_mutex);
  return (int)len;
}

/* ISR per tx e rx */

ISR(USART_RX_vect){
  uint8_t d = UDR0;
  uint8_t next = rb_inc(rx_head, RX_BUFSZ);
  if (next != rx_tail){
    rxbuf[rx_head] = d;
    rx_head = next;
  }
  if (uart_rx_wq.size){
    TCB* t = TCBList_dequeue(&uart_rx_wq);
    if (t){ t->status = Ready; TCBList_enqueue(&running_queue, t); }
  }
}

ISR(USART_UDRE_vect){
  if (tx_head == tx_tail){
    UCSR0B &= ~(1<<UDRIE0);
    if (uart_tx_wq.size){
      TCB* t = TCBList_dequeue(&uart_tx_wq);
      if (t){
        t->status = Ready;
        TCBList_enqueue(&running_queue, t); 
      }
    }
    return;
  }
  UDR0 = txbuf[tx_tail];
  tx_tail = rb_inc(tx_tail, TX_BUFSZ);
}