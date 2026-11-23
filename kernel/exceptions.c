#include <arch/bsp/irq_ctrl.h>
#include <arch/bsp/timer.h>
#include <arch/bsp/uart.h>
#include <arch/cpu/exceptions.h>
#include <kernel/exceptions.h>
#include <lib/kprintf.h>
#include <stddef.h>

/**
 * \file exceptions.c
 *
 * Ausgabe nach Codebeispiel A. FS[4] des Fault-Status kommt aus Bit 10,
 * nicht aus Bit 9. Data Abort, Prefetch Abort, SVC und Undefined
 * geben danach EOT aus und halten an. IRQ kehrt zurück.
 */

bool irq_debug = false;

static constexpr unsigned int PSR_MODE_MASK = 0b11111u;
static constexpr unsigned int PSR_T = 1u << 5;
static constexpr unsigned int PSR_F = 1u << 6;
static constexpr unsigned int PSR_I = 1u << 7;
static constexpr unsigned int PSR_E = 1u << 9;
static constexpr unsigned int PSR_V = 1u << 28;
static constexpr unsigned int PSR_C = 1u << 29;
static constexpr unsigned int PSR_Z = 1u << 30;
static constexpr unsigned int PSR_N = 1u << 31;

enum psr_mode {
	PSR_USR = 0b10000,
	PSR_IRQ = 0b10010,
	PSR_SVC = 0b10011,
	PSR_ABT = 0b10111,
	PSR_UND = 0b11011,
	PSR_SYS = 0b11111,
};

enum exception_kind {
	EXC_UNDEFINED,
	EXC_SUPERVISOR_CALL,
	EXC_PREFETCH_ABORT,
	EXC_DATA_ABORT,
	EXC_IRQ,
};

static_assert(sizeof(struct mode_regs) == 14 * sizeof(unsigned int));
static_assert(offsetof(struct mode_regs, irq_spsr) == 16);
static_assert(offsetof(struct mode_regs, supervisor_spsr) == 52);
static_assert(offsetof(struct exc_frame, lr) == 13 * sizeof(unsigned int));

static unsigned int read_dfsr(void)
{
	unsigned int value;
	asm volatile("mrc p15, 0, %0, c5, c0, 0" : "=r"(value));
	return value;
}

static unsigned int read_ifsr(void)
{
	unsigned int value;
	asm volatile("mrc p15, 0, %0, c5, c0, 1" : "=r"(value));
	return value;
}

static unsigned int read_dfar(void)
{
	unsigned int value;
	asm volatile("mrc p15, 0, %0, c6, c0, 0" : "=r"(value));
	return value;
}

static unsigned int read_ifar(void)
{
	unsigned int value;
	asm volatile("mrc p15, 0, %0, c6, c0, 2" : "=r"(value));
	return value;
}

static unsigned int read_cpsr(void)
{
	unsigned int value;
	asm volatile("mrs %0, cpsr" : "=r"(value));
	return value;
}

static const char *get_fsr_description(unsigned int fsr)
{
	static const char *fsr_sources[] = {
		[0b00000] = "No function, reset value",
		[0b00001] = "Alignment fault",
		[0b00010] = "Debug event fault",
		[0b00011] = "Access Flag fault on Section",
		[0b00100] = "Cache maintenance operation fault",
		[0b00101] = "Translation fault on Section",
		[0b00110] = "Access Flag fault on Page",
		[0b00111] = "Translation fault on Page",
		[0b01000] = "Precise External Abort",
		[0b01001] = "Domain fault on Section",
		[0b01011] = "Domain fault on Page",
		[0b01100] = "External abort on Section",
		[0b01101] = "Permission fault on Section",
		[0b01110] = "External abort on Page",
		[0b01111] = "Permission fault on Page",
		[0b10000] = "TLB conflict abort",
		[0b10100] = "Implementation definedf fault",
		[0b10110] = "External Abort",
		[0b11000] = "Asynchronous parity error on memory access",
		[0b11001] = "Synchronous parity error on memory access",
		[0b11010] = "Implementation defined fault",
		[0b11100] = "Synchronous parity error on translation table walk on section",
		[0b11110] = "Synchronous parity error on translation table walk on page",
	};

	/* FS[3:0] liegen in Bit 3:0, FS[4] liegt in Bit 10. */
	constexpr unsigned int fsr_status_bit4 = 10;
	unsigned int fsr_status = (fsr & 0b1111u) | (((fsr >> fsr_status_bit4) & 1u) << 4);
	unsigned int source_count = sizeof(fsr_sources) / sizeof(fsr_sources[0]);

	if (fsr_status >= source_count || fsr_sources[fsr_status] == NULL) {
		return "Invalid fault status register value";
	}
	return fsr_sources[fsr_status];
}

static void print_psr(unsigned int psr)
{
	unsigned int mode = psr & PSR_MODE_MASK;
	const char *right_aligned_mode_name;

	kprintf("%c%c%c%c %c %c%c%c", psr & PSR_N ? 'N' : '_', psr & PSR_Z ? 'Z' : '_',
		psr & PSR_C ? 'C' : '_', psr & PSR_V ? 'V' : '_', psr & PSR_E ? 'E' : '_',
		psr & PSR_I ? 'I' : '_', psr & PSR_F ? 'F' : '_', psr & PSR_T ? 'T' : '_');

	switch (mode) {
	case PSR_USR:
		right_aligned_mode_name = "    User";
		break;
	case PSR_SYS:
		right_aligned_mode_name = " System";
		break;
	case PSR_SVC:
		right_aligned_mode_name = "Supervisor";
		break;
	case PSR_ABT:
		right_aligned_mode_name = "   Abort";
		break;
	case PSR_IRQ:
		right_aligned_mode_name = "     IRQ";
		break;
	case PSR_UND:
		right_aligned_mode_name = " Undefined";
		break;
	default:
		right_aligned_mode_name = " Invalid";
		break;
	}

	kprintf(" %s", right_aligned_mode_name);
	kprintf(" 0x%08x", psr);
}

