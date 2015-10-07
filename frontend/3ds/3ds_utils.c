
#include "3ds.h"
#include "3ds_utils.h"
#include <stdlib.h>

typedef s32 (*ctr_callback_type)(void);


static u32 temp_addr;
s32 convert_temp_addr (void)
{
   __asm__ volatile(
   "cpsid aif \n\t"
   "ldr r0, [%0] \n\t"
   "mcr p15, 0, r0, c7, c8, 1 \n\t"
   "mrc p15, 0, r0, c7, c4, 0 \n\t"
   "str r0, [%0] \n\t"
   :
   :"r"(&temp_addr)
   :"r0");
   return 0;
}

u32 get_PA(u32 addr)
{
   temp_addr = addr;
   svcBackdoor(convert_temp_addr);
   temp_addr = (temp_addr & 0xFFFFF000) | (addr & 0xFFF);
	return temp_addr;
}

static void ctr_enable_all_svc_kernel(void)
{
   __asm__ volatile("cpsid aif");

   u32*  svc_access_control = *(*(u32***)0xFFFF9000 + 0x22) - 0x6;

   svc_access_control[0]=0xFFFFFFFE;
   svc_access_control[1]=0xFFFFFFFF;
   svc_access_control[2]=0xFFFFFFFF;
   svc_access_control[3]=0x3FFFFFFF;
}


static void ctr_invalidate_ICache_kernel(void)
{
   __asm__ volatile(
      "cpsid aif\n\t"
      "mov r0, #0\n\t"
      "mcr p15, 0, r0, c7, c5, 0\n\t");
}

static void ctr_flush_DCache_kernel(void)
{
   __asm__ volatile(
      "cpsid aif\n\t"
      "mov r0, #0\n\t"
      "mcr p15, 0, r0, c7, c10, 0\n\t");

}


static void ctr_enable_all_svc(void)
{
   svcBackdoor((ctr_callback_type)ctr_enable_all_svc_kernel);
}

void ctr_invalidate_ICache(void)
{
//   __asm__ volatile("svc 0x2E\n\t");
   svcBackdoor((ctr_callback_type)ctr_invalidate_ICache_kernel);

}

void ctr_flush_DCache(void)
{
//   __asm__ volatile("svc 0x4B\n\t");
   svcBackdoor((ctr_callback_type)ctr_flush_DCache_kernel);
}


void ctr_flush_invalidate_cache(void)
{
   ctr_flush_DCache();
   ctr_invalidate_ICache();
}

extern char translation_cache[1 << 22];
extern char translation_cache_w[1 << 22];

static u32 translation_cache_voffset;
static u32 translation_cache_w_voffset;

int ctr_svchack_init(void)
{
   extern unsigned int __service_ptr;

   if(__service_ptr)
      return 0;

   /* CFW */
   ctr_enable_all_svc();

   uint32_t currentHandle;
   svcDuplicateHandle(&currentHandle, 0xFFFF8001);
   svcControlProcessMemory(currentHandle, (u32)translation_cache, 0x0,
                           0x400000, MEMOP_PROT, 0b111);

//   svcControlProcessMemory(currentHandle, (u32)translation_cache_w, 0x0,
//                           0x400000, MEMOP_PROT, 0b111);

   svcCloseHandle(currentHandle);

   translation_cache_w_voffset = get_PA((u32)translation_cache_w) - (u32)translation_cache_w - 0x0C000000;
   translation_cache_voffset = get_PA((u32)translation_cache) - (u32)translation_cache - 0x0C000000;
   printf("translation_cache         : 0x%08X\n", translation_cache);
   printf("translation_cache   PA    : 0x%08X\n", get_PA((u32)translation_cache));
   printf("translation_cache_voffset : 0x%08X\n", translation_cache_voffset);
   printf("translation_cache_w         : 0x%08X\n", translation_cache_w);
   printf("translation_cache_w PA      : 0x%08X\n", get_PA((u32)translation_cache_w));
   printf("translation_cache_w_voffset : 0x%08X\n", translation_cache_w_voffset);
   DEBUG_HOLD();

   u32 ptr, ptr_offset, test_passed;

   test_passed = 1;
   ptr_offset = get_PA((u32)translation_cache) - (u32)translation_cache;
   for (ptr = (u32)translation_cache; ptr < ((u32)translation_cache + 0x400000); ptr+=0x1000)
   {
      if (get_PA(ptr)!= (ptr + ptr_offset))
      {
         test_passed = 0;
         break;
      }
   }
   printf("translation_cache linear test : %s\n", test_passed? "PASSED": "FAILED");
   DEBUG_HOLD();

   test_passed = 1;
   ptr_offset = get_PA((u32)translation_cache_w) - (u32)translation_cache_w;
   for (ptr = (u32)translation_cache_w; ptr < ((u32)translation_cache_w + 0x400000); ptr+=0x1000)
   {
      if (get_PA(ptr)!= (ptr + ptr_offset))
      {
         test_passed = 0;
         break;
      }
   }
   printf("translation_cache_x linear test : %s\n", test_passed? "PASSED": "FAILED");
   DEBUG_HOLD();


   return 1;
}
Result GX_SetTextureCopy2(u32* gxbuf, u32* inadr, u32 indim, u32* outadr, u32 outdim, u32 size, u32 flags)
{
	if(!gxbuf)gxbuf=gxCmdBuf;

	u32 gxCommand[0x8];
	gxCommand[0]=0x01000004; //CommandID
	gxCommand[1]=(u32)inadr;
	gxCommand[2]=(u32)outadr;
	gxCommand[3]=size;
	gxCommand[4]=indim;
	gxCommand[5]=outdim;
	gxCommand[6]=flags;
	gxCommand[7]=0x0;

	return GSPGPU_SubmitGxCommand(gxbuf, gxCommand, NULL);
}

