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

#include "server.h"
#include "client.h"

#ifndef NULL
#define NULL ((void*)0)
#endif

static uint32_t message = 0;
INTERRUPT_HANDLER(notifyHdlrAsm, notifyHdlr)
    ch_log ("Being notified by child ");puthex(caller);puts("\n");
    //resume((uint32_t)child->part, 3);
END_OF_INTERRUPT

INTERRUPT_HANDLER(intHdlrAsm, intHdlr)
    ch_log("Caught interrupt call from "); puthex(caller); puts("\n");
END_OF_INTERRUPT

/*
 * Prepares the fake interrupt vector to receive new interrupts
 */
void initInterrupts()
{
    registerInterrupt(0,(void*) resumeHandler, (void*)0x2010000);
	registerInterrupt(42, notifyHdlrAsm, (uint32_t*)0x2020000);
    registerInterrupt(0x87, intHdlrAsm, (uint32_t*)0x2020000);
	return;
}

void main(pip_fpinfo* bootinfo)
{
	uint32_t i;
    ch_log("Initializing interrupts\n");
    initInterrupts();
    ch_log("Done\n");
    ch_log("Pseudo-hypershell client\n");
    vsti();
    while(1)
    {
        ch_log("Sending PING\n");
        notify(0, 0x80);
    }
    for(;;);
}
