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

#ifndef __RTL8139__
#define __RTL8139__

#define RTL8139_IOBASE		0xC000
#define RTL8139_LOG(a)		puts("[NET:RTL8139] "); puts(a)
#define RTL8139_DESCRIPTORS 4
#define RTL8139_BUFFERSIZE	0x1FF8

#define RTL8139_TXSTATUS	0x10
#define RTL8139_TXBUFFER	0x20


#define RTL8139_TXTHRESH	256
#define RTL8139_TXFLAGS		((RTL8139_TXTHRESH << 11) & 0x003f0000);
#define RTL8139_TXTOK		0x00004000

char RTL8139_macaddr[6]; // RTL8139 mac address

void RTL8139_init();
void RTL8139_sendpacket(uintptr_t pkt, uintptr_t length);

static const char pingpkt[130] = {0xb4,0xe9,0xb0,0xc8,0xa1,0x80,0x38,0xc9,0x86,0x29,0xb4,0x45,0x08,0x00,0x45,0x00,0x00,0x54,0xc4,0xab,0x00,0x00,0x40,0x01,0x00,0x00,0x86,0xce,0x19,0x4a,0x08,0x08,0x08,0x08,0x08,0x00,0xcf,0x38,0x26,0x08,0x00,0x00,0x57,0x31,0xd7,0xc0,0x00,0x0e,0xe8,0xbb,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37};

struct eth_header {
	uint8_t preamble;
	char mac_dst[6];
	char mac_src[6];
	char eth_type[2];
	char payload;
} __attribute__((packed));

#endif
