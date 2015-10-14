
#include "3ds.h"
#include "3ds_utils.h"
#include "../libpcsxcore/new_dynarec/assem_arm.h"
#include <stdlib.h>
#include <string.h>
//#include "khax.h"

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



/* only works if addr is page aligned, and the first 16bytes are
 * different from the first 16bytes in all other pages in FCRAM */

u32* get_fake_linear_addr(u32* addr)
{
   extern Handle gspEvents[GSPEVENT_MAX];


//   u32 top_fcram = 0x8000000;
   u32 top_fcram   = 0x4000000;
   u32 buffer_size = 0x10000;
   u32 stepSize    = 0x100;

   u32 pages_to_scan = buffer_size >> 4;
   u32 skipSize     = stepSize - 0x10;
   u32* buffer = linearMemAlign(buffer_size, 0x1000);

//   DEBUG_HOLD();
   addr = (u32*)((u32)addr & ~0xFFF);

//   svcSleepThread(10000000);

   u32 start_address;
   u32* ret = (u32*)-1;

   while (top_fcram >= pages_to_scan * stepSize)
   {
      start_address = (u32*)(0x14000000 + top_fcram - pages_to_scan * stepSize);

      GSPGPU_InvalidateDataCache(NULL, buffer, buffer_size);
      svcClearEvent(gspEvents[GSPEVENT_PPF]);
         GX_SetTextureCopy2(NULL, start_address, (skipSize << 16) | 0x10, buffer, 0x10, buffer_size, 0xC);
      svcWaitSynchronization(gspEvents[GSPEVENT_PPF], U64_MAX);


//      DEBUG_HOLD();
      int i;
      u32* ptr = buffer;
      for (i=0; i < pages_to_scan ; i += (0x1000 / stepSize), ptr += (0x1000 / stepSize) * 0x4)
         if(!memcmp(ptr, addr, 0x10))
            break;

      if(i < pages_to_scan)
      {
         ret = (u32*)((u32)start_address + i * stepSize);
         break;
      }

      top_fcram -= pages_to_scan * stepSize;
   }
//   DEBUG_HOLD();

   linearFree(buffer);
   return ret;
}


static Result ctrGuSetCommandList_First(bool queued, u32* buf0a, u32 buf0s, u32* buf1a, u32 buf1s, u32* buf2a, u32 buf2s)
{
   u32 gxCommand[0x8];
   gxCommand[0]=0x05 | (queued? 0x01000000 : 0x0); //CommandID
   gxCommand[1]=(u32)buf0a; //buf0 address
   gxCommand[2]=(u32)buf0s; //buf0 size
   gxCommand[3]=(u32)buf1a; //buf1 address
   gxCommand[4]=(u32)buf1s; //buf1 size
   gxCommand[5]=(u32)buf2a; //buf2 address
   gxCommand[6]=(u32)buf2s; //buf2 size
   gxCommand[7]=0x0;

   return GSPGPU_SubmitGxCommand(gxCmdBuf, gxCommand, NULL);
}


void GSPwn(void *dest, const void *src, size_t size)
{
	// Attempt a flush of the source, but ignore the result, since we may have just been asked to
	// read unmapped memory or something similar.
//	GSPGPU_FlushDataCache(NULL, (u32)src & ~0xFFF, (size + 0x1FFF) & ~0xFFF);

//   ctrGuSetCommandList_First(false, src, size, 0,0,0,0);
	// Invalidate the destination's cache, since we're about to overwrite it.  Likewise, ignore
	// errors, since it may be the destination that is an unmapped address.
//	GSPGPU_InvalidateDataCache(NULL, dest, size);

   extern Handle gspEvents[GSPEVENT_MAX];
//   svcClearEvent(gspEvents[GSPEVENT_PPF]);
//   svcWaitSynchronization(gspEvents[GSPEVENT_PPF], U64_MAX);
//   DEBUG_HOLD();
   svcClearEvent(gspEvents[GSPEVENT_PPF]);
	if (GX_SetTextureCopy2(NULL, src, 0, dest, 0, size, 8))
		exit(1);
//   DEBUG_HOLD();
   svcWaitSynchronization(gspEvents[GSPEVENT_PPF], U64_MAX);
//   DEBUG_HOLD();
//   svcClearEvent(gspEvents[GSPEVENT_PPF]);
//   svcWaitSynchronization(gspEvents[GSPEVENT_PPF], size * 100 );
//   svcSleepThread(1000000);
   svcSleepThread(10000000);
//   DEBUG_HOLD();

//   gspWaitForPPF();

}
extern char translation_cache[1 << TARGET_SIZE_2];
//extern char * translation_cache;
extern char translation_cache_w[1 << TARGET_SIZE_2];

static u32 translation_cache_voffset;
static u32 translation_cache_w_voffset;

