#ifndef _ASM_X86_BOOTPARAM_H
#define _ASM_X86_BOOTPARAM_H

/* setup_data types */
#define SETUP_NONE			0
#define SETUP_E820_EXT			1
#define SETUP_DTB			2
#define SETUP_PCI			3
#define SETUP_EFI			4
#define SETUP_APPLE_PROPERTIES		5

/* ram_size flags */
#define RAMDISK_IMAGE_START_MASK	0x07FF
#define RAMDISK_PROMPT_FLAG		0x8000
#define RAMDISK_LOAD_FLAG		0x4000

/* loadflags */
#define LOADED_HIGH	(1<<0)
#define KASLR_FLAG	(1<<1)
#define QUIET_FLAG	(1<<5)
#define KEEP_SEGMENTS	(1<<6)
#define CAN_USE_HEAP	(1<<7)

/* xloadflags */
#define XLF_KERNEL_64			(1<<0)
#define XLF_CAN_BE_LOADED_ABOVE_4G	(1<<1)
#define XLF_EFI_HANDOVER_32		(1<<2)
#define XLF_EFI_HANDOVER_64		(1<<3)
#define XLF_EFI_KEXEC			(1<<4)

#ifndef __ASSEMBLY__

#include <stdint.h>

/* Taken from Linux Kernel source */
#include "screen_info.h"
#include "apm_bios.h"
#include "edd.h"
#include "e820.h"
#include "ist.h"
#include "edid.h"

/* Our Linux header (how naïve was I) */
#include "linux.h"

/* extensible setup data list node */
struct setup_data {
	uint64_t next;
	uint32_t type;
	uint32_t len;
	uint8_t data[0];
};

struct sys_desc_table {
	uint16_t length;
	uint8_t  table[14];
};

/* Gleaned from OFW's set-parameters in cpu/x86/pc/linux.fth */
struct olpc_ofw_header {
	uint32_t ofw_magic;	/* OFW signature */
	uint32_t ofw_version;
	uint32_t cif_handler;	/* callback into OFW */
	uint32_t irq_desc_table;
} __attribute__((packed));

struct efi_info {
	uint32_t efi_loader_signature;
	uint32_t efi_systab;
	uint32_t efi_memdesc_size;
	uint32_t efi_memdesc_version;
	uint32_t efi_memmap;
	uint32_t efi_memmap_size;
	uint32_t efi_systab_hi;
	uint32_t efi_memmap_hi;
};

/* The so-called "zeropage" */
struct boot_params {
	struct screen_info screen_info;			/* 0x000 */
	struct apm_bios_info apm_bios_info;		/* 0x040 */
	uint8_t  _pad2[4];						/* 0x054 */
	uint64_t  tboot_addr;					/* 0x058 */
	struct ist_info ist_info;				/* 0x060 */
	uint8_t  _pad3[16];						/* 0x070 */
	uint8_t  hd0_info[16];					/* obsolete! */		/* 0x080 */
	uint8_t  hd1_info[16];					/* obsolete! */		/* 0x090 */
	struct sys_desc_table sys_desc_table;	/* obsolete! */	/* 0x0a0 */
	struct olpc_ofw_header olpc_ofw_header;	/* 0x0b0 */
	uint32_t ext_ramdisk_image;				/* 0x0c0 */
	uint32_t ext_ramdisk_size;				/* 0x0c4 */
	uint32_t ext_cmd_line_ptr;				/* 0x0c8 */
	uint8_t  _pad4[116];					/* 0x0cc */
	struct edid_info edid_info;				/* 0x140 */
	struct efi_info efi_info;				/* 0x1c0 */
	uint32_t alt_mem_k;						/* 0x1e0 */
	uint32_t scratch;						/* Scratch field! */	/* 0x1e4 */
	uint8_t  e820_entries;					/* 0x1e8 */
	uint8_t  eddbuf_entries;				/* 0x1e9 */
	uint8_t  edd_mbr_sig_buf_entries;		/* 0x1ea */
	uint8_t  kbd_status;					/* 0x1eb */
	uint8_t  _pad5[3];						/* 0x1ec */
	/*
	 * The sentinel is set to a nonzero value (0xff) in header.S.
	 *
	 * A bootloader is supposed to only take setup_header and put
	 * it into a clean boot_params buffer. If it turns out that
	 * it is clumsy or too generous with the buffer, it most
	 * probably will pick up the sentinel variable too. The fact
	 * that this variable then is still 0xff will let kernel
	 * know that some variables in boot_params are invalid and
	 * kernel should zero out certain portions of boot_params.
	 */
	uint8_t  sentinel;						/* 0x1ef */
	uint8_t  _pad6[1];						/* 0x1f0 */
	linux_header_t hdr;						/* setup header */	/* 0x1f1 */
	uint8_t  _pad7[0x290-0x1f1-sizeof(linux_header_t)];
	uint32_t edd_mbr_sig_buffer[EDD_MBR_SIG_MAX];	/* 0x290 */
	struct e820entry e820_map[E820MAX];		/* 0x2d0 */
	uint8_t  _pad8[48];						/* 0xcd0 */
	struct edd_info eddbuf[EDDMAXNR];		/* 0xd00 */
	uint8_t  _pad9[276];					/* 0xeec */
} __attribute__((packed));

