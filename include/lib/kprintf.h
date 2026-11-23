#ifndef KPRINT_H_
#define KPRINT_H_

/**
 * \file kprintf.h
 * \brief Enthält Funktion für I/O über den UART des Raspi
 *
 * Hilfreiche Funktionen für formatierte String Ausgaben über den UART. Der
 * UART muss zuerst initialisiert werdem mit \p uart_init() aus \p uart.h
 */

/**
 * \brief Einfache printf ähnliche Funktion wie aus stdio.h
 * \param format Format String
 * \param ... Argumente
 *
 * | Vorhandene Format Parameter (%..)    | Vorhandene Feldbreitenparameter |
 * | :------------------------------------|:--------------------------------|
 * | c: Einzelner char                    | [0-9]*: Feldbreite              |
 * | s: Null-Byte terminierter String     | 0: Null Padding                 |
 * | u: unsigned int in Dezimal (auch lu) | #: Prefix                       |
 * | i: int in Dezimal          (auch li) | +: Vorzeichen                   |
 * | x: unsigned int in Hexadezimal       |                                 |
 * | p: Pointer                           |                                 |
 * | %: Prozent                           |                                 |
 */
void kprintf [[gnu::format(printf, 1, 2)]] (const char *format, ...);

#endif // KPRINT_H_
