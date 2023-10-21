#include "uart.h"
#include "string.h"

char g_Char = 'A';
const char g_Char2 = 'B';
int g_A = 0;
int g_B;


void delay(volatile unsigned int d)
{
	while (d--);
}

int main(void)
{
	char c;
	void (* funcptr)(const char *s, unsigned int val);
	funcptr = put_s_hex;
	
	uart_init();
	putchar('1');
	putchar('0');
	putchar('0');
	putchar('a');
	putchar('s');
	putchar('k');
	putchar('\r');
	putchar('\n');
	funcptr("hello, test function ptr", 123);

	put_s_hex("g_A = ", g_A);
	put_s_hex("g_B = ", g_B);
	put_s_hex("g_Char's  addr = ", &g_Char);
	put_s_hex("g_Char2's addr = ", &g_Char2);
	puthex(&g_Char);
	puthex(&g_Char2);
	putchar('\r');
	putchar('\n');
	
	putchar(g_Char);
	putchar(g_Char2);

	while (1)
	{
		c = getchar();
		putchar(c);
		putchar(c+1);
	}
	
	return 0;
}

