#ifndef ARCH_BSP_TIMER_H_
#define ARCH_BSP_TIMER_H_

/**
 * \file timer.h
 * \brief BCM2835 System Timer, Compare-Register C1.
 */

void timer_init(void);
void timer_handle_irq(void);

#endif
