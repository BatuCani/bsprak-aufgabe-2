#ifndef ARCH_BSP_IRQ_CTRL_H_
#define ARCH_BSP_IRQ_CTRL_H_

/**
 * \file irq_ctrl.h
 * \brief BCM2835 Interrupt Controller.
 *
 * System Timer 1 liegt in Pending 1, Bit 1.
 * PL011 (IRQ 57) liegt in Pending 2, Bit 25.
 */

enum irq_line {
	IRQ_LINE_TIMER1 = 1u << 1,
	IRQ_LINE_UART	= 1u << 25,
};

void irq_ctrl_enable(void);
unsigned int irq_ctrl_pending1(void);
unsigned int irq_ctrl_pending2(void);

#endif
