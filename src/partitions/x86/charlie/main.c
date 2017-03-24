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

/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Multiplexer test file
 *
 *        Version:  1.0
 *        Created:  15/07/2015 15:30:12
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Quentin Bergougnoux (qb), quentin.bergougnoux@gmail.com
 *        Company:  Université Lille 1
 *
 * =====================================================================================
 */
#include <stdint.h>
#include <pip/fpinfo.h>
#include <pip/io.h>
#include <pip/paging.h>
#include <pip/vidt.h>
#include <pip/api.h>

#include "charlie.h"
#include "rtl8139.h"
#include "freertos.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

static uint32_t sched_state = 0;
static uint32_t curpart = 0;
static uint32_t enableScheduling = 0;
uint32_t jiffies = 0;

typedef struct ctx_buf_s {
	uint32_t used;
	ctx_t ctx;
} ctx_buf_t ;

ctx_buf_t *ctx_list = NULL;
#define CTX_MAX (PGSIZE / sizeof(*ctx_list))

task_t partitions[PARTITION_COUNT];

void * memcpy ( void * destination, const void * source, unsigned num )
{
	uint32_t i;
	uint8_t *src;
	uint8_t *dest;
	src = (uint8_t*) source;
	dest = (uint8_t*) destination;
	for(i=0; i<num; i++)
	{
		*dest = *src;
		dest++;
		src++;
	}
	return destination;
}
void * memset( void * ptr, int value, unsigned num )
{
	char *temp = (char*) ptr;
	for(; num != 0; num--) *temp++ = value;
	return ptr;
}

void initCtxAlloc(void)
{
	int i;
	ctx_list = allocPage();

	if (!ctx_list) {
		ch_log("no mem in charlie..\n");
		for(;;);
	}
	for (i=0;i<CTX_MAX;i++)
		ctx_list[i].used = 0;
}
ctx_t *allocCtx(void)
{
	int i;
	for (i=0; i< CTX_MAX; i++){
		if (!ctx_list[i].used){
			ctx_list[i].used = 1;
			return &ctx_list[i].ctx;
		}
	}
	return NULL;
}
ctx_t *freeCtx(ctx_t *c)
{
	ctx_buf_t *buf = (ctx_buf_t*)(((void*)c)-4);
	buf->used = 0;
}

task_t* findChild(uint32_t part)
{
	unsigned i;

	for (i=0;i<PARTITION_COUNT;i++)
	{
		if (partitions[i].part == (void*)part)
			return &partitions[i];
	}
	return NULL;
}

void pushChildContext(task_t *task, uint32_t idx)
{
	vidt_int_ctx_t *c = &((&task->vidt->intCtx)[idx]);
	ctx_t *sctx = allocCtx();
	/* init */
	memcpy(&sctx->ctx, c, sizeof(*c));
	sctx->index = idx;

	/* link */
	sctx->next = task->ctx_list;
	task->ctx_list = sctx;

	/* invalidate */
	c->valid = 0;
}

int popChildContext(task_t *task, uint32_t idx)
{
	vidt_int_ctx_t *c = &((&task->vidt->intCtx)[idx]);
	ctx_t *p=NULL, *sctx = task->ctx_list;

	while (sctx){
		if (sctx->index == idx){
			/* unlink */
			if (p){
				p->next = sctx->next;
			}
			else{
				task->ctx_list = sctx->next;
			}
			memcpy(c, &sctx->ctx, sizeof(*c));
			freeCtx(sctx);
			return 0;
		}
		p = sctx;
		sctx = sctx->next;
	}
	return -1;
}

#define PANIC() {vcli(); for(;;);}
#define MIN(a,b) ((a)<(b) ? (a) : (b))

extern void* _partition1, *_epartition1;
extern void* _partition2, *_epartition2;
extern void* _partition3, *_epartition3;
extern void* _partition4, *_epartition4;
extern void* _partition5, *_epartition5;
extern void* _partition6, *_epartition6;
extern void* _partition7, *_epartition7;
extern void* _partition8, *_epartition8;

