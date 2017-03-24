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

#include "freertos.h"
#include "charlie.h"

#define RTOS_LOG(a) puts("[LOADER:FREERTOS] "); puts(a)

int
FreeRTOSBootstrap(uint32_t base, uint32_t length, uint32_t load_addr, task_t* part)
{
	uint32_t offset, i;
	void *rtospd, *rtossh1, *rtossh2, *rtossh3;


	RTOS_LOG("Allocating pages for FreeRTOS partition... ");
	part->part = allocPage();

	rtospd = allocPage();
	rtossh1 = allocPage();
	rtossh2 = allocPage();
	rtossh3 = allocPage();
	puts("done.\n");
	RTOS_LOG("\tPartition descriptor : ");	puthex((uint32_t)part->part); puts("\n");
	RTOS_LOG("\tpd : "); puthex((uint32_t)rtospd); puts("\n");
	
	RTOS_LOG("Creating FreeRTOS partition... ");

	if(createPartition((uint32_t)part->part, (uint32_t)rtospd, (uint32_t)rtossh1, (uint32_t)rtossh2, (uint32_t)rtossh3))
		puts("done.\n");
	else
		goto fail;

	RTOS_LOG("Mapping rtos partition ... ");
	/* Map the whole FreeRTOS partition */
	for(offset = 0; offset < length; offset+=0x1000){
		//RTOS_LOG("\t\tmapping partition : ");puthex(base + offset);puts(" - ");	puthex(load_addr + offset);puts("\n");
		if (mapPageWrapper((void*)(base + offset), part->part, (void*)(load_addr + offset))){
			goto fail;
		}
	}
	puts("done\n");
/*
	RTOS_LOG("Mapping kernel into partition ... ");
	for (offset = 0x100000; offset <= 0x500000; offset++){
		if (mapPageWrapper((void*)(offset), part->part, (void*)(offset))){
			goto fail;
		}
	}
	puts("done\n");
*/
	

	RTOS_LOG("Preparing RTOS to boot ... ");
	/* map stack for rtos */
	if(mapPageWrapper(allocPage(), part->part, (void*)0x79F000))
		goto fail;

#ifdef MINIMAL
	/* interrupt stacks for minimal partition*/
	if(mapPageWrapper(allocPage(), part->part, (void*)0x8001000)
	|| mapPageWrapper(allocPage(), part->part, (void*)0x8000000))
		goto fail;
#endif

#ifdef RTOS
	/* needed by freertos */
	if(mapPageWrapper(allocPage(), part->part, (void*)0x760000)
	|| mapPageWrapper(tmp, part->part, (void*)0x75F000))
		goto fail;
#endif
	puts("done.\n");
	part->state = PTS_NONE;


	/* prepare vidt */
	part->vidt = (vidt_t*)allocPage();
	puts("vidt at ");puthex((uint32_t)part->vidt);puts("\n");
	part->vidt->vint[0].eip = load_addr;
	part->vidt->vint[0].esp = 0x79FFFC;
	part->vidt->flags = 0;

	/* map vidt */
	if (mapPageWrapper(part->vidt, part->part, (void*)0xFFFFF000))
		goto fail;
	
	return 0;
fail:
	puts("fail\n");
	return -1;
}
