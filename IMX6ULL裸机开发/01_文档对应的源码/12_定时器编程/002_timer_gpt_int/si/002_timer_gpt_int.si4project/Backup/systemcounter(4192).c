
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


#define OS_CYCLE_PER_TICK (g_sysClock / RT_TICK_PER_SECOND)

unsigned int g_sysClock;

unsigned int HalClockFreqRead(void)
{
    return READ_TIMER_REG32(TIMER_REG_CNTFRQ);
}

void HalClockFreqWrite(UINT32 freq)
{
    WRITE_TIMER_REG32(TIMER_REG_CNTFRQ, freq);
}

static void TimerCtlWrite(UINT32 cntpCtl)
{
    WRITE_TIMER_REG32(TIMER_REG_CTL, cntpCtl);
}

static UINT64 TimerCvalRead(void)
{
    return READ_TIMER_REG64(TIMER_REG_CVAL);
}
static UINT32 TimerTvalRead(void)
{
    return READ_TIMER_REG32(TIMER_REG_TVAL);
}


static void TimerCvalWrite(UINT64 cval)
{
    WRITE_TIMER_REG64(TIMER_REG_CVAL, cval);
}

static void TimerTvalWrite(UINT32 tval)
{
    WRITE_TIMER_REG32(TIMER_REG_TVAL, tval);
}

UINT64 HalClockGetCycles(voi)
{
    UINT64 cntpct;

    cntpct = READ_TIMER_REG64(TIMER_REG_CT);
    return cntpct;
}

void put_s_hex(const char *s, unsigned int val);

static void rt_hw_timer_isr(int vector, void *param)
{
	static int cnt=0;
    //put_s_hex("rt_hw_timer_isr ", cnt++);

    //TimerCtlWrite(0);

    rt_tick_increase(); 

	/* 
	 * bit0: OF1 Output Compare 1 Flag
	 * write 1 clear it */
	GPT1->SR |= 1;

    /*
     * use last cval to generate the next tick's timing is
     * absolute and accurate. DO NOT use tval to drive the
     * generic time in which case tick will be slower.
     */
    //TimerCvalWrite(TimerCvalRead() + OS_CYCLE_PER_TICK);
    
    //TimerTvalWrite(OS_CYCLE_PER_TICK);
    //TimerCtlWrite(1);
}


#define NUM_HAL_INTERRUPT_CNTPSIRQ      29
#define NUM_HAL_INTERRUPT_CNTPNSIRQ     30
#define OS_TICK_INT_NUM                 NUM_HAL_INTERRUPT_CNTPSIRQ // use secure physical timer for now

void rt_hw_clock_start(void)
{
    rt_hw_interrupt_umask(OS_TICK_INT_NUM);

    /* triggle the first tick */
    TimerCtlWrite(0);
    TimerTvalWrite(OS_CYCLE_PER_TICK);
	//TimerCvalWrite(HalClockGetCycles() + OS_CYCLE_PER_TICK);
    TimerCtlWrite(1);
}

extern void uart_putch_virt(char c);
int myputs(const char *s)
{
	while (*s)
	{
		uart_putch_virt(*s);
		s++;
	}
	return 0;
}

void puthex(unsigned int val)
{
	/* 0x76543210 */
	int i, j;

	//puts("0x");
	uart_putch_virt('0');
	uart_putch_virt('x');
	for (i = 7; i >= 0; i--)
	{
		j = (val >> (i*4)) & 0xf;
		if ((j >= 0) && (j <= 9))
			uart_putch_virt('0' + j);
		else
			uart_putch_virt('A' + j - 0xA);
	}	
}

void put_s_hex(const char *s, unsigned int val)
{
	myputs(s);
	puthex(val);
	myputs("\r\n");
}

#define u32 unsigned int 

/* System Counter */
struct sctr_regs {
	u32 cntcr;
	u32 cntsr;
	u32 cntcv1;
	u32 cntcv2;
	u32 resv1[4];
	u32 cntfid0;
	u32 cntfid1;
	u32 cntfid2;
	u32 resv2[1001];
	u32 counterid[1];
};
#define SC_CNTCR_ENABLE		(1 << 0)
#define SC_CNTCR_HDBG		(1 << 1)
#define SC_CNTCR_FREQ0		(1 << 8)
#define SC_CNTCR_FREQ1		(1 << 9)

#define AIPS2_ARB_BASE_ADDR             0x02100000

#define ATZ2_BASE_ADDR              AIPS2_ARB_BASE_ADDR

#define AIPS2_OFF_BASE_ADDR         (ATZ2_BASE_ADDR + 0x80000)

#define SYSCNT_CTRL_IPS_BASE_ADDR   (AIPS2_OFF_BASE_ADDR + 0x5C000)
#define SCTR_BASE_ADDR              SYSCNT_CTRL_IPS_BASE_ADDR

static int imx6ull_system_countter_init(void)
{
        struct sctr_regs *sctr = (struct sctr_regs *)(SCTR_BASE_ADDR - PV_OFFSET);
        unsigned long val, freq;

        freq = 8000000;

        sctr->cntfid0 = freq;

        /* Enable system counter */
        val = sctr->cntcr;
        val &= ~(SC_CNTCR_FREQ0 | SC_CNTCR_FREQ1);
        val |= SC_CNTCR_FREQ0 | SC_CNTCR_ENABLE | SC_CNTCR_HDBG;
        sctr->cntcr = val;

        return 0;
}