static const struct {uint32_t start, end;} rtoses[] = {
	{(uint32_t)&_partition1, (uint32_t)&_epartition1},
#if (PARTITION_COUNT > 1)
	{(uint32_t)&_partition2, (uint32_t)&_epartition2},
#if (PARTITION_COUNT > 2)
	{(uint32_t)&_partition3, (uint32_t)&_epartition3},
#if (PARTITION_COUNT > 3)
	{(uint32_t)&_partition4, (uint32_t)&_epartition4},
#if (PARTITION_COUNT > 4)
	{(uint32_t)&_partition5, (uint32_t)&_epartition5},
#if (PARTITION_COUNT > 5)
	{(uint32_t)&_partition6, (uint32_t)&_epartition6},
#if (PARTITION_COUNT > 6)
	{(uint32_t)&_partition7, (uint32_t)&_epartition7},
#if (PARTITION_COUNT > 7)
	{(uint32_t)&_partition8, (uint32_t)&_epartition8},
#endif
#endif
#endif
#endif
#endif
#endif
#endif
};

/**
 * Page fault irq handler
 */
INTERRUPT_HANDLER(pfAsm, pfHandler)
	task_t* child = findChild(caller);
	ch_log("Page fault at ");
	puthex(data1);
	puts("\n");
	if(child)
	{
		ch_log("Child partition ");
		puthex(caller);
		puts(" triggered a Page Fault, this is bad.\n");
	} else {
		ch_log("Dammit, I triggered a Page Fault. Bad Dobby\n");
		PANIC();
	}
END_OF_INTERRUPT

/**
 * General protection failure irq handler
 */
INTERRUPT_HANDLER(gpfAsm, gpfHandler)
	task_t* child = findChild(caller);

	if(child)
	{
		ch_log("Child partition ");
		puthex(caller);
		puts(" triggered a General Protection Fault, this is bad.\n");
	} else {
		ch_log("Dammit, I triggered a General Protection Fault. Bad Dobby\n");
	}
	PANIC();
END_OF_INTERRUPT

/**
 * Unused irq handler fornow
 */
INTERRUPT_HANDLER(hwAccessAsm, hwAccess)
    uint32_t ret = 0;
    task_t *child = findChild(caller);
    if (!child){
        ch_log("Fault from invalid child ?!\n");
	PANIC();
    }
#if 0
    ch_log("hwAccessAsm caught ("); puthex(data1);puts(" - " ); puthex(data2); puts(")\n");
    ch_log("\tfrom child : ");puthex(caller);puts("\n");
#endif
    /* Perform IO */
    if(data1 & 0xF0000000)
    {
    	/* inb: get result and fix child context */
    	ret = (uint32_t)inb(data1 &~0xF0000000);
	child->vidt->isrCtx.regs.eax = ret;
    }
    else
    {
    	outb(data1, data2);
    }
    resume(caller, 1); /* Resume partition */
END_OF_INTERRUPT


static uint32_t message = 0;
INTERRUPT_HANDLER(notifyHdlrAsm, notifyHdlr)
    task_t *child = findChild(caller);
    if (!child){
        ch_log("Fault from invalid child ?!\n");
        PANIC();
    }
    ch_log ("Being notified by child ");puthex(caller);puts("\n");
    /* resume NOTIF_PARENT_CTX of child */
    child->vidt->notifyParentCtx.regs.eax = message;
    message += 12;
    resume((uint32_t)child->part, 3);
END_OF_INTERRUPT

void mini_yield(void)
{
    task_t *part = &partitions[0];
    jiffies++;

    if (part->vidt->isrCtx.valid){
        ch_log("wtf, unhandled child's isr context\n");
	for(;;);
    }
    else{
	if (!part->vidt->intCtx.valid)
	{
		// ch_log("no child ctx, popin\n");
		if (popChildContext(part, 0)){
			ch_log("no child ctx, nothing to do\n");
			vsti();
			for(;;);
		}
	}
	resume((uint32_t)part->part, 0);
    }
}

