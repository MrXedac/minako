/*******************************************************************************/
/*  © Université Lille 1, The Pip Development Team (2015-2016)                 */
/*                                                                             */
/*  This software is a computer program whose purpose is to run a minimal,     */
/*  hypervisor relying on proven properties such as memory isolation.          */
/*                                                                             */
/*  This software is governed by the CeCILL license under French law and       */
/*  abiding by the rules of distribution of free software.  You can  use,      */
/*  modify and/ or redistribute the software under the terms of the CeCILL     */
/*  license as circulated by CEA, CNRS and INRIA at the following URL          */
/*  "http://www.cecill.info".                                                  */
/*                                                                             */
/*  As a counterpart to the access to the source code and  rights to copy,     */
/*  modify and redistribute granted by the license, users are provided only    */
/*  with a limited warranty  and the software's author,  the holder of the     */
/*  economic rights,  and the successive licensors  have only  limited         */
/*  liability.                                                                 */
/*                                                                             */
/*  In this respect, the user's attention is drawn to the risks associated     */
/*  with loading,  using,  modifying and/or developing or reproducing the      */
/*  software by the user in light of its specific status of free software,     */
/*  that may mean  that it is complicated to manipulate,  and  that  also      */
/*  therefore means  that it is reserved for developers  and  experienced      */
/*  professionals having in-depth computer knowledge. Users are therefore      */
/*  encouraged to load and test the software's suitability as regards their    */
/*  requirements in conditions enabling the security of their systems and/or   */
/*  data to be ensured and,  more generally, to use and operate it in the      */
/*  same conditions as regards security.                                       */
/*                                                                             */
/*  The fact that you are presently reading this means that you have had       */
/*  knowledge of the CeCILL license and that you accept its terms.             */
/*******************************************************************************/

#include <stdint.h>
#include <pip/fpinfo.h>
#include <pip/io.h>
#include <pip/paging.h>
#include <pip/vidt.h>
#include <pip/api.h>

#include "minako.h"
#include "linux.h"
#include "stdlib.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

task_t linuxDescriptor;

#define PANIC() {vcli(); for(;;);}
#define MIN(a,b) ((a)<(b) ? (a) : (b))

extern void* _linux, *_elinux;

static const struct {uint32_t start, end;} linuxInfo = {
	(uint32_t)&_linux, (uint32_t)&_elinux,
};

/**
 * Page fault irq handler
 */
INTERRUPT_HANDLER(pfAsm, pfHandler)
	/*log("Page fault at ");
	puthex(data1);
	puts("\n");*/
	if(caller)
	{
		/*log("Linux partition ");
		puthex(caller);
		puts(" triggered a Page Fault, please wait.\n");*/
		if(data1 >= 0xC0000000)
		{
			uint32_t pagePhys = data1 - 0xC0000000;
			uint32_t pageVirt = data1 & 0xFFFFF000;
			uint32_t tmp = unmapPage(caller, pagePhys);
			/* log("Unmapping VAddr "); puthex(pagePhys); puts(" to remap VAddr "); puthex(tmp); puts(" -> "); puthex(pageVirt); puts("\n"); */
			if(mapPageWrapper(tmp, caller, pageVirt))
			{
				//log("Linux seems to have tried to access hardware memory. Mapping a new pase \"as if\".\n");
				uint32_t tmpPage = allocPage();
				if(mapPageWrapper(tmpPage, caller, pageVirt))
				{
					log("Still failed. Exiting.\n");
					PANIC();
				}
				log("[WARNING] Minako failed to unmap page at "); puthex(pagePhys);
				puts(" (requested access at ");
				puthex(pageVirt);
				puts(")");
				puts(" and thus mapped "); puthex(tmpPage); puts(" as a replacement; this is broken\n");
			}
			//log("Page remapped! Swapping complete. Resuming execution as if nothing happened :)\n");
			resume(caller, 0);
		} else {
			log("Linux partition ");
			puthex(caller);
			puts(" triggered a Page Fault at address ");
			puthex(data1);
			puts("\n");
			PANIC();
		}

	} else {
		log("Hypershell server pagefaulted\n");
		PANIC();
	}
