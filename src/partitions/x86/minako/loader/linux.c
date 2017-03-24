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

#include "minako.h"
#include "linux.h"
#include "bootparam.h"

#define LINUX_LOG(a) puts("[LOADER:LINUX] "); puts(a)


int LinuxBootstrap(uint32_t base, uint32_t length, uint32_t load_addr, task_t* part)
{
	uint32_t offset, i;
	void *rtospd, *rtossh1, *rtossh2, *rtossh3;
	LINUX_LOG("Parsing Linux header... \n");
	
	/* First check legacy loader header to ensure we are coherent */
	linux_header_t* linHeader = (linux_header_t*)(base + LINUX_HEADER_OFFSET);
	LINUX_LOG("Linux header should be at "); puthex((uint32_t)linHeader); puts("\n");
	if(linHeader->boot_flag == 0xAA55)
	{
		LINUX_LOG("Kernel header is correct!\n");
	}
	else
	{
		LINUX_LOG("Kernel header is incorrect. Aborting.\n");
		goto fail;
	}
	
	/* Dump Linux image version */
	LINUX_LOG("Preparing to load Linux version ");
	char* linuxVersionString = (char*)(base + linHeader->kernel_version + 0x200 /* Header sector size */);
	puts(linuxVersionString);
	puts("\n");
	
	/* Find Linux image protected mode entry-point */
	uint32_t protOffset = (linHeader->setup_sects + 1) * 512;
	LINUX_LOG("\t-> Protected mode code offset is "); puthex(protOffset); puts("\n");
	
	/* Update code32_start field with new entrypoint */
	linHeader->code32_start = 0x800000 + protOffset;
	
	/* Find GZIP kernel payload */
	uintptr_t payloadMagicLocation = base + protOffset + linHeader->payload_offset;
	if(*(uint16_t*)payloadMagicLocation == GZIP_MAGIC)
	{
		LINUX_LOG("Found GZIP kernel payload !\n");
	}
	else
	{
		LINUX_LOG("Couldn't find GZIP kernel payload. Aborting.\n");
		for(;;);
	}
	
	/* Fill bootloader type. We have an unknown type (yet ;p) */
	linHeader->type_of_loader = 0xFF;

	/* Specify to Linux that we REALLY don't want it to mess with our segments */
	linHeader->loadflags |= 0x40;
	
	/* We don't have a ramdisk (yet ;p) */
	linHeader->ramdisk_image = linHeader->ramdisk_size = 0x0;
	
	/* We have an odd way of loading Linux, but let's assume our setup heap ends somewhere - it shouldn't be used anyway */
	linHeader->heap_end_ptr = 0x2000;

	/* Now prepare our Linux partition */
	uint32_t laddr = base;
	
	LINUX_LOG("Allocating pages for Linux partition... ");
	part->part = allocPage();
	rtospd = allocPage();
	rtossh1 = allocPage();
	rtossh2 = allocPage();
	rtossh3 = allocPage();
	puts("done.\n");
	LINUX_LOG("\tPartition descriptor : ");	puthex((uint32_t)part->part); puts("\n");
	LINUX_LOG("\tpd : "); puthex((uint32_t)rtospd); puts("\n");
	
	LINUX_LOG("Creating Linux partition... ");

	if(createPartition((uint32_t)part->part, (uint32_t)rtospd, (uint32_t)rtossh1, (uint32_t)rtossh2, (uint32_t)rtossh3))
		puts("done.\n");
	else
		goto fail;
	
	/* Map additional data : boot parameters "zero" page */
	/* uintptr_t kernel_cmdline = base + 0x228;
	LINUX_LOG("Filling in kernel command line.\n");
	uintptr_t linux_cfgpage = (uintptr_t)allocPage();
	mapPageWrapper((void*)linux_cfgpage, part->part, (void*)LINUX_CONFIG_PAGE);
	strcpy((char*)linux_cfgpage, "auto");
	*(uint32_t*)kernel_cmdline = 0xFFFFA000;
	puts("\n"); */

	/* Now, map the whole kernel thing */
	LINUX_LOG("Mapping Linux partition... ");
	for(offset = 0; offset < length; offset+=0x1000){
		if (mapPageWrapper((void*)(base + offset), part->part, (void*)(laddr + offset)))
			goto fail;
		if(offset % 0x10000 == 0)
			puts(".");
	}
	
	uint32_t lastPage = laddr + offset;
	puts("done, mapped until "); puthex(lastPage); puts("\n");
	
	/* Now map an early-boot stack */
	LINUX_LOG("Giving a whole bunch of memory for Linux to toy with...");
	uint32_t tmp, addr;
	for(addr = lastPage; addr <= 0x1200000; addr += 0x1000)
	{
		uintptr_t tmp = (uintptr_t)allocPage();
		if(mapPageWrapper((void*)tmp, part->part, (void*)addr)) {
			LINUX_LOG("Failed at "); puthex(addr); puts("\n");
			goto fail;
		}
		if(addr % 0x100000 == 0)
			puts(".");
	}
	puts("\n");
	LINUX_LOG("At this point, we should prepare a proper virtual memory environment for Linux. For now, we'll adopt the \"Touche pas à ça p'tit con\" strategy.\n");

	/* At this point we're good, we just need to setup the zero page */
	#define LINUX_CONFIG_ZEROPAGE 0x1201000
	LINUX_LOG("Setting up zero page...\n");
	uintptr_t linux_zeropage = (uintptr_t)allocPage();
	mapPageWrapper((void*)linux_zeropage, part->part, (void*)LINUX_CONFIG_ZEROPAGE);
	memset((void*)linux_zeropage, 0x00000000, 0x1000);
	
	/* Copy the legacy boot info into the zero page */
	struct boot_params* zeroParams = (struct boot_params*)linux_zeropage;
	memcpy(&(zeroParams->hdr), linHeader, sizeof(linux_header_t));
	
	/* Check for correctness of magic 0xAA55 into the new header */
	if(zeroParams->hdr.boot_flag == 0xAA55)
	{
		LINUX_LOG("Kernel header is still correct!\n");
	}
	else
	{
		LINUX_LOG("Kernel header is incorrect. Aborting.\n");
		goto fail;
	}
	
	/* Fill-in some video device information */
	zeroParams->screen_info.orig_x = 0;
	zeroParams->screen_info.orig_y = 0;
	zeroParams->screen_info.orig_video_cols = 80;
	zeroParams->screen_info.orig_video_lines = 25;
	
	/* At this point, we have our zero page at LINUX_CONFIG_ZEROPAGE into the Linux partition */

	/* We'll also allocate some stuff for the Linux decompressor to store pointers to the early-boot console. */
	#define LINUX_LOADER_CONSOLE 0x1202000
	LINUX_LOG("Allocating early console API...\n");
	uintptr_t linux_console = (uintptr_t)allocPage();
	mapPageWrapper((void*)linux_console, part->part, (void*)LINUX_LOADER_CONSOLE);
	memset((void*)linux_console, 0x00000000, 0x1000);
	
	/* Now map an early-boot stack */
	LINUX_LOG("Giving a whole bunch of memory for Linux to toy with...");
	for(addr = 0x1203000; addr <= 0x10000000; addr += 0x1000)
	{
		uintptr_t tmp = (uintptr_t)allocPage();
		if(mapPageWrapper((void*)tmp, part->part, (void*)addr)) {
			LINUX_LOG("Failed at "); puthex(addr); puts("\n");
			goto fail;
		}
		if(addr % 0x1000000 == 0)
			puts(".");
	}
	
	/* We'll put the early-boot stack at the end of this bunch of memory, so tmp holds the value of the last allocated page, that is, our stack */
	uint32_t targetStack = tmp;
	puts("done.\n");

	/* Prepare the virtual interrupt vector to allow Linux to boot. */
	part->vidt = (vidt_t*)allocPage();
	puts("vidt at ");puthex((uint32_t)part->vidt);puts("\n");
	part->vidt->vint[0].eip = laddr + 0x80;
	part->vidt->vint[0].esp = 0xC00FFC - sizeof(int_ctx_t);
	part->vidt->flags = 0x1;

	/* The boot process is a bit tricky, but funny, come to think of it */
	LINUX_LOG("Preparing fake resume info for Linux...\n");
	int_ctx_t* info = (int_ctx_t*)((uint32_t)(part->vidt) + 0xF0C);
	LINUX_LOG("VIDT context buffer at "); puthex(info); puts("\n");
	info->regs.esi = LINUX_CONFIG_ZEROPAGE;
	info->regs.ebp = info->regs.edi = info->regs.ebx = 0x0;
	info->cs = 0x1B;
	info->useresp = 0xC00FFC;
	info->ss = 0x23;
	info->eip = laddr + protOffset;
	
	/* Finally, map VIDT */
	if (mapPageWrapper(part->vidt, part->part, (void*)0xFFFFF000))
		goto fail;
	
	return 0;
fail:
	puts("fail\n");
	return -1;
}
