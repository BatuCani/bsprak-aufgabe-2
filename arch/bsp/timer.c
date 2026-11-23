#include <arch/bsp/timer.h>
#include <kernel/config.h>
#include <lib/kprintf.h>
#include <stddef.h>

/**
 * \file timer.c
 *
 * BCM2835 System Timer. C0 und C2 sind auf der Hardware für die GPU
 * reserviert, deshalb läuft der periodische Tick über C1.
 */

static constexpr unsigned int TIMER_BASE = 0x3F003000;
static constexpr unsigned int TIMER_CS_M1 = 1u << 1;
static constexpr unsigned int TIMER_CS_ALL = 0xFu;

struct systimer {
	unsigned int cs;
	unsigned int clo;
	unsigned int chi;
	unsigned int c0;
	unsigned int c1;
	unsigned int c2;
	unsigned int c3;
};

static_assert(offsetof(struct systimer, clo) == 0x04);
static_assert(offsetof(struct systimer, c1) == 0x10);
static_assert(offsetof(struct systimer, c3) == 0x18);

static volatile struct systimer *const timer = (struct systimer *)TIMER_BASE;

static unsigned int next_deadline(void)
{
	unsigned int next = timer->c1 + TIMER_INTERVAL;
	unsigned int now = timer->clo;

	/* Verpassten Tick nachholen, sonst feuert der Compare sofort wieder. */
	if ((int)(next - now) <= 0) {
		next = now + TIMER_INTERVAL;
	}
	return next;
}

void timer_init(void)
{
	timer->cs = TIMER_CS_ALL;
	timer->c1 = timer->clo + TIMER_INTERVAL;
}

void timer_handle_irq(void)
{
	timer->c1 = next_deadline();
	timer->cs = TIMER_CS_M1;
	kprintf("!\n");
}