uint32_t currentHandle;
//char translation_cache[1 << TARGET_SIZE_2] __attribute__((aligned(4096)));
int ctr_svchack_init(void)
{
   extern unsigned int __service_ptr;


//   if(translation_cache_ptr != translation_cache)
//   {
//      ((u32*)translation_cache_ptr)[0]= 0xF346ABC3;
//      ((u32*)translation_cache_ptr)[1]= 0xBCD3AD69;
//      ((u32*)translation_cache_ptr)[2]= 0x03FB3A2D;
//      ((u32*)translation_cache_ptr)[3]= 0x3DF853BC;

////      svcFlushProcessDataCache(0xFFFF8001, translation_cache_w, 16);
//      ctrGuSetCommandList_First(true, translation_cache_ptr, 16, 0,0,0,0);
//   }


   ((u32*)translation_cache_w_ptr)[0]= 0x3DF853BC;
   ((u32*)translation_cache_w_ptr)[1]= 0x03FB3A2D;
   ((u32*)translation_cache_w_ptr)[2]= 0xBCD3AD69;
   ((u32*)translation_cache_w_ptr)[3]= 0xF346ABC3;
//   svcFlushProcessDataCache(0xFFFF8001, translation_cache_w, 16);
   ctrGuSetCommandList_First(true, translation_cache_w_ptr, 16, 0,0,0,0);
//   DEBUG_HOLD();

   translation_cache_voffset   = (u32)get_fake_linear_addr(translation_cache_ptr)   - (u32)translation_cache_ptr;
   translation_cache_w_voffset = (u32)get_fake_linear_addr(translation_cache_w_ptr) - (u32)translation_cache_w_ptr;

//   translation_cache_voffset = translation_cache_w_voffset;

   printf("tr_cache_ptr      : 0x%08X\n", translation_cache_ptr);
   printf("tr_cache_voffset  : 0x%08X\n", translation_cache_voffset);
   printf("tr_cache_w_ptr    : 0x%08X\n", translation_cache_w_ptr);
   printf("tr_cache_w_voffset: 0x%08X\n", translation_cache_w_voffset);
   printf("tr_cache_offset   : 0x%08X\n", translation_cache_offset);

   DEBUG_HOLD();

#if 1
   u32 ptr, ptr_offset, test_passed, fake_ptr;

   test_passed = 1;
   for (ptr = (u32)translation_cache_ptr; ptr < ((u32)translation_cache_ptr + (1 << TARGET_SIZE_2)); ptr+=0x1000)
   {
      fake_ptr = (u32)get_fake_linear_addr(ptr);
      if (fake_ptr!= (ptr + translation_cache_voffset))
      {
         test_passed = 0;
         break;
      }
   }
   printf("translation_cache_ptr linear test : %s\n", test_passed? "PASSED": "FAILED");
   if(!test_passed)
   {
      printf("failed @0x%08X --> 0x%08X\n", ptr, fake_ptr);
      printf("offset old:0x%08X new: 0x%08X\n", translation_cache_voffset, fake_ptr - ptr);
   }

   test_passed = 1;
   for (ptr = (u32)translation_cache_w_ptr; ptr < ((u32)translation_cache_w_ptr + (1 << TARGET_SIZE_2)); ptr+=0x1000)
   {
      ((u32*)ptr)[0]= 0x3DF853BC|ptr;
      ((u32*)ptr)[1]= 0x03FB3A2D|ptr;
      ((u32*)ptr)[2]= 0xBCD3AD69|ptr;
      ((u32*)ptr)[3]= 0xF346ABC3|ptr;
      ctrGuSetCommandList_First(true, ptr, 16, 0,0,0,0);
      fake_ptr = (u32)get_fake_linear_addr(ptr);
      if (fake_ptr!= (ptr + translation_cache_w_voffset))
      {
         test_passed = 0;
         break;
      }
   }
   printf("translation_cache_w_ptr linear test : %s\n", test_passed? "PASSED": "FAILED");
   if(!test_passed)
   {
      printf("failed @0x%08X --> 0x%08X\n", ptr, fake_ptr);
      printf("offset old:0x%08X new: 0x%08X\n", translation_cache_w_voffset, fake_ptr - ptr);
   }

   DEBUG_HOLD();
#endif
//   if(__service_ptr)
      return 1;

#if 0

//   if(__service_ptr)
//      return 0;
//      if(khaxInit())
//         return 0;

   /* CFW */
   ctr_enable_all_svc();
#if 1
   svcDuplicateHandle(&currentHandle, 0xFFFF8001);
   svcControlProcessMemory(currentHandle, (u32)translation_cache_ptr, 0x0,
                           1 << TARGET_SIZE_2, MEMOP_PROT, 0b111);
//                           1 << TARGET_SIZE_2, MEMOP_PROT, 0b101);
   svcControlProcessMemory(currentHandle, (u32)translation_cache_w_ptr, 0x0,
                           1 << TARGET_SIZE_2, MEMOP_PROT, 0b111);

   svcCloseHandle(currentHandle);
#endif

   translation_cache_voffset = get_PA((u32)translation_cache_ptr) - (u32)translation_cache_ptr - 0x0C000000;
   translation_cache_w_voffset = get_PA((u32)translation_cache_w_ptr) - (u32)translation_cache_w_ptr - 0x0C000000;


   printf("tr_cache_ptr      : 0x%08X\n", translation_cache_ptr);
   printf("tr_cache_ptr PA   : 0x%08X\n", get_PA((u32)translation_cache_ptr));
   printf("tr_cache_voffset  : 0x%08X\n", translation_cache_voffset);
   printf("tr_cache_w_ptr    : 0x%08X\n", translation_cache_w_ptr);
   printf("tr_cache_w_ptr PA : 0x%08X\n", get_PA((u32)translation_cache_w_ptr));
   printf("tr_cache_w_voffset: 0x%08X\n", translation_cache_w_voffset);
   DEBUG_HOLD();

//   u32 ptr, ptr_offset, test_passed;

   test_passed = 1;
   ptr_offset = get_PA((u32)translation_cache_ptr) - (u32)translation_cache_ptr;
   for (ptr = (u32)translation_cache_ptr; ptr < ((u32)translation_cache_ptr + (1 << TARGET_SIZE_2)); ptr+=0x1000)
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
   ptr_offset = get_PA((u32)translation_cache_w_ptr) - (u32)translation_cache_w_ptr;
   for (ptr = (u32)translation_cache_w_ptr; ptr < ((u32)translation_cache_w_ptr + (1 << TARGET_SIZE_2)); ptr+=0x1000)
   {
      if (get_PA(ptr)!= (ptr + ptr_offset))
      {
         test_passed = 0;
         break;
      }
   }
   printf("translation_cache_w linear test : %s\n", test_passed? "PASSED": "FAILED");
   DEBUG_HOLD();

   return 1;
#endif
}

