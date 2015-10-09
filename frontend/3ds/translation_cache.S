#char translation_cache[1 << 22] __attribute__((aligned(4096)));

.bss
.align	12
.global translation_cache
.global _translation_cache
.type	translation_cache, %object
.size	translation_cache, 0x400000
translation_cache:
_translation_cache:
	.space	0x400000

