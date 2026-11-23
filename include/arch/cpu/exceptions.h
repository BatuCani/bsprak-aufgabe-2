#ifndef ARCH_CPU_EXCEPTIONS_H_
#define ARCH_CPU_EXCEPTIONS_H_

/**
 * \file exceptions.h
 *
 * Gesicherter Registersatz, den die Trampoline an die C-Handler geben.
 * Reihenfolge entspricht stmfd sp!, {r0-r12, lr}.
 */

struct exc_frame {
	unsigned int r0;
	unsigned int r1;
	unsigned int r2;
	unsigned int r3;
	unsigned int r4;
	unsigned int r5;
	unsigned int r6;
	unsigned int r7;
	unsigned int r8;
	unsigned int r9;
	unsigned int r10;
	unsigned int r11;
	unsigned int r12;
	unsigned int lr;
};

struct mode_regs {
	unsigned int user_lr;
	unsigned int user_sp;
	unsigned int irq_lr;
	unsigned int irq_sp;
	unsigned int irq_spsr;
	unsigned int abort_lr;
	unsigned int abort_sp;
	unsigned int abort_spsr;
	unsigned int undefined_lr;
	unsigned int undefined_sp;
	unsigned int undefined_spsr;
	unsigned int supervisor_lr;
	unsigned int supervisor_sp;
	unsigned int supervisor_spsr;
};

struct mode_regs read_mode_specific_registers(unsigned int entry_lr);

void undefined_handler(struct exc_frame *frame);
void supervisor_handler(struct exc_frame *frame);
void prefetch_abort_handler(struct exc_frame *frame);
void data_abort_handler(struct exc_frame *frame);
void irq_handler(struct exc_frame *frame);

#endif
