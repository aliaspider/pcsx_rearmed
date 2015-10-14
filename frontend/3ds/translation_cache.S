#char translation_cache[1 << TARGET_SIZE_2] __attribute__((aligned(4096)));
#
#	.text
#	.comm	translation_cache,4194304,4096


#define TARGET_SIZE_2 21

.section .text
.align	15
.space	0x200000
.align	15
.global translation_cache
.type	translation_cache, %object
.size	translation_cache, 1 << TARGET_SIZE_2
translation_cache:
   .word 0xF346ABC3
   .word 0xBCD3AD69
   .word 0x03FB3A2D
   .word 0x3DF853BC
	.space	(1 << TARGET_SIZE_2) - 0x10

.section .bss
.align	15
.space	0x100000
.align	15
.global translation_cache_w
.type	translation_cache_w, %object
.size	translation_cache_w, 1 << TARGET_SIZE_2
translation_cache_w:
	.space	1 << TARGET_SIZE_2