/**
 * enum x86_hardware_subarch - x86 hardware subarchitecture
 *
 * The x86 hardware_subarch and hardware_subarch_data were added as of the x86
 * boot protocol 2.07 to help distinguish and support custom x86 boot
 * sequences. This enum represents accepted values for the x86
 * hardware_subarch.  Custom x86 boot sequences (not X86_SUBARCH_PC) do not
 * have or simply *cannot* make use of natural stubs like BIOS or EFI, the
 * hardware_subarch can be used on the Linux entry path to revector to a
 * subarchitecture stub when needed. This subarchitecture stub can be used to
 * set up Linux boot parameters or for special care to account for nonstandard
 * handling of page tables.
 *
 * These enums should only ever be used by x86 code, and the code that uses
 * it should be well contained and compartamentalized.
 *
 * KVM and Xen HVM do not have a subarch as these are expected to follow
 * standard x86 boot entries. If there is a genuine need for "hypervisor" type
 * that should be considered separately in the future. Future guest types
 * should seriously consider working with standard x86 boot stubs such as
 * the BIOS or EFI boot stubs.
 *
 * WARNING: this enum is only used for legacy hacks, for platform features that
 *	    are not easily enumerated or discoverable. You should not ever use
 *	    this for new features.
 *
 * @X86_SUBARCH_PC: Should be used if the hardware is enumerable using standard
 *	PC mechanisms (PCI, ACPI) and doesn't need a special boot flow.
 * @X86_SUBARCH_LGUEST: Used for x86 hypervisor demo, lguest
 * @X86_SUBARCH_XEN: Used for Xen guest types which follow the PV boot path,
 * 	which start at asm startup_xen() entry point and later jump to the C
 * 	xen_start_kernel() entry point. Both domU and dom0 type of guests are
 * 	currently supportd through this PV boot path.
 * @X86_SUBARCH_INTEL_MID: Used for Intel MID (Mobile Internet Device) platform
 *	systems which do not have the PCI legacy interfaces.
 * @X86_SUBARCH_CE4100: Used for Intel CE media processor (CE4100) SoC for
 * 	for settop boxes and media devices, the use of a subarch for CE4100
 * 	is more of a hack...
 */
enum x86_hardware_subarch {
	X86_SUBARCH_PC = 0,
	X86_SUBARCH_LGUEST,
	X86_SUBARCH_XEN,
	X86_SUBARCH_INTEL_MID,
	X86_SUBARCH_CE4100,
	X86_NR_SUBARCHS,
};

#endif /* __ASSEMBLY__ */

#endif /* _ASM_X86_BOOTPARAM_H */