END_OF_INTERRUPT

/**
 * General protection failure irq handler
 */
INTERRUPT_HANDLER(gpfAsm, gpfHandler)
	if(caller)
	{
		log("Child partition ");
		puthex(caller);
		puts(" triggered a General Protection Fault, this is bad.\n");
	} else {
		log("Hypershell server protection fault\n");
	}
	PANIC();
END_OF_INTERRUPT

INTERRUPT_HANDLER(timerAsm, timerHandler)
	/*if(caller == linuxDescriptor.part)
	{
		resume((uint32_t)linuxDescriptor.part, 1);
	} else dispatch((uint32_t)linuxDescriptor.part, 1, caller, 0);

log("bruh\n");
for(;;);*/
	/* cassé */
	/* Check for Linux's VIDT entry, and whether we're in Linux or not */
	if((linuxDescriptor.vidt->vint[1].eip) && (linuxDescriptor.vidt->vint[0].eip == 0xDEADBEEF) && (caller != linuxDescriptor.part))
	{
		/* We got a timer EIP, use timer handler */
		dispatch((uint32_t)linuxDescriptor.part, 1, caller, data2);
	} else {
		/* No handler setup, Linux is in early boot : manually resume setting PipFlags to VCLI */
		resume((uint32_t)linuxDescriptor.part, 1);
	}
END_OF_INTERRUPT

/*
 * Prepares the fake interrupt vector to receive new interrupts
 */
void initInterrupts()
{
	//registerInterrupt(33, &timerAsm, (uint32_t*)0x2020000); // We can use the same stack for both interrupts, or choose different stacks, let's play a bit
	registerInterrupt(33, &timerAsm, (uint32_t*)0x2020000); // We can use the same stack for both interrupts, or choose different stacks, let's play a bit
	registerInterrupt(14, &gpfAsm, (uint32_t*)0x2030000); /* General Protection Fault */
	registerInterrupt(15, &pfAsm, (uint32_t*)0x2040000); /* Page Fault */

	return;
}

void parse_bootinfo(pip_fpinfo* bootinfo)
{
	if(bootinfo->magic == FPINFO_MAGIC)
	log("\tBootinfo seems to be correct.\n");
	else {
	log("\tBootinfo is invalid. Aborting.\n");
	PANIC();
	}

	log("\tAvailable memory starts at ");
	puthex((uint32_t)bootinfo->membegin);
	puts(" and ends at ");
	puthex((uint32_t)bootinfo->memend);
	puts("\n");

	log("\tPip revision ");
	puts(bootinfo->revision);
	puts("\n");

	return;
}

void main(pip_fpinfo* bootinfo)
{
	uint32_t i;

	log("        _             _\n");
	log("  /\\/\\ (_)_ __   __ _| | _____\n");
	log(" /    \\| | '_ \\ / _` | |/ / _ \\\n");
	log("/ /\\/\\ \\ | | | | (_| |   < (_) |\n");
	log("\\/    \\/_|_| |_|\\__,_|_|\\_\\___/\n");
    log("Minako / Linux Loader\n");
	log("© Université Lille 1, The Pip Development Team (2015-2017)\n");
	log("For exclusive use with the Pip proto-kernel.\n");

	log("Linux image info: \n");
	parse_bootinfo(bootinfo);

	log("Initializing paging.\n");
	initPaging((void*)bootinfo->membegin, (void*)bootinfo->memend);

	log("Initializing interrupts... ");
	initInterrupts();
	puts("done.\n");

	log("Linux image info: \n");
	log("\tImage address : "); puthex(linuxInfo.start); puts("\n");
	log("\tImage address end : "); puthex(linuxInfo.end); puts("\n");
	
	log("\tImage size : "); puthex(linuxInfo.end - linuxInfo.start); puts(" bytes\n");
	if (LinuxBootstrap(linuxInfo.start, linuxInfo.end - linuxInfo.start, 0x700000, &linuxDescriptor)){
		log("Linux image failed\n");
		PANIC();
	}


	log("I'm done. I'll be booting Linux in a few seconds.\n");

	/* discard main context.. */
	resume((uint32_t)linuxDescriptor.part, 0);
	PANIC();
}
