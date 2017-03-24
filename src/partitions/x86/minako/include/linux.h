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

#ifndef __LINUX_LOADER__
#define __LINUX_LOADER__
#include <minako.h>

/**
 * \struct int_stack_s
 * \brief Stack context from interrupt/exception
 */
typedef struct int_stack_s
{
	pushad_regs_t regs;//!< Interrupt handler saved regs
	uint32_t int_no; //!< Interrupt number
	uint32_t err_code; //!< Interrupt error code
	uint32_t eip; //!< Execution pointer
	uint32_t cs; //!< Code segment
	uint32_t eflags; //!< CPU flags
	/* only present when we're coming from userland */
	uint32_t useresp; //!< User-mode ESP
	uint32_t ss; //!< Stack segment
} int_ctx_t ;

#define LINUX_HEADER_OFFSET	0x1F1
#define GZIP_MAGIC			0x8B1F
#define	LEGACY_MAGIC		0xAA55
struct linux_image_header_s
{
	uint8_t		setup_sects;
	uint16_t	root_flags;
	uint32_t	syssize;
	uint16_t	ram_size;
	uint16_t	vid_mode;
	uint16_t	root_dev;
	uint16_t	boot_flag; /* 0xAA55 */
	uint16_t	jump;
	uint32_t	header; /* "HdrS" */
	uint16_t	version;
	uint32_t	realmode_swtch;
	uint16_t	start_sys_seg;
	uint16_t	kernel_version;
	uint8_t		type_of_loader;
	uint8_t		loadflags;
	uint16_t	setup_move_size;
	uint32_t	code32_start;
	uint32_t	ramdisk_image;
	uint32_t	ramdisk_size;
	uint32_t	bootsect_kludge;
	uint16_t	heap_end_ptr;
	uint8_t		ext_loader_ver;
	uint8_t		ext_loader_type;
	uint32_t	cmd_line_ptr;
	uint32_t	initrd_addr_max;
	uint32_t	kernel_alignment;
	uint8_t		relocatable_kernel;
	uint8_t		min_alignment;
	uint16_t	xloadflags;
	uint32_t	cmdline_size;
	uint32_t	hardware_subarch;
	uint64_t	hardware_subarch_data;
	uint32_t	payload_offset;
	uint32_t	payload_length;
	uint64_t	setup_data;
	uint64_t	pref_address;
	uint32_t	init_size;
	uint32_t	handover_offset;
} __attribute__((packed));

typedef struct linux_image_header_s linux_header_t;

int LinuxBootstrap(uint32_t base, uint32_t length, uint32_t load_addr, task_t*);

#endif
