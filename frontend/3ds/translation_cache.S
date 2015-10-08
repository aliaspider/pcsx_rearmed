
.text
.align	15
.global translation_cache
.global _translation_cache
.type	translation_cache, %object
.size	translation_cache, 8*1024*1024
translation_cache:
_translation_cache:
	.space	8*1024*1024


//.bss
//.align	15
//.global translation_cache_w
//.global _translation_cache_w
//.type	translation_cache_w, %object
//.size	translation_cache_w, 8*1024*1024
//translation_cache_w:
//_translation_cache_w:
//.space	8*1024*1024
