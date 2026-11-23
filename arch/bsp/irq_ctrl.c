#include <arch/bsp/irq_ctrl.h>
#include <stddef.h>

/**
 * \file irq_ctrl.c
 * \brief BCM2835 IRQ-Controller, Basis 0x3F00B200.
 */

static constexpr unsigned int IRQ_BASE = 0x3F00B200;

struct irq_ctrl {
	unsigned int basic_pending;
	unsigned int pending1;
	unsigned int pending2;
	unsigned int fiq_control;
	unsigned int enable1;
	unsigned int enable2;
	unsigned int enable_basic;
	unsigned int disable1;
	unsigned int disable2;
	unsigned int disable_basic;
};

static_assert(offsetof(struct irq_ctrl, pending1) == 0x04);
static_assert(offsetof(struct irq_ctrl, pending2) == 0x08);
static_assert(offsetof(struct irq_ctrl, enable1) == 0x10);
static_assert(offsetof(struct irq_ctrl, enable2) == 0x14);

static volatile struct irq_ctrl *const irq = (struct irq_ctrl *)IRQ_BASE;

void irq_ctrl_enable(void)
{
	irq->enable1 = IRQ_LINE_TIMER1;
	irq->enable2 = IRQ_LINE_UART;
}

unsigned int irq_ctrl_pending1(void)
{
	return irq->pending1;
}

unsigned int irq_ctrl_pending2(void)
{
	return irq->pending2;
}
