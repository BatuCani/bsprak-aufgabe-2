#include <arch/bsp/uart.h>
#include <kernel/panic.h>
#include <lib/ringbuffer.h>
#include <stddef.h>

/**
 * \file uart.c
 *
 * PL011 an 0x3F201000. Empfang läuft über IRQ in einen Ringpuffer
 * mit 128 Zeichen. uart_getc liest nur noch aus diesem Puffer.
 *
 * buff_putc aus der Vorgabe liefert true, wenn der Puffer voll ist
 * und das Zeichen nicht übernommen wurde.
 */

static constexpr unsigned int UART_BASE = 0x3F201000;

static constexpr unsigned int FR_RXFE  = 1u << 4;
static constexpr unsigned int FR_TXFF  = 1u << 5;
static constexpr unsigned int LCRH_FEN = 1u << 4;
static constexpr unsigned int LCRH_WLEN_8 = 3u << 5;
static constexpr unsigned int CR_UARTEN = 1u << 0;
static constexpr unsigned int CR_TXE = 1u << 8;
static constexpr unsigned int CR_RXE = 1u << 9;
static constexpr unsigned int IMSC_RXIM = 1u << 4;
static constexpr unsigned int ICR_RXIC = 1u << 4;
static constexpr unsigned int ICR_RTIC = 1u << 6;
static constexpr unsigned int ICR_OEIC = 1u << 10;

static constexpr unsigned int RX_BUFFER_SIZE = 128;

struct pl011 {
	unsigned int dr;
	unsigned int rsrecr;
	unsigned int unused0[4];
	unsigned int fr;
	unsigned int unused1;
	unsigned int ilpr;
	unsigned int ibrd;
	unsigned int fbrd;
	unsigned int lcrh;
	unsigned int cr;
	unsigned int ifls;
	unsigned int imsc;
	unsigned int ris;
	unsigned int mis;
	unsigned int icr;
};

static_assert(offsetof(struct pl011, fr) == 0x18);
static_assert(offsetof(struct pl011, lcrh) == 0x2C);
static_assert(offsetof(struct pl011, cr) == 0x30);
static_assert(offsetof(struct pl011, imsc) == 0x38);
static_assert(offsetof(struct pl011, icr) == 0x44);

static volatile struct pl011 *const uart = (struct pl011 *)UART_BASE;
static bool uart_ready;

create_ringbuffer(uart_rx, RX_BUFFER_SIZE);

void uart_init(void)
{
	/* PL011: erst abschalten, dann LCRH, dann wieder aktivieren.
	 * Ein Schreibzugriff auf LCRH übernimmt die Leitungskonfiguration.
	 */
	uart->cr = 0;
	uart->lcrh = LCRH_WLEN_8 | LCRH_FEN;
	uart->cr = CR_UARTEN | CR_TXE | CR_RXE;
	uart_ready = true;
}

void uart_putc(char chr)
{
	if (!uart_ready) {
		panic("UART nicht initialisiert");
	}

	while ((uart->fr & FR_TXFF) != 0) {
	}
	uart->dr = (unsigned char)chr;
}

char uart_getc(void)
{
	if (!uart_ready) {
		panic("UART nicht initialisiert");
	}

	return buff_getc(uart_rx);
}

bool uart_read_available(void)
{
	if (!uart_ready) {
		panic("UART nicht initialisiert");
	}

	return !buff_is_empty(uart_rx);
}

void uart_irq_enable(void)
{
	if (!uart_ready) {
		panic("UART nicht initialisiert");
	}

	uart->icr = ICR_RXIC | ICR_RTIC | ICR_OEIC;
	uart->imsc |= IMSC_RXIM;
}

void uart_handle_rx(void)
{
	while ((uart->fr & FR_RXFE) == 0) {
		char chr = (char)uart->dr;
		/* true heißt: Puffer voll, Zeichen wurde nicht gespeichert. */
		(void)buff_putc(uart_rx, chr);
	}
	uart->icr = ICR_RXIC | ICR_RTIC | ICR_OEIC;
}
