#include <arch/bsp/irq_ctrl.h>
#include <arch/bsp/timer.h>
#include <arch/bsp/uart.h>
#include <arch/bsp/yellow_led.h>
#include <kernel/config.h>
#include <kernel/exceptions.h>
#include <kernel/ktest.h>
#include <lib/kprintf.h>
#include <tests/regcheck.h>

/**
 * \file start.c
 *
 * Startet UART, System Timer und IRQs, danach die Demonstrationsschleife.
 */

static void subprogram [[noreturn]] (void);

void test_kernel_set_config [[gnu::weak]] (void)
{
}

static void enable_irqs(void)
{
	asm volatile("dsb\n\t"
		     "cpsie i" ::: "memory");
}

void start_kernel [[noreturn]] (void)
{
	init_yellow();
	yellow_on();

	uart_init();
	timer_init();
	uart_irq_enable();
	irq_ctrl_enable();
	enable_irqs();

	kprintf("=== Betriebssystem gestartet ===\n");
	test_kernel();

	while (true) {
		char c = uart_getc();
		switch (c) {
		case 'd':
			irq_debug = !irq_debug;
			break;
		case 'a':
			do_data_abort();
			break;
		case 'p':
			do_prefetch_abort();
			break;
		case 's':
			do_supervisor_call();
			break;
		case 'u':
			do_undefined_inst();
			break;
		case 'c':
			register_checker();
			break;
		case 'e':
			subprogram();
			break;
		default:
			kprintf("Unknown input: %c\n", c);
			break;
		}
	}
}

static void subprogram [[noreturn]] (void)
{
	while (true) {
		char c = uart_getc();
		for (unsigned int n = 0; n < PRINT_COUNT; n++) {
			uart_putc(c);
			for (volatile unsigned int i = 0; i < BUSY_WAIT_COUNTER; i++) {
			}
		}
	}
}
