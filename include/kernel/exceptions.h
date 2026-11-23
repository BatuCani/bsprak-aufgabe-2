#ifndef KERNEL_EXCEPTIONS_H_
#define KERNEL_EXCEPTIONS_H_

/**
 * \file exceptions.h
 *
 * irq_debug hat external linkage und ist standardmäßig false.
 * 'd' im Hauptloop kippt den Wert.
 */

extern bool irq_debug;

void do_data_abort(void);
void do_prefetch_abort(void);
void do_supervisor_call(void);
void do_undefined_inst(void);

#endif
