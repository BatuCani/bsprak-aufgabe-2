#ifndef UART_H_
#define UART_H_

/**
 * \file uart.h
 * \brief Implementierung des UART für den Raspi 2B
 *
 * Vor der Nutzung muss der UART mit der Funktion \p uart_init() initialisiert
 * werden.
 * Das Empfangen von Zeichen kann auf Interrupt Betrieb umgeschaltet werden.
 * Das Senden von Zeichen ist immer ohne Interrupt Betrieb.
 */

/**
 * \brief Initialisiert den UART
 *
 * Initialisiert den UART und aktiviert diesen.
 */
void uart_init(void);

/**
 * \brief Sendet ein Zeichen
 *
 * **Blockierend**
 *
 * Liest in einer aktiven Schleife das Flag Register
 * aus, ob ein Byte geschrieben werden kann und schreibt
 * dieses sobald möglich.
 *
 * \param chr Der zu sendende Charakter
 */
void uart_putc(char chr);

/**
 * \brief Liest ein Zeichen
 *
 * **Blockierend**
 *
 * Liest in einer aktiven Schleife das Flag Register aus bis ein Byte
 * gelesen werden kann und gibt dieses zurück.
 * Um blockierendes Lesen zu vermeiden, kann zuerst mit \p uart_read_available
 * überprüft werden, ob ein Byte vorhanden ist.
 *
 * \return Der empfangenen Charakter
 */
char uart_getc(void);


/**
 * \brief Überprüft ob ein Byte zum lesen vorhanden ist
 *
 * Überprüft, ob ein Byte mit \p uart_getc gelesen werden
 * kann.
 * \return true falls Byte vorhanden, sonst false
 */
[[nodiscard]] bool uart_read_available(void);

/**
 * \brief Aktiviert Interrupts am UART
 */
void uart_irq_enable(void);

/**
 * \brief Liest alle empfangenen Zeichen in den Ringpuffer und quittiert den IRQ.
 */
void uart_handle_rx(void);


#endif // UART_H_
