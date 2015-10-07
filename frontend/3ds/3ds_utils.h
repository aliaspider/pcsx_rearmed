#ifndef _3DS_UTILS_H
#define _3DS_UTILS_H

void ctr_invalidate_ICache(void);
void ctr_flush_DCache(void);
void ctr_flush_DCache_range(void* start, void* end);

void ctr_flush_invalidate_cache(void);

int ctr_svchack_init(void);

#include <stdio.h>
#define DEBUG_HOLD() do{printf("%s@%s:%d.\n",__FUNCTION__, __FILE__, __LINE__);fflush(stdout);wait_for_input();}while(0)

void wait_for_input(void);

#endif // _3DS_UTILS_H
