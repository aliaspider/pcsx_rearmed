
.text
.align	12
.global translation_cache
.global _translation_cache
.type	translation_cache, %object
.size	translation_cache, 8*1024*1024
translation_cache:
_translation_cache:
	.space	8*1024*1024


@ vim:filetype=armasm
