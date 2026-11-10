/* ARM relocation numbers; the arm reader is compiled but never entered here. */
#ifndef SHIM_ARM_RELOC_H
#define SHIM_ARM_RELOC_H
enum reloc_type_arm {
	ARM_RELOC_VANILLA, ARM_RELOC_PAIR, ARM_RELOC_SECTDIFF, ARM_RELOC_LOCAL_SECTDIFF, ARM_RELOC_PB_LA_PTR,
	ARM_RELOC_BR24, ARM_THUMB_RELOC_BR22, ARM_THUMB_32BIT_BRANCH
};
#endif
