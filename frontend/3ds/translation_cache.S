
   .text
	.align	12
	.global translation_cache
   .global _translation_cache
	.type	translation_cache, %object
	.size	translation_cache, 4*1024*1024
translation_cache:
_translation_cache:
	.space	4*1024*1024
