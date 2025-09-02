# Arduino Uno mini-OS con scheduler preemptive + UART "gestita dall'OS"

Questo progetto fornisce:
- **Scheduler preemptive** con TCB, context switch in ASM
- **Driver UART** con buffer circolari RX/TX e API bloccanti
- **Timer2** come tick a 2 kHz
- Esempio con tre thread: `t1`, `t2`, `echo`

## Requisiti
- `avr-gcc`, `avr-libc`, `avrdude` installati
- Arduino **Uno (ATmega328P, 16 MHz)** con bootloader attivo

## Build
```bash
make
```

## Flash (imposta la seriale corretta)
```bash
make flash PORT=/dev/ttyACM0
```

Se il tuo bootloader usa 57600 baud:
```bash
make flash PORT=/dev/ttyACM0 AVRDUDE_BAUD=57600
```

## Serial Monitor
Apri un terminale a 115200 8N1 e vedrai:
```
t1
t2
...
Echo pronto. Digita e invio.
```
L'eco restituisce i caratteri ricevuti (con CR->LF).

## Struttura
```
Makefile
include/os.h
src/arch.S
src/scheduler.c
src/tcb.c
src/uart.c
src/main.c
```

## Note tecniche
- Lo stack iniziale del thread contiene i registri "dummy" e l'indirizzo di `_trampoline`.
- Il tick preemptive chiama `schedule()` ad ogni interrupt `TIMER2_COMPA_vect`.
- Le API UART si bloccano mettendo il thread in wait-queue e riattivandolo dagli ISR.