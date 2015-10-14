#char translation_cache[1 << 22] __attribute__((aligned(4096)));
#
#	.text
#	.comm	translation_cache,4194304,4096



.section .text
.align	15
.space	0x200000
.align	15
.global translation_cache
.type	translation_cache, %object
.size	translation_cache, 0x200000
translation_cache:
   .word 0xF346ABC3
   .word 0xBCD3AD69
   .word 0x03FB3A2D
   .word 0x3DF853BC
	.space	0x200000 - 0x10

.section .bss
.align	15
.space	0x100000
.align	15
.global translation_cache_w
.type	translation_cache_w, %object
.size	translation_cache_w, 0x200000
translation_cache_w:
	.space	0x200000
