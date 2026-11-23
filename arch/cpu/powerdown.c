#include <arch/cpu/powerdown.h>

void powerdown [[gnu::noreturn]] (void)
{
	// see https://developer.arm.com/documentation/dui0058/d/semihosting/semihosting/the-swi-interface?lang=en
	int angel_SWIreason_ReportException = 0x18;
	int ADP_Stopped_ApplicationExit = 0x20026;

	asm("svc 0x00123456" :: "{r0}"(angel_SWIreason_ReportException), "{r1}"(ADP_Stopped_ApplicationExit));

	for(;;){}
}
