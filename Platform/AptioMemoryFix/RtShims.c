/**

  Runtime Services Wrappers.

  by Download-Fritz & vit9696

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include "Config.h"
#include "Lib.h"
#include "RtShims.h"

extern UINTN gRtShimsDataStart;
extern UINTN gRtShimsDataEnd;

extern UINTN RtShimGetVariable;
extern UINTN RtShimGetNextVariableName;
extern UINTN RtShimSetVariable;
extern UINTN RtShimGetTime;
extern UINTN RtShimSetTime;
extern UINTN RtShimGetWakeupTime;
extern UINTN RtShimSetWakeupTime;
extern UINTN RtShimGetNextHighMonoCount;
extern UINTN RtShimResetSystem;

VOID *gRtShims = NULL;

STATIC BOOLEAN mRtShimsAddrUpdated = FALSE;

STATIC ShimPtrs mShimPtrArray[] = {
  { &gGetVariable },
  { &gSetVariable },
  { &gGetNextVariableName },
  { &gGetTime },
  { &gSetTime },
  { &gGetWakeupTime },
  { &gSetWakeupTime },
  { &gGetNextHighMonoCount },
  { &gResetSystem }
};

VOID InstallRtShims (
  EFI_GET_VARIABLE GetVariableOverride
  )
{
  EFI_STATUS Status;

#if APTIOFIX_ALLOCATE_POOL_GIVES_STABLE_ADDR == 1
  //
  // Allocating from pool may use random addresses, including the ones requested
  // by the kernel may sit, so is very dangerous.
  // However, it almost always produces the same address across the reboots
  // unlike AllocatePagesFromTop, which is necessary for a memory map reuse
  // when waking from hibernation.
  // Remove once we can wake with APTIOFIX_HIBERNATION_FORCE_OLD_MEMORYMAP = 0.
  //
  Status = gBS->AllocatePool (
    EfiRuntimeServicesCode,
    ((UINTN)&gRtShimsDataEnd - (UINTN)&gRtShimsDataStart),
    &gRtShims
    );
#else
  EFI_PHYSICAL_ADDRESS RtShims = BASE_4GB;
  Status = AllocatePagesFromTop (
    EfiRuntimeServicesCode,
    EFI_SIZE_TO_PAGES ((UINTN)&gRtShimsDataEnd - (UINTN)&gRtShimsDataStart),
    &RtShims
    );
  gRtShims             = (VOID *)(UINTN)RtShims;
#endif

  if (!EFI_ERROR (Status)) {
    gGetVariable          = (UINTN)gRT->GetVariable;
    gSetVariable          = (UINTN)gRT->SetVariable;
    gGetNextVariableName  = (UINTN)gRT->GetNextVariableName;
    gGetTime              = (UINTN)gRT->GetTime;
    gSetTime              = (UINTN)gRT->SetTime;
    gGetWakeupTime        = (UINTN)gRT->GetWakeupTime;
    gSetWakeupTime        = (UINTN)gRT->SetWakeupTime;
    gGetNextHighMonoCount = (UINTN)gRT->GetNextHighMonotonicCount;
    gResetSystem          = (UINTN)gRT->ResetSystem;

    gGetVariableOverride  = (UINTN)GetVariableOverride;

    CopyMem (
      gRtShims,
      (VOID *)&gRtShimsDataStart,
      ((UINTN)&gRtShimsDataEnd - (UINTN)&gRtShimsDataStart)
      );

    gRT->GetVariable               = (EFI_GET_VARIABLE)((UINTN)gRtShims              + ((UINTN)&RtShimGetVariable          - (UINTN)&gRtShimsDataStart));
    gRT->SetVariable               = (EFI_SET_VARIABLE)((UINTN)gRtShims              + ((UINTN)&RtShimSetVariable          - (UINTN)&gRtShimsDataStart));
    gRT->GetNextVariableName       = (EFI_GET_NEXT_VARIABLE_NAME)((UINTN)gRtShims    + ((UINTN)&RtShimGetNextVariableName  - (UINTN)&gRtShimsDataStart));
    gRT->GetTime                   = (EFI_GET_TIME)((UINTN)gRtShims                  + ((UINTN)&RtShimGetTime              - (UINTN)&gRtShimsDataStart));
    gRT->SetTime                   = (EFI_SET_TIME)((UINTN)gRtShims                  + ((UINTN)&RtShimSetTime              - (UINTN)&gRtShimsDataStart));
    gRT->GetWakeupTime             = (EFI_GET_WAKEUP_TIME)((UINTN)gRtShims           + ((UINTN)&RtShimGetWakeupTime        - (UINTN)&gRtShimsDataStart));
    gRT->SetWakeupTime             = (EFI_SET_WAKEUP_TIME)((UINTN)gRtShims           + ((UINTN)&RtShimSetWakeupTime        - (UINTN)&gRtShimsDataStart));
    gRT->GetNextHighMonotonicCount = (EFI_GET_NEXT_HIGH_MONO_COUNT)((UINTN)gRtShims  + ((UINTN)&RtShimGetNextHighMonoCount - (UINTN)&gRtShimsDataStart));
    gRT->ResetSystem               = (EFI_RESET_SYSTEM)((UINTN)gRtShims              + ((UINTN)&RtShimResetSystem          - (UINTN)&gRtShimsDataStart));

    gRT->Hdr.CRC32 = 0;
    gBS->CalculateCrc32(gRT, gRT->Hdr.HeaderSize, &gRT->Hdr.CRC32);
  } else {
    DEBUG ((DEBUG_VERBOSE, "Nulling RtShims\n"));
    gRtShims = NULL;
  }
}


VOID
UninstallRtShims (
  VOID
  )
{
  gRT->GetVariable               = (EFI_GET_VARIABLE)gGetVariable;
  gRT->GetNextVariableName       = (EFI_GET_NEXT_VARIABLE_NAME)gGetNextVariableName;
  gRT->SetVariable               = (EFI_SET_VARIABLE)gSetVariable;
  gRT->GetTime                   = (EFI_GET_TIME)gGetTime;
  gRT->SetTime                   = (EFI_SET_TIME)gSetTime;
  gRT->GetWakeupTime             = (EFI_GET_WAKEUP_TIME)gGetWakeupTime;
  gRT->SetWakeupTime             = (EFI_SET_WAKEUP_TIME)gSetWakeupTime;
  gRT->GetNextHighMonotonicCount = (EFI_GET_NEXT_HIGH_MONO_COUNT)gGetNextHighMonoCount;
  gRT->ResetSystem               = (EFI_RESET_SYSTEM)gResetSystem;

  gRT->Hdr.CRC32 = 0;
  gBS->CalculateCrc32(gRT, gRT->Hdr.HeaderSize, &gRT->Hdr.CRC32);
}

VOID
VirtualizeRtShims (
  UINTN                  MemoryMapSize,
  UINTN                  DescriptorSize,
  EFI_MEMORY_DESCRIPTOR  *MemoryMap
  )
{
  EFI_MEMORY_DESCRIPTOR  *Desc;
  UINTN                  Index, Index2, FixedCount = 0;

  //
  // For some reason creating an event for catching SetVirtualAddress doesn't work on APTIO IV Z77,
  // So we cannot use a dedicated ConvertPointer function and have to implement everything manually.
  //

  //
  // Are we already done?
  //
  if (mRtShimsAddrUpdated)
    return;

  Desc = MemoryMap;

  //
  // Custom GetVariable wrapper is no longer allowed!
  //
  *(UINTN *)((UINTN)gRtShims + ((UINTN)&gGetVariableOverride - (UINTN)&gRtShimsDataStart)) = 0;

  for (Index = 0; Index < ARRAY_SIZE (mShimPtrArray); ++Index) {
    mShimPtrArray[Index].Func = (UINTN *)((UINTN)gRtShims + ((UINTN)(mShimPtrArray[Index].gFunc) - (UINTN)&gRtShimsDataStart));
  }

  for (Index = 0; Index < (MemoryMapSize / DescriptorSize); ++Index) {
    for (Index2 = 0; Index2 < ARRAY_SIZE (mShimPtrArray); ++Index2) {
      if (
        !mShimPtrArray[Index2].Fixed &&
        (*(mShimPtrArray[Index2].gFunc) >= Desc->PhysicalStart) &&
        (*(mShimPtrArray[Index2].gFunc) < (Desc->PhysicalStart + EFI_PAGES_TO_SIZE (Desc->NumberOfPages)))
      ) {
        mShimPtrArray[Index2].Fixed = TRUE;
        *(mShimPtrArray[Index2].Func) += (Desc->VirtualStart - Desc->PhysicalStart);
        FixedCount++;
      }
    }

    if (FixedCount == ARRAY_SIZE (mShimPtrArray)) {
      break;
    }

    Desc = NEXT_MEMORY_DESCRIPTOR (Desc, DescriptorSize);
  }

  mRtShimsAddrUpdated = TRUE;
}
