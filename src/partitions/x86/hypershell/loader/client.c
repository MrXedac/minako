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
#include <pip/api.h>
#include <pip/vidt.h>
#include <pip/paging.h>
#include <string.h>

#include "server.h"
#include "client.h"

#define CLIENT_LOG(a) puts("[LOADER:LINUX] "); puts(a)

char *strcpy(char *d, const char *s){
	char *saved = d;
	while ((*d++ = *s++) != '\0');
	
	return saved;
}

int
ClientBootstrap(uint32_t base, uint32_t length, uint32_t load_addr, task_t* part)
{
	uint32_t offset, i;
	void *rtospd, *rtossh1, *rtossh2, *rtossh3;
	CLIENT_LOG("Parsing Linux header... \n");
	CLIENT_LOG("Checking flags : ");
	uintptr_t flag_addr = base + 0x1FE;
	puthex(*(uint16_t*)flag_addr);
	puts("\n");
	if(*(uint16_t*)flag_addr == 0xAA55)
	{
		CLIENT_LOG("Kernel header is correct!\n");
	}
	else
	{
		CLIENT_LOG("Kernel header is incorrect. Aborting.\n");
		goto fail;
	}
	
	CLIENT_LOG("Kernel version string : ");
	uintptr_t version_string = base + 0x20E;
	//char* vstr = (char*)*(uint32_t*)version_string;
	puthex(*(uint32_t*)version_string);
	//puts(vstr);
	
	uintptr_t code32_start = base + 0x214;
	CLIENT_LOG("Code32_Start : ");
	puthex(*(uint32_t*)code32_start);
	puts("\n");
	CLIENT_LOG("Setting up 32 bits entrypoint.\n");
	*(uint32_t*)code32_start = 0x800080;
	
	uintptr_t pref_laddr = base + 0x258;
	CLIENT_LOG("Prefered loading address : ");
	puthex(*(uint32_t*)pref_laddr);
	puts("\n");
	
	uintptr_t krn_off = base + 0x248;
	CLIENT_LOG("Kernel payload offset : ");
	puthex(*(uint32_t*)krn_off);
	puts("\n");
	
	uintptr_t krn_lgt = base + 0x24C;
	CLIENT_LOG("Kernel payload length : ");
	puthex(*(uint32_t*)krn_lgt);
	puts("\n");
	
	uintptr_t bt_type = base + 0x210;
	CLIENT_LOG("Filling in bootloader type.\n");
	*(uint8_t*)bt_type = 0xFF;
	puts("\n");
	
	uintptr_t lflags = base + 0x211;
	CLIENT_LOG("Filling in loadflags.\n");
	*(uint8_t*)lflags |= 0x40;
	puts("\n");
	
	uintptr_t rdisk = base + 0x218;
	CLIENT_LOG("Filling in ramdisk address.\n");
	*(uint32_t*)rdisk = 0x0;
	puts("\n");
	
	uintptr_t rdisksize = base + 0x21c;
	CLIENT_LOG("Filling in ramdisk size.\n");
	*(uint32_t*)rdisksize = 0x0;
	puts("\n");
	
	uintptr_t heap_end_ptr = base + 0x224;
	CLIENT_LOG("Filling in heap end pointer.\n");
	*(uint32_t*)heap_end_ptr = 0x011FFDFF;
	puts("\n");
	
	uintptr_t setupsect = base + 0x1F1;
	uint8_t setupSectorCount = *(uint8_t*)setupsect;
	CLIENT_LOG("Setup sector size : ");
	puthex(setupSectorCount);
	puts("\n");
	
	CLIENT_LOG("Protected mode kernel offset : ");
	uint32_t protOffset = (setupSectorCount + 1) * 512;
	puthex((setupSectorCount + 1) * 512);
	puts("\n");
	
	uint32_t laddr = *(uint32_t*)pref_laddr;
	
	CLIENT_LOG("32 bits payload magic : ");
	uintptr_t payloadMagicLocation = base + protOffset + *(uint32_t*)krn_off;
	puts("Protected mode payload should be at "); puthex(payloadMagicLocation); puts("\n");
	puthex(*(uint16_t*)payloadMagicLocation);
	puts("\n");
	if(*(uint16_t*)payloadMagicLocation == 0x8B1F)
	{
		CLIENT_LOG("Found GZIP kernel payload !\n");
	}
	else
	{
		CLIENT_LOG("Couldn't find GZIP kernel payload. Aborting.\n");
		for(;;);
	}
	

	
	CLIENT_LOG("Allocating pages for Linux partition... ");
	part->part = allocPage();

	rtospd = allocPage();
	rtossh1 = allocPage();
	rtossh2 = allocPage();
	rtossh3 = allocPage();
	puts("done.\n");
	CLIENT_LOG("\tPartition descriptor : ");	puthex((uint32_t)part->part); puts("\n");
	CLIENT_LOG("\tpd : "); puthex((uint32_t)rtospd); puts("\n");
	
	CLIENT_LOG("Creating Linux partition... ");

	if(createPartition((uint32_t)part->part, (uint32_t)rtospd, (uint32_t)rtossh1, (uint32_t)rtossh2, (uint32_t)rtossh3))
		puts("done.\n");
	else
		goto fail;

	
	#define LINUX_CONFIG_PAGE 0xB00000
	uintptr_t kernel_cmdline = base + 0x228;
	CLIENT_LOG("Filling in kernel command line.\n");
	uintptr_t linux_cfgpage = (uintptr_t)allocPage();
	mapPageWrapper(linux_cfgpage, part->part, (void*)LINUX_CONFIG_PAGE);
	strcpy((char*)linux_cfgpage, "auto");
	*(uint32_t*)kernel_cmdline = 0xFFFFA000;
	puts("\n");
	
	#define LINUX_CONFIG_ZEROPAGE 0xB01000
	CLIENT_LOG("Setting up ZERO page\n");
	uintptr_t linux_zeropage = (uintptr_t)allocPage();
	mapPageWrapper(linux_zeropage, part->part, (void*)LINUX_CONFIG_ZEROPAGE);
	//memcpy((void*)linux_zeropage, base + 0x1f1, 0x268 - 0x1f1);
	memset((void*)linux_zeropage, 0x00000000, 0x1000);
	
	CLIENT_LOG("Mapping Linux partition ... ");
	/* Map the whole FreeRTOS partition */
	for(offset = 0; offset < length; offset+=0x1000){
		//CLIENT_LOG("\t\tmapping partition : ");puthex(base + offset);puts(" - ");	puthex(load_addr + offset);puts("\n");
		if (mapPageWrapper((void*)(base + offset), part->part, (void*)(laddr + offset))){
			CLIENT_LOG("Failed to map page ");
			puthex(base + offset);
			puts(" to ");
			puthex(load_addr + offset);
			puts("\n");
			goto fail;
		}
		CLIENT_LOG("Mapped page ");
		puthex(base + offset);
		puts(" to ");
		puthex(laddr + offset);
		puts("\n");
	}
	puts("done\n");
/*
	CLIENT_LOG("Mapping kernel into partition ... ");
	for (offset = 0x100000; offset <= 0x500000; offset++){
		if (mapPageWrapper((void*)(offset), part->part, (void*)(offset))){
			goto fail;
		}
	}
	puts("done\n");
*/
	

	CLIENT_LOG("Preparing Linux to boot ... ");
	/* map stack for rtos */
	uintptr_t targetStack = (uintptr_t)allocPage();
	if(mapPageWrapper(targetStack, part->part, (void*)0x1200000))
		goto fail;

#ifdef MINIMAL
	/* interrupt stacks for minimal partition*/
	/*if(mapPageWrapper(allocPage(), part->part, (void*)0x8001000)
	|| mapPageWrapper(allocPage(), part->part, (void*)0x8000000))
		goto fail;*/
#endif

#ifdef RTOS
	/* needed by freertos */
	/*if(mapPageWrapper(allocPage(), part->part, (void*)0x760000)
	|| mapPageWrapper(tmp, part->part, (void*)0x75F000))
		goto fail;*/
#endif
	puts("done.\n");
	part->state = PTS_NONE;


	/* prepare vidt */
	part->vidt = (vidt_t*)allocPage();
	puts("vidt at ");puthex((uint32_t)part->vidt);puts("\n");
	part->vidt->vint[0].eip = laddr + 0x80;
	part->vidt->vint[0].esp = 0x1200FFC - sizeof(int_ctx_t);
	part->vidt->flags = 0x1;

	CLIENT_LOG("Preparing fake resume info for Linux...\n");
	int_ctx_t* info = (int_ctx_t*)((uint32_t)(part->vidt) + 0xF0C);
	CLIENT_LOG("VIDT context buffer at "); puthex(info); puts("\n");
	info->regs.esi = LINUX_CONFIG_ZEROPAGE;
	info->regs.ebp = info->regs.edi = info->regs.ebx = 0x0;
	info->cs = 0x1B;
	info->useresp = 0x1200FFC;
	info->ss = 0x23;
	info->eip = laddr + protOffset;
	
	/* map vidt */
	if (mapPageWrapper(part->vidt, part->part, (void*)0xFFFFF000))
		goto fail;
	
	return 0;
fail:
	puts("fail\n");
	return -1;
}
