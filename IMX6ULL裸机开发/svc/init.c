
#include "string.h"

#if 0
void SystemInit(void *loadaddr)
{
	extern char __text_start;
	extern char __bss_start;
	extern char __bss_end;

	unsigned int len;
	
	/* text/rodata/data重定位 */
	len = &__bss_start - &__text_start;
	memcpy(&__text_start, loadaddr, len);    /* 目的, 源, 长度 */

	/* clear bss */
	len = &__bss_end - &__bss_start;
	memset(&__bss_start, 0, len);   /* dest, val(0), len */
}

#else
void SystemInit(void *loadaddr)
{
	extern char __text_start[];
	extern char __bss_start[];
	extern char __bss_end[];

	unsigned int len;
	
	/* text/rodata/data重定位 */
	len = __bss_start - __text_start;
	memcpy(__text_start, loadaddr, len);    /* 目的, 源, 长度 */

	/* clear bss */
	len = __bss_end - __bss_start;
	memset(__bss_start, 0, len);   /* dest, val(0), len */
}

#endif