static timer_switch = 0;
void timerHandler(void)
{
	task_t *part = &partitions[0];
	timer_switch ^= 1;
	if (timer_switch && !(part->vidt->flags&1))
	{
		/* dispatch timer it to child */
		if (part->vidt->intCtx.valid){
			// ch_log("pushin child ctx\n");
			pushChildContext(part, 0);
		}
		dispatch((uint32_t)part->part, 33, 0,0);
		ch_log("dispatch returned!\n");
		/* (no) PANIC() */
	}
	mini_yield();
}

void resumeHandler(void)
{
	mini_yield();
}

/*
 * Prepares the fake interrupt vector to receive new interrupts
 */
void initInterrupts()
{
    	registerInterrupt(0,(void*) resumeHandler, (void*)0x2010000);
	//registerInterrupt(33, &timerAsm, (uint32_t*)0x2020000); // We can use the same stack for both interrupts, or choose different stacks, let's play a bit
	registerInterrupt(33, timerHandler, (uint32_t*)0x2020000); // We can use the same stack for both interrupts, or choose different stacks, let's play a bit
	registerInterrupt(42, notifyHdlrAsm, (uint32_t*)0x2020000);
	// registerInterrupt(34, &keyboardAsm, (uint32_t*)0x2A00000); // We can use the same stack for both interrupts, or choose different stacks, let's play a bit
	registerInterrupt(14, &gpfAsm, (uint32_t*)0x2030000); /* General Protection Fault */
	registerInterrupt(15, &pfAsm, (uint32_t*)0x2040000); /* Page Fault */
	registerInterrupt(64, &hwAccessAsm, (uint32_t*)0x2050000); /* Hardware access request */

	// registerInterrupt(87, &testIrqAsm, (uint32_t*)0x2000000);
	return;
}

void parse_bootinfo(pip_fpinfo* bootinfo)
{
	if(bootinfo->magic == FPINFO_MAGIC)
	ch_log("\tBootinfo seems to be correct.\n");
	else {
	ch_log("\tBootinfo is invalid. Aborting.\n");
	PANIC();
	}

	ch_log("\tAvailable memory starts at ");
	puthex((uint32_t)bootinfo->membegin);
	puts(" and ends at ");
	puthex((uint32_t)bootinfo->memend);
	puts("\n");

	ch_log("\tPip revision ");
	puts(bootinfo->revision);
	puts("\n");

	return;
}

void main(pip_fpinfo* bootinfo)
{
	uint32_t i;

	puts(bootinfo->revision);
	ch_log("FreeRTOS image info: \n");
	parse_bootinfo(bootinfo);

	ch_log("Initializing paging... ");
	initPaging((void*)bootinfo->membegin, (void*)bootinfo->memend);
	puts("done.\n");

	ch_log("Initializing interrupts... ");
	initInterrupts();
	puts("done.\n");

	initCtxAlloc();

	#if 0
	ch_log("Initializing hardware...\n");
	//RTL8139_init();
	#endif

	/* Fill in the partition list with RTOS bootstrap */
	for (i=0;i<PARTITION_COUNT;i++)
	{
		uint32_t sz = rtoses[i].end - rtoses[i].start;
		/* init task structure */
		memset(&partitions[i], 0, sizeof(partitions[i]));

		ch_log("FreeRTOS image info: \n");
		ch_log("\tImage address : "); puthex(rtoses[i].start); puts("\n");
		ch_log("\tImage size : "); puthex(sz); puts(" bytes\n");
		if (FreeRTOSBootstrap(rtoses[i].start, sz, 0x700000, &partitions[i])){
			ch_log("FreeRTOS image failed\n");
			PANIC();
		}
	}

	ch_log("I'm done. I'll be booting the system in a few seconds.\n");

	/* discard main context.. */
	dispatch((uint32_t)partitions[0].part, 0, 0, 0);
	PANIC();
}