/*
 * frame->lr ist der Hardware-Wert beim Eintritt.
 * Data Abort: fehlerhafte Instruktion = LR - 8.
 * Alles andere, inklusive IRQ-Rücksprungadresse: LR - 4.
 */
static unsigned int exception_source(enum exception_kind kind, unsigned int entry_lr)
{
	if (kind == EXC_DATA_ABORT) {
		return entry_lr - 8u;
	}
	return entry_lr - 4u;
}

static void print_exception_infos(enum exception_kind kind, struct exc_frame *frame)
{
	static const char *exception_names[] = {
		[EXC_UNDEFINED] = "Undefined Instruction",
		[EXC_SUPERVISOR_CALL] = "Supervisor Call",
		[EXC_PREFETCH_ABORT] = "Prefetch Abort",
		[EXC_DATA_ABORT] = "Data Abort",
		[EXC_IRQ] = "Interrupt",
	};
	unsigned int source = exception_source(kind, frame->lr);
	struct mode_regs mode_regs = read_mode_specific_registers(frame->lr);
	unsigned int cpsr = read_cpsr();

	kprintf("\n############ EXCEPTION ############\n");
	kprintf("%s an Adresse: 0x%08x\n", exception_names[kind], source);

	if (kind == EXC_DATA_ABORT) {
		unsigned int dfsr = read_dfsr();
		kprintf("Data Fault Status Register: 0x%08x -> %s\n", dfsr,
			get_fsr_description(dfsr));
		kprintf("Data Fault Adress Register: 0x%08x\n", read_dfar());
	}

	if (kind == EXC_PREFETCH_ABORT) {
		unsigned int ifsr = read_ifsr();
		kprintf("Instruction Fault Status Register: 0x%08x -> %s\n", ifsr,
			get_fsr_description(ifsr));
		kprintf("Instruction Fault Adress Register: 0x%08x\n", read_ifar());
	}

	kprintf("\n>> Registerschnappschuss <<\n");
	kprintf("R0: 0x%08x  R5: 0x%08x  R10: 0x%08x\n", frame->r0, frame->r5, frame->r10);
	kprintf("R1: 0x%08x  R6: 0x%08x  R11: 0x%08x\n", frame->r1, frame->r6, frame->r11);
	kprintf("R2: 0x%08x  R7: 0x%08x  R12: 0x%08x\n", frame->r2, frame->r7, frame->r12);
	kprintf("R3: 0x%08x  R8: 0x%08x\n", frame->r3, frame->r8);
	kprintf("R4: 0x%08x  R9: 0x%08x\n", frame->r4, frame->r9);

	kprintf("\n>> Modusspezifische Register <<\n");
	kprintf("User/System | LR: 0x%08x | SP: 0x%08x | CPSR: ", mode_regs.user_lr,
		mode_regs.user_sp);
	print_psr(cpsr);
	kprintf("\nIRQ         | LR: 0x%08x | SP: 0x%08x | SPSR: ", mode_regs.irq_lr,
		mode_regs.irq_sp);
	print_psr(mode_regs.irq_spsr);
	kprintf("\nAbort       | LR: 0x%08x | SP: 0x%08x | SPSR: ", mode_regs.abort_lr,
		mode_regs.abort_sp);
	print_psr(mode_regs.abort_spsr);
	kprintf("\nUndefined   | LR: 0x%08x | SP: 0x%08x | SPSR: ", mode_regs.undefined_lr,
		mode_regs.undefined_sp);
	print_psr(mode_regs.undefined_spsr);
	kprintf("\nSupervisor  | LR: 0x%08x | SP: 0x%08x | SPSR: ", mode_regs.supervisor_lr,
		mode_regs.supervisor_sp);
	print_psr(mode_regs.supervisor_spsr);
	kprintf("\n");
}

static void stop_after_exception [[noreturn]] (void)
{
	uart_putc('\4');
	asm volatile("cpsid if" ::: "memory");
	while (true) {
	}
}

void undefined_handler(struct exc_frame *frame)
{
	print_exception_infos(EXC_UNDEFINED, frame);
	stop_after_exception();
}

void supervisor_handler(struct exc_frame *frame)
{
	print_exception_infos(EXC_SUPERVISOR_CALL, frame);
	stop_after_exception();
}

void prefetch_abort_handler(struct exc_frame *frame)
{
	print_exception_infos(EXC_PREFETCH_ABORT, frame);
	stop_after_exception();
}

void data_abort_handler(struct exc_frame *frame)
{
	print_exception_infos(EXC_DATA_ABORT, frame);
	stop_after_exception();
}

void irq_handler(struct exc_frame *frame)
{
	unsigned int pending1 = irq_ctrl_pending1();
	unsigned int pending2 = irq_ctrl_pending2();

	if ((pending1 & IRQ_LINE_TIMER1) != 0) {
		timer_handle_irq();
	}
	if ((pending2 & IRQ_LINE_UART) != 0) {
		uart_handle_rx();
	}
	if (irq_debug) {
		print_exception_infos(EXC_IRQ, frame);
	}
}

void do_data_abort(void)
{
	unsigned int value;
	asm volatile("ldr %0, [%1]" : "=r"(value) : "r"(1u));
	(void)value;
}

void do_prefetch_abort(void)
{
	asm volatile("bkpt #0");
}

void do_supervisor_call(void)
{
	asm volatile("svc #0");
}

void do_undefined_inst(void)
{
	asm volatile(".word 0xe7ffffff");
}
