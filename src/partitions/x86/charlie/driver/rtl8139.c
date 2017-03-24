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
#include <pip/io.h>
#include <pip/debug.h>
#include <pip/paging.h>
#include <pip/vidt.h>

#include "rtl8139.h"

void RTL8139_init()
{
	RTL8139_LOG("Initializing Realtek RTL8139 network adapter...\n");
	char i;
	for (i = 0; i < 6; i++)
	{
		RTL8139_macaddr[i] = inb(RTL8139_IOBASE + i);
	}
	RTL8139_LOG("\tMac address: ");
	for (i = 0; i < 6; i++)
	{
		puthex((uint8_t)RTL8139_macaddr[i]);
		puts(":");
	}
	
	puts("\n");
	RTL8139_LOG("Powering on the chip...\n");
	outb(RTL8139_IOBASE + 0x52, 0x0);
	RTL8139_LOG("Resetting chip...\n");
	outb(RTL8139_IOBASE + 0x37, 0x10); /* Set the Reset bit (0x10) to the Command Register (0x37) */
	while(inb(RTL8139_IOBASE + 0x37) & 0x10); /* Wait until the chipset is ready */
	
	RTL8139_LOG("Configuring adapter...\n");
	outb(RTL8139_IOBASE + 0x50, 0xC0); /* Unlock config registers */
	outb(RTL8139_IOBASE + 0x37, 0x0C); /* Enable Rx/Tx in the Command register */
	outl(RTL8139_IOBASE + 0x40, (4 << 8)); /* Setup DMA burst */
	RTL8139_LOG("Setting up descriptors...\n");
	uintptr_t rxbuffer = (uintptr_t)allocPage();
	outl(RTL8139_IOBASE + 0x30, (uintptr_t)rxbuffer); /* Set the RX buffer address */
	outl(RTL8139_IOBASE + 0x44, 0xF | (1 << 7)); // (1 << 7) is the WRAP bit, 0xf is AB+AM+APM+AAP : receive EVERYTHING
	uint32_t bufi;
	for(bufi = 0; bufi < 4; bufi++)
	{
		uintptr_t buf = (uintptr_t)allocPage();
		uint32_t tmpi;
		for(tmpi = 0; tmpi < 130; tmpi++)
		{
			char* target = (char*)((uint32_t)buf + tmpi * sizeof(char));
			*target = pingpkt[tmpi];
		}
		outaddrl(RTL8139_IOBASE + 0x4 * bufi + RTL8139_TXBUFFER, (uint32_t)buf);
		RTL8139_LOG("Registered page ");
		puthex(buf);
		puts(" to RTL8139 buffer ");
		putdec(bufi);
		puts("\n");
	}
	
	RTL8139_LOG("Enabling RX/TX...\n");
	outw(RTL8139_IOBASE + 0x40, (4 << 8)); /* TX_DMA_BURST */
	outw(RTL8139_IOBASE + 0x3C, 0x0005); /* Enable Rx/Tx interrupts */
	outb(RTL8139_IOBASE + 0x50, 0x00); /* Lock config registers */
	outb(RTL8139_IOBASE + 0x37, 0x0C); /* Enable Rx/Tx in the Command register */
	RTL8139_LOG("Done. Sending PIPHELLO packet..\n");
	uint32_t* piphello = (uint32_t*)allocPage();
	struct eth_header* hdr = (struct eth_header*)piphello;
	uint32_t tmp;
	for(tmp=0; tmp<6; tmp++)
	{
		hdr->mac_src[tmp] = RTL8139_macaddr[tmp];
		hdr->mac_dst[tmp] = 0xFF;
	}
	hdr->eth_type[0] = 0x08;
	hdr->eth_type[1] = 0x00;
	
	uint32_t* payload = (uint32_t*)&(hdr->payload);
	*payload = 0xCAFECAFE;
	
	RTL8139_sendpacket(0, 130 * sizeof(char));
	RTL8139_LOG("Done.\n");
}

void RTL8139_sendpacket(uint32_t bufferid, uintptr_t length)
{
	RTL8139_LOG("Sending packet\n");
	outaddrl(RTL8139_IOBASE + 0x4 * bufferid + RTL8139_TXBUFFER, (uint32_t)pingpkt);
	
	/* 60 bytes is the minimal packet size... */
	uint32_t len = length;
	if(len < 60)
		len = 60;
	
	outl(RTL8139_IOBASE + 0x4 * bufferid + RTL8139_TXSTATUS, len | ((RTL8139_TXTHRESH << 11) & 0x003f0000));
	/* Now the chipset should send the packet... */
	
	// outb(RTL8139_IOBASE + 0x38, 0x40); /* Set-up TX poll */
	while(!(inl(RTL8139_IOBASE + RTL8139_TXSTATUS) & RTL8139_TXTOK) == RTL8139_TXTOK)
	{
		RTL8139_LOG("Did not receive TOK, waiting\n");
	}
	/* Wait for packet send (check ownership bit) */
	return;
}
