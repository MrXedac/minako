/*
 *  linux/arch/i386/kernel/head32.c -- prepare to run common code
 *
 *  Copyright (C) 2000 Andrea Arcangeli <andrea@suse.de> SuSE
 *  Copyright (C) 2007 Eric Biederman <ebiederm@xmission.com>
 */

#include <linux/init.h>
#include <linux/start_kernel.h>
#include <linux/mm.h>
#include <linux/memblock.h>

#include <asm/setup.h>
#include <asm/sections.h>
#include <asm/e820.h>
#include <asm/page.h>
#include <asm/apic.h>
#include <asm/io_apic.h>
#include <asm/bios_ebda.h>
#include <asm/tlbflush.h>
#include <asm/bootparam_utils.h>

#define SERIAL_PORT 		0x3f8
#define SERIAL_PORT_BUSY	0x3fd
void puthex(uint32_t n);

void __attribute__((__cdecl__)) pipoutb(uint32_t port, uint32_t value)
{
	__asm volatile("PUSHL %0; \
			PUSHL %1; \
			LCALL $0x30, $0;"
			:
			:"r"(value & 0x000000FF), "r"(port & 0x0000FFFF));
	return;
}

uint32_t __attribute__((__cdecl__)) pipinb(uint32_t port)
{
	uint32_t res;

	__asm volatile("PUSHL %0; \
			LCALL $0x38, $0; \
			ADDL $4, %%ESP; \
			RET"
			:
			:"r"(port & 0x0000FFFF));

	__asm volatile("MOV %%EAX, %0"
			:"=r"(res));
	return res;
}

void __attribute__((__cdecl__)) putc(char c)
{
    while (!(pip_inb((uint32_t)SERIAL_PORT_BUSY & 0x0000FFFF) & 0x20));
    pip_outb((uint32_t)SERIAL_PORT & 0x0000FFFF, (uint32_t)c);
}

void __attribute__((__cdecl__)) puts(char *msg)
{
	int i = 0;
	while(msg[i])
		putc(msg[i++]);
		//pipinb((uint32_t)SERIAL_PORT_BUSY & 0x0000FFFF);
}

#define H2C(n) ((n)<10?(n)+'0':(n)+'A'-10)
void puthex(uint32_t n)
{
	uint32_t tmp;
	putc('0');
	putc('x');
	char noZeroes=1;
	int i;
	for(i=28; i>0; i-=4)
	{
		tmp = (n >> i) & 0xF;
		if(tmp == 0 && noZeroes)
			continue;
		if(tmp >= 0xA)
		{
			noZeroes = 0;
			putc(tmp - 0xA + 'a');
		}
		else
		{
			noZeroes = 0;
			putc(tmp + '0');
		}
	}
	tmp = n & 0xF;
	if(tmp >= 0xA)
		putc(tmp - 0xA + 'a');
	else
		putc(tmp + '0');
	return;
}

void putdec(unsigned long n)
{
        char buf[16], *ptr = &buf[sizeof(buf)-1];

        if (!n){
                putc('0');
                return;
        }

        for ( *ptr = 0; ptr > buf && n; n /= 10){
                *--ptr = '0'+(n%10);
        }

        puts(ptr);
}


static void __init i386_default_early_setup(void)
{
	/* Initialize 32bit specific setup functions */
	x86_init.resources.reserve_resources = i386_reserve_resources;
	x86_init.mpparse.setup_ioapic_ids = setup_ioapic_ids_from_mpc;
}

void __debugHalt(void)
{
	puts("   Debug halt requested.\n");
	puts("   At this point, we're into the kernel in early-boot stage. Yay!\n");
	for(;;);
}

void __init clear_bss(void)
{
	puts("Clearing the BSS from ");
	puthex(__bss_start);
	puts(" to ");
	puthex(__bss_stop);
	puts("\n");
	memset(__bss_start, 0, __bss_stop - __bss_start);
}

asmlinkage __visible void __init i386_start_kernel(void)
{
	//clear_bss();
	puts("early boot in i386_start_kernel\n");
	puts("skipping the bunch of crap I'm not allowed to do\n");
//	cr4_init_shadow();
	sanitize_boot_params(&boot_params);

	x86_early_init_platform_quirks();


	/* Call the subarch specific early setup function */
	switch (boot_params.hdr.hardware_subarch) {
	case X86_SUBARCH_INTEL_MID:
		x86_intel_mid_early_setup();
		break;
	case X86_SUBARCH_CE4100:
		x86_ce4100_early_setup();
		break;
	default:
		i386_default_early_setup();
		break;
	}
	puts("exiting i386_start_kernel to jump into start_kernel()\n");

	start_kernel();
}