void GSPwn(void *dest, const void *src, size_t size)
{
	// Attempt a flush of the source, but ignore the result, since we may have just been asked to
	// read unmapped memory or something similar.
//	GSPGPU_FlushDataCache(NULL, src, size);

	// Invalidate the destination's cache, since we're about to overwrite it.  Likewise, ignore
	// errors, since it may be the destination that is an unmapped address.
//	GSPGPU_InvalidateDataCache(NULL, dest, size);

   extern Handle gspEvents[GSPEVENT_MAX];
   svcClearEvent(gspEvents[GSPEVENT_PPF]);

	if (GX_SetTextureCopy(NULL, src, 0, dest, 0, size, 8))
		exit(1);
   svcWaitSynchronization(gspEvents[GSPEVENT_PPF], U64_MAX);

//   gspWaitForPPF();

}

u32* translation_cache_clean = translation_cache;
void ctr_flush_DCache_range2(void* start, void* end)
{
   start = (void*)((u32)start & ~0xF);
   end   = (void*)(((u32)end   + 0xF) & ~0xF);
   u32 src = (u32)start - (u32)translation_cache + (u32)translation_cache_w + translation_cache_w_voffset;
   u32 dst = (u32)start + translation_cache_voffset;
   u32 size = (u32)(end)-(u32)(start);
//   size = (size + 0xF) & ~0xF;
//   if(src & 0xF)
//      DEBUG_HOLD();
//   if(dst & 0xF)
//      DEBUG_HOLD();
//   u32* tmp;
//   for (tmp = start; tmp < end; tmp++)
//   {
//      if(*tmp != 0xFCFCFCFC)
//      {
//         printf("tmp : 0x%08X = 0x%08X", tmp, *tmp);
//         DEBUG_HOLD();
//         break;
//      }
//   }

//   translation_cache_clean = end;

//   svcFlushProcessDataCache(0xFFFF8001, (u32)start - (u32)translation_cache + (u32)translation_cache_w, size);
   svcFlushProcessDataCache(0xFFFF8001, (u32)start , size);
//   printf("start: 0x%08X, end: 0x%08X\n", start, end);
//   printf("GSPwn(0x%08X, 0x%08X, %u)\n", dst, src, size);
//   DEBUG_HOLD();
   ctr_flush_invalidate_cache();
   GSPwn((void*)dst, (void*)src, size);
   ctr_flush_invalidate_cache();

//   svcFlushProcessDataCache(0xFFFF8001, start, (u32)(end)-(u32)(start));
}
#include <string.h>
void ctr_flush_DCache_range(void* start, void* end)
{
   u32 src = (u32)start - (u32)translation_cache + (u32)translation_cache_w;
   u32 dst = (u32)start;
   u32 size = (u32)(end)-(u32)(start);

//   u32* tmp;
//   for (tmp = start; tmp < end; tmp++)
//   {
//      if(*tmp != 0xFCFCFCFC)
//      {
//         printf("tmp : 0x%08X = 0x%08X", tmp, *tmp);
//         DEBUG_HOLD();
//         break;
//      }
//   }
   u32* tmp1, *tmp2;
   for (tmp1 = translation_cache, tmp2=translation_cache_w; tmp1 < start; tmp1++,tmp2++)
   {
      if(*tmp1 != *tmp2)
      {
         printf("tmp1 : 0x%08X = 0x%08X\ntmp2 : 0x%08X = 0x%08X\n", tmp1, *tmp1, tmp2, *tmp2);
         DEBUG_HOLD();
         break;
      }
   }

   translation_cache_clean = end;

   memcpy(dst, src, size);
//   svcFlushProcessDataCache(0xFFFF8001, (u32)dst , size);

//   memcpy(translation_cache, translation_cache_w, 0x400000);
//   DEBUG_HOLD();
//   svcFlushProcessDataCache(0xFFFF8001, translation_cache , 0x400000);

   ctr_flush_invalidate_cache();

//   svcFlushProcessDataCache(0xFFFF8001, start, (u32)(end)-(u32)(start));
}
