#include <avr/io.h>

	.text
	.global archContextSwitch
	.global archFirstThreadRestore

; void archContextSwitch(TCB* old_tcb, TCB* new_tcb)
; old in r24:r25, new in r22:r23
archContextSwitch:
	; Save R2-R17, R28-R29
	push r2
	push r3
	push r4
	push r5
	push r6
	push r7
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15
	push r16
	push r17
	push r28
	push r29

	; Save SP into old_tcb->sp_save_ptr (Y points to old)
	in  r16, _SFR_IO_ADDR(SPL)
	in  r17, _SFR_IO_ADDR(SPH)
	mov r28, r24
	mov r29, r25
	st  Y,  r16
	std Y+1,r17

	; Load SP from new_tcb->sp_save_ptr
	mov r28, r22
	mov r29, r23
	ld  r16, Y
	ldd r17, Y+1
	out _SFR_IO_ADDR(SPL), r16
	out _SFR_IO_ADDR(SPH), r17

	; Restore regs
	pop r29
	pop r28
	pop r17
	pop r16
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop r7
	pop r6
	pop r5
	pop r4
	pop r3
	pop r2

	ret

; void archFirstThreadRestore(TCB* new_tcb)
; new in r24:r25
archFirstThreadRestore:
	; Load SP from new_tcb->sp_save_ptr
	mov r28, r24
	mov r29, r25
	ld  r16, Y
	ldd r17, Y+1
    out _SFR_IO_ADDR(SPL), r16
    out _SFR_IO_ADDR(SPH), r17

	; Restore regs
	pop r29
	pop r28
	pop r17
	pop r16
	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop r7
	pop r6
	pop r5
	pop r4
	pop r3
	pop r2

	ret
