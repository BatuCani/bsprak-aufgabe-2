#include <kernel/ktest.h>

.macro set_mode_stack ctrl, stack
	mrs r0, cpsr
	bic r0, r0, #0xFF
	orr r0, r0, #\ctrl
	msr cpsr_c, r0
	ldr sp, =\stack
.endm

.section .init

.global _start
_start:
	/* Erkennen ob Hypervisor Mode aktiv */
	mrs r0, cpsr
	and r0, r0, #0x1F
	mov r1, #0x1A	//Modebits = 0x1A = Hypervisor Mode
	cmp r0, r1		//Vergleichen und Statusbits setzen für bedingte Sprünge

	/* Wenn Hypemode erkannt: abschalten (sprung nach _exitHyper)
	 * wenn nicht erkannt: dann weitermachen (weiterlaufen nach _bsprak)
	 */
	beq _exitHyper

	/* Qemu startet immer alle 4 Kerne
	 * Wir lassen alle anderen Kerne endless loopen
	 */
_checkCores:
	/* Id des Cpu Cores Abfragen */
	mrc p15, 0, r0, c0, c0, 5
	/* Falls nicht core 0 Core disablen */
	tst r0, #3
	bne _parkCore

/* not modeled in qemu 6.0 */
_enableAlignCheck:
	/* SCTLR des cp15 laden */
	mrc p15, 0, r0, c1, c0, 0
	/* A-bit (c1[1]) auf 1 setzen, V-bit (c1[13]) löschen.
	 * V=0: die Vektorbasis kommt aus VBAR, nicht aus 0xFFFF0000.
	 */
	orr r0, r0, #0x2
	bic r0, r0, #(1 << 13)
	/* neues SCTLR speichern */
	mcr p15, 0, r0, c1, c0, 0


_bsprak:
	/* Stacks full descending, je 4KiB, 8-Byte-ausgerichtet.
	 * Oberkante ist das Ende des 128MiB-RAM. Der SP selbst wird
	 * erst beschrieben, wenn etwas gepusht wird.
	 *   SVC 0x08000000
	 *   ABT 0x07FFF000
	 *   UND 0x07FFE000
	 *   IRQ 0x07FFD000
	 *   SYS 0x07FFC000  (geteilt mit User)
	 * cpsr_c umfasst Mode, T, I und F. I und F bleiben maskiert,
	 * bis start_kernel die IRQs freigibt.
	 */
	set_mode_stack 0xD2, 0x07FFD000
	set_mode_stack 0xD7, 0x07FFF000
	set_mode_stack 0xDB, 0x07FFE000
	set_mode_stack 0xDF, 0x07FFC000
	set_mode_stack 0xD3, 0x08000000

	ldr r0, =vector_table
	mcr p15, 0, r0, c12, c0, 0

	/* Ermöglicht der Testbibliothek frühzeitig Konfigurationsvariablen
	 * in der kernel/config.c auf einen gewünschten Wert zu setzen.
	 * Achtet darauf, selbige Variablen erst nach diesem Aufruf zur
	 * Initialisierung zu nutzen.
	 * Diesen Aufruf unbedingt direkt vor dem Aufruf von `start_kernel`
	 * stehen lassen, sonst kann das zu Punktabzügen führen.
	 */
	bl  test_kernel_set_config
	/* Zu c Code springen */
	bl  start_kernel
.Lend:
	WFI
	b .Lend

_parkCore:
	/* Interrupts für Core 1 bis 3 ausschalten  */
	cpsid if
	/* In Endlosschleife springen */
	b .Lend

_exitHyper:

	/* Rücksprungadresse ins Hypervisor Rücksprungregister schreiben.
	 * Dies ist das Label _bsprak für die normale Programmausfürung.
	 */
	ldr lr, =_checkCores
	msr ELR_hyp, lr

	/* Der Hypervisor mode ist auch "nur" eine etwas kompliziertere Exception.
	 * Also springen wir aus dieser zurück.
	 * (Rücksprunge in Aufgabe 2 sind anders zu lösen!).
	 * Dazu zunächst das CPSR auslesen und die Modebits zum Supervisor ändern.
	 * Danach dies in das SPSR für den Rücksprung schreiben.
	 */
	mrs r0, cpsr
	bic r0, r0, #0x1F
	orr r0, r0, #0x13	//Supervisormode
	msr spsr_hyp, r0
	eret
