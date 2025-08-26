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

static volatile uint8_t rxbuf[RX_BUFSZ], txbuf[TX_BUFSZ];
static volatile uint8_t rx_head=0, rx_tail=0;
static volatile uint8_t tx_head=0, tx_tail=0;

/* Wait-queues for blocking on buffer space/data */
static TCBList uart_rx_wq = {0};
static TCBList uart_tx_wq = {0};

/* Mutexes to provide atomic multi-byte operations (no interleaving) */
static OsMutex tx_mutex;
static OsMutex rx_mutex;

static inline uint8_t rb_inc(uint8_t v, uint8_t sz){ v++; if (v==sz) v=0; return v; }
static inline uint8_t rb_free_tx(void){
  uint8_t head = tx_head, tail = tx_tail;
  if (head >= tail) return (TX_BUFSZ - (head - tail) - 1);
  else return (tail - head - 1);
}
static inline uint8_t rb_available_rx(void){
  uint8_t head = rx_head, tail = rx_tail;
  if (head >= tail) return (head - tail);
  else return (RX_BUFSZ - (tail - head));
}

void serial_init(uint32_t baud){
  uint16_t ubrr = (uint16_t)(F_CPU/16UL/baud - 1);
  UBRR0H = (uint8_t)(ubrr>>8);
  UBRR0L = (uint8_t)ubrr;
  UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);               /* 8N1 */
  UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0);     /* enable RX, TX, RX interrupt */

  mutex_init(&tx_mutex);
  mutex_init(&rx_mutex);
}

/* ===== Byte-level primitives (may interleave across tasks) ===== */

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
  UCSR0B |= (1<<UDRIE0); /* enable UDRE interrupt */
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

/* ===== Atomic multi-byte API ===== */

int serial_write_atomic(const uint8_t* data, uint16_t len){
  if (!data || len==0) return 0;
  mutex_lock(&tx_mutex);
  uint16_t i = 0;
  while (i < len){
    cli();
    uint8_t free = rb_free_tx();
    sei();
    if (free == 0){
      /* wait for space to appear */
      cli();
      ((TCB*)current_tcb)->status = Blocked;
      TCBList_enqueue(&uart_tx_wq, (TCB*)current_tcb);
      schedule();
      sei();
      continue;
    }
    /* copy as much as possible in this iteration */
    uint16_t chunk = len - i;
    if (chunk > free) chunk = free;

    /* write 'chunk' bytes into the ring buffer */
    for (uint16_t k=0; k<chunk; ++k){
      serial_putc((char)data[i+k]); /* putc already blocks briefly and enables UDRE */
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
    int c = serial_getc(); /* blocks until a byte is available */
    dst[i++] = (uint8_t)c;
  }
  mutex_unlock(&rx_mutex);
  return (int)len;
}

/* ===== Convenience wrappers ===== */
int serial_write(const char* s){
  while (*s) serial_putc(*s++);
  return 0;
}

int serial_printf_atomic(const char* fmt, ...){
  char buf[96]; /* small stack buffer; adjust if needed */
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (n < 0) return n;
  if (n >= (int)sizeof(buf)) n = (int)sizeof(buf)-1;
  return serial_write_atomic((const uint8_t*)buf, (uint16_t)n);
}

/* ===== UART ISRs ===== */

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
      if (t){ t->status = Ready; TCBList_enqueue(&running_queue, t); }
    }
    return;
  }
  UDR0 = txbuf[tx_tail];
  tx_tail = rb_inc(tx_tail, TX_BUFSZ);
}

/* ===== printf redirection (optional) ===== */
static int uart_putchar(char c, FILE* f){
  (void)f;
  if (c=='\n') serial_putc('\r');
  return serial_putc(c);
}
static FILE mystdout;

void printf_init_uart(void){
  fdev_setup_stream(&mystdout, uart_putchar, NULL, _FDEV_SETUP_WRITE);
  stdout = &mystdout;
}
