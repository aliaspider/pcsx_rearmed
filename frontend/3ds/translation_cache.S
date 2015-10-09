#char translation_cache[1 << 22] __attribute__((aligned(4096)));
#
#	.text
#	.comm	translation_cache,4194304,4096



.section .text
.align	15
.space	0x100000
.global translation_cache
#.global _translation_cache
.type	translation_cache, %object
.size	translation_cache, 0x400000
translation_cache:
#_translation_cache:
	.space	0x400000
