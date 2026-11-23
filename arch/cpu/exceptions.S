.section .text

/*
 * Vektorbasis, 32-Byte-ausgerichtet. Einträge bei 0x00, 0x14 und 0x1c
 * werden von uns nicht benutzt.
 *
 * Auf dem Stack liegt das LR, wie es die CPU beim Eintritt geschrieben hat.
 * Erst beim Rücksprung wird der Offset abgezogen, damit ein Sprung
 * die Ausführung fortsetzt:
 *   Undefined / SVC : 0  (LR zeigt schon auf die nächste Instruktion)
 *   Prefetch Abort  : 4
 *   Data Abort      : 8
 *   IRQ             : 4
 */
.align 5
.global vector_table
vector_table:
	b	unused_exception
	b	undefined_entry
	b	supervisor_entry
	b	prefetch_entry
	b	data_abort_entry
	b	unused_exception
	b	irq_entry
	b	unused_exception

unused_exception:
	b	unused_exception

/* 14 Register sind 56 Byte. Die Stacks starten 8-Byte-ausgerichtet,
 * 56 ist durch 8 teilbar, also bleibt SP für den C-Aufruf ausgerichtet.
 * Der SVC-Stack ist der Kernel-Stack. Solange niemand ihn auf 4 Byte
 * stehen lässt, braucht das Trampolin kein extra Ausrichten.
 */
.macro exc_entry offset, handler
	stmfd	sp!, {r0-r12, lr}
	mov	r0, sp
	bl	\handler
	ldmfd	sp!, {r0-r12, lr}
	sub	lr, lr, #\offset
	movs	pc, lr
.endm

undefined_entry:
	exc_entry 0, undefined_handler

supervisor_entry:
	exc_entry 0, supervisor_handler

prefetch_entry:
	exc_entry 4, prefetch_abort_handler

data_abort_entry:
	exc_entry 8, data_abort_handler

irq_entry:
	exc_entry 4, irq_handler

/*
 * Liest die banked Register. r0 zeigt auf struct mode_regs
 * (AAPCS: große Rückgabestruktur kommt als versteckter Pointer in r0).
 * Der Aufrufer-Modus wird am Ende wiederhergestellt.
 *
 * Offsets müssen zu struct mode_regs passen.
 */
.global read_mode_specific_registers
read_mode_specific_registers:
	/* r0 = Zielstruktur, r1 = LR vom Exception-Eintritt.
	 * bl hat das banked LR bereits überschrieben, deshalb zurückschreiben.
	 */
	mov	ip, lr
	mov	lr, r1
	mrs	r1, cpsr

	cps	#0x1F
	str	lr, [r0, #0]
	str	sp, [r0, #4]

	cps	#0x12
	str	lr, [r0, #8]
	str	sp, [r0, #12]
	mrs	r2, spsr
	str	r2, [r0, #16]

	cps	#0x17
	str	lr, [r0, #20]
	str	sp, [r0, #24]
	mrs	r2, spsr
	str	r2, [r0, #28]

	cps	#0x1B
	str	lr, [r0, #32]
	str	sp, [r0, #36]
	mrs	r2, spsr
	str	r2, [r0, #40]

	cps	#0x13
	str	lr, [r0, #44]
	str	sp, [r0, #48]
	mrs	r2, spsr
	str	r2, [r0, #52]

	msr	cpsr_c, r1
	mov	lr, ip
	bx	lr
