#include "uart.h"
#include "string.h"

#define UINT32 unsigned int
#define UINT64 unsigned long long


/* ARM System Registers */
#define DSB     __asm__ volatile("dsb" ::: "memory")
#define DMB     __asm__ volatile("dmb" ::: "memory")
#define ISB     __asm__ volatile("isb" ::: "memory")
#define BARRIER __asm__ volatile("":::"memory")

#define ARM_SYSREG_READ(REG)                    \
({                                              \
    UINT32 _val;                                \
    __asm__ volatile("mrc " REG : "=r" (_val)); \
    _val;                                       \
})

#define ARM_SYSREG_WRITE(REG, val)              \
({                                              \
    __asm__ volatile("mcr " REG :: "r" (val));  \
    ISB;                                        \
})

#define ARM_SYSREG64_READ(REG)                   \
({                                               \
    UINT64 _val;                                 \
    __asm__ volatile("mrrc " REG : "=r" (_val)); \
    _val;                                        \
})

#define ARM_SYSREG64_WRITE(REG, val)             \
({                                               \
    __asm__ volatile("mcrr " REG :: "r" (val));  \
    ISB;                                         \
})

#define CP14_REG(CRn, Op1, CRm, Op2)    "p14, "#Op1", %0, "#CRn","#CRm","#Op2
#define CP15_REG(CRn, Op1, CRm, Op2)    "p15, "#Op1", %0, "#CRn","#CRm","#Op2
#define CP15_REG64(CRn, Op1)            "p15, "#Op1", %0,    %H0,"#CRn

/*
 * Identification registers (c0)
 */
#define MIDR                CP15_REG(c0, 0, c0, 0)    /* Main ID Register */
#define MPIDR               CP15_REG(c0, 0, c0, 5)    /* Multiprocessor Affinity Register */
#define CCSIDR              CP15_REG(c0, 1, c0, 0)    /* Cache Size ID Registers */
#define CLIDR               CP15_REG(c0, 1, c0, 1)    /* Cache Level ID Register */
#define VPIDR               CP15_REG(c0, 4, c0, 0)    /* Virtualization Processor ID Register */
#define VMPIDR              CP15_REG(c0, 4, c0, 5)    /* Virtualization Multiprocessor ID Register */

/*
 * System control registers (c1)
 */
#define SCTLR               CP15_REG(c1, 0, c0, 0)    /* System Control Register */
#define ACTLR               CP15_REG(c1, 0, c0, 1)    /* Auxiliary Control Register */
#define CPACR               CP15_REG(c1, 0, c0, 2)    /* Coprocessor Access Control Register */

/*
 * Memory protection and control registers (c2 & c3)
 */
#define TTBR0               CP15_REG(c2, 0, c0, 0)    /* Translation Table Base Register 0 */
#define TTBR1               CP15_REG(c2, 0, c0, 1)    /* Translation Table Base Register 1 */
#define TTBCR               CP15_REG(c2, 0, c0, 2)    /* Translation Table Base Control Register */
#define DACR                CP15_REG(c3, 0, c0, 0)    /* Domain Access Control Register */

/*
 * Memory system fault registers (c5 & c6)
 */
#define DFSR                CP15_REG(c5, 0, c0, 0)    /* Data Fault Status Register */
#define IFSR                CP15_REG(c5, 0, c0, 1)    /* Instruction Fault Status Register */
#define DFAR                CP15_REG(c6, 0, c0, 0)    /* Data Fault Address Register */
#define IFAR                CP15_REG(c6, 0, c0, 2)    /* Instruction Fault Address Register */

/*
 * Process, context and thread ID registers (c13)
 */
#define FCSEIDR             CP15_REG(c13, 0, c0, 0)    /* FCSE Process ID Register */
#define CONTEXTIDR          CP15_REG(c13, 0, c0, 1)    /* Context ID Register */
#define TPIDRURW            CP15_REG(c13, 0, c0, 2)    /* User Read/Write Thread ID Register */
#define TPIDRURO            CP15_REG(c13, 0, c0, 3)    /* User Read-Only Thread ID Register */
#define TPIDRPRW            CP15_REG(c13, 0, c0, 4)    /* PL1 only Thread ID Register */

#define MPIDR_CPUID_MASK    (0xffU)



#define STRING_COMB(x, y, z)        x ## y ## z

#define TIMER_REG(reg)              STRING_COMB(TIMER_REG_, CNTP, reg)
#define TIMER_REG_CTL               TIMER_REG(_CTL)     /* 32 bits */
#define TIMER_REG_TVAL              TIMER_REG(_TVAL)    /* 32 bits */
#define TIMER_REG_CVAL              TIMER_REG(_CVAL)    /* 64 bits */
#define TIMER_REG_CT                TIMER_REG(CT)       /* 64 bits */


#define TIMER_REG_CNTFRQ            CP15_REG(c14, 0, c0, 0)

/* CNTP AArch32 registers */
#define TIMER_REG_CNTP_CTL          CP15_REG(c14, 0, c2, 1)
#define TIMER_REG_CNTP_TVAL         CP15_REG(c14, 0, c2, 0)
#define TIMER_REG_CNTP_CVAL         CP15_REG64(c14, 2)
#define TIMER_REG_CNTPCT            CP15_REG64(c14, 0)

/* CNTPS AArch32 registers are banked and accessed though CNTP */
#define CNTPS CNTP

#define READ_TIMER_REG32(reg)       ARM_SYSREG_READ(reg)
#define READ_TIMER_REG64(reg)       ARM_SYSREG64_READ(reg)
#define WRITE_TIMER_REG32(reg, val) ARM_SYSREG_WRITE(reg, val)
#define WRITE_TIMER_REG64(reg, val) ARM_SYSREG64_WRITE(reg, val)




unsigned int HalClockFreqRead(void)
{
    return READ_TIMER_REG32(TIMER_REG_CNTFRQ);
}

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
	
	uart_init();
	putchar('1');
	putchar('0');
	putchar('0');
	putchar('a');
	putchar('s');
	putchar('k');
	putchar('\r');
	putchar('\n');

	put_s_hex("system tick freq = ", HalClockFreqRead());
	//put_s_hex("g_Char2's addr = ", &g_Char2);
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