//u32* translation_cache_clean = translation_cache;
void ctr_flush_DCache_range(void* start, void* end)
{
//   svcFlushProcessDataCache(0xFFFF8001, (u32)start - (u32)translation_cache + (u32)translation_cache_w, (u32)end - (u32)start);
//   svcInvalidateProcessDataCache(0xFFFF8001, (u32)start, (u32)end - (u32)start);
//   GSPGPU_FlushDataCache(NULL, (u32)start - (u32)translation_cache + (u32)translation_cache_w, (u32)end - (u32)start);
//   DEBUG_HOLD();


//   DEBUG_HOLD();
   start = (void*)((u32)start & ~0xF);
   end   = (void*)(((u32)end   + 0xF) & ~0xF);

   ctrGuSetCommandList_First(true, (u32)start + translation_cache_offset, (u32)end - (u32)start, 0,0,0,0);

   GSPGPU_InvalidateDataCache(NULL, (u32)start + translation_cache_offset, (u32)end - (u32)start);

   u32 src = (u32)start + translation_cache_offset + translation_cache_w_voffset;
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


//   svcFlushProcessDataCache(0xFFFF8001, (u32)start , size);
//   printf("start: 0x%08X, end: 0x%08X\n", start, end);
//   printf("GSPwn(0x%08X, 0x%08X, %u)\n", dst, src, size);
//   DEBUG_HOLD();
//   ctr_flush_invalidate_cache();
//   DEBUG_HOLD();
   GSPwn((void*)dst, (void*)src, size);
//   DEBUG_HOLD();
//   ctr_flush_invalidate_cache();

//   svcFlushProcessDataCache(0xFFFF8001, start, (u32)(end)-(u32)(start));
}
#include <string.h>
void ctr_flush_DCache_range22(void* start, void* end)
{
//   printf("start : 0x%08X, end: 0x%08X\n", start, end);
//   DEBUG_HOLD();


   u32 src = (u32)start + translation_cache_offset;
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
//   u32* tmp1, *tmp2;
//   for (tmp1 = translation_cache, tmp2=translation_cache_w; tmp1 < start; tmp1++,tmp2++)
//   {
//      if(*tmp1 != *tmp2)
//      {
//         printf("tmp1 : 0x%08X = 0x%08X\ntmp2 : 0x%08X = 0x%08X\n", tmp1, *tmp1, tmp2, *tmp2);
//         DEBUG_HOLD();
//         break;
//      }
//   }

//   translation_cache_clean = end;

   memcpy(dst, src, size);
//   svcFlushProcessDataCache(0xFFFF8001, (u32)dst , size);

//   memcpy(translation_cache, translation_cache_w, 0x400000);
//   DEBUG_HOLD();
//   svcFlushProcessDataCache(0xFFFF8001, translation_cache , 0x400000);

   ctr_flush_invalidate_cache();

//   svcFlushProcessDataCache(0xFFFF8001, start, (u32)(end)-(u32)(start));
}
