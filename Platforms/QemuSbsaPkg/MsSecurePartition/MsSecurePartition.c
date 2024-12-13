/** @file
  Microsoft Secure Partition

  Copyright (c), Microsoft Corporation.
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

// Secure Partition Headers
#include <PiMm.h>
#include <Base.h>
#include <IndustryStandard/ArmFfaSvc.h>

#include <Library/StandaloneMmCoreEntryPoint.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/IoLib.h>
#include <Library/PcdLib.h>
#include <Library/ArmSvcLib.h>
#include <Library/ArmFfaLib.h>
#include <Library/ArmFfaLibEx.h>
#include <Library/NotificationServiceLib.h>
#include <Library/TestServiceLib.h>
#include <Library/TpmServiceLib.h>

// Service specific structures/variables
EFI_GUID NotificationServiceGuid = NOTIFICATION_SERVICE_UUID;
EFI_GUID TpmServiceGuid = TPM_SERVICE_UUID;
EFI_GUID TestServiceGuid = TEST_SERVICE_UUID;
volatile BOOLEAN Loop = TRUE;

/**
  Message Handler for the Microsoft Secure Partition

  @param  Request       The incoming message
  @param  Response      The outgoing message

**/
STATIC
VOID
EFIAPI
MsSecurePartitionHandleMessage (
  DIRECT_MSG_ARGS_EX *Request, 
  DIRECT_MSG_ARGS_EX *Response
  )
{
  ZeroMem (Response, sizeof (DIRECT_MSG_ARGS_EX));
  Response->SourceId = Request->DestinationId;
  Response->DestinationId = Request->SourceId;

  if (!CompareMem(&Request->ServiceGuid, &NotificationServiceGuid, sizeof(EFI_GUID))) {
    NotificationServiceHandle (Request, Response);
  } else if (!CompareMem(&Request->ServiceGuid, &TpmServiceGuid, sizeof(EFI_GUID))) {
    TpmServiceHandle(Request, Response);
  } else if (!CompareMem(&Request->ServiceGuid, &TestServiceGuid, sizeof(EFI_GUID))) {
    TestServiceHandle(Request, Response);
  } else {
    DEBUG ((DEBUG_ERROR, "Invalid secure partition service UUID\n"));
    Response->Arg0 = EFI_NOT_FOUND;
  }
}

/**
  The Entry Point for Microsoft Secure Partition.

  @param  HobStart       Pointer to the start of the HOB list.

  @retval EFI_SUCCESS             Success.
  @retval EFI_UNSUPPORTED         Unsupported operation.
**/
EFI_STATUS
EFIAPI
MsSecurePartitionMain (
  IN VOID  *HobStart
  )
{
  EFI_STATUS          Status;
  DIRECT_MSG_ARGS_EX  Request;
  DIRECT_MSG_ARGS_EX  Response;

  DEBUG ((DEBUG_INFO, "%a - 0x%x\n", __func__, HobStart));
  DUMP_HEX (DEBUG_ERROR, 0, 0x000001001FFFE000, EFI_PAGE_SIZE, "");

  // Initialize the services running in this secure partition
  NotificationServiceInit ();
  TpmServiceInit ();
  TestServiceInit ();

  DEBUG ((DEBUG_INFO, "MS-Services secure partition initialized and running!\n"));

  Status = FfaMessageWait (&Request);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "Failed to wait for message %r\n", Status));
    while (Loop) {
    }
  }

  while (1) {
    MsSecurePartitionHandleMessage(&Request, &Response);

    Status = FfaMessageSendDirectResp2(&Response, &Request);
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to send direct response %r\n", Status));
      Status = FfaMessageWait(&Request);
      if (EFI_ERROR (Status)) {
        DEBUG ((DEBUG_ERROR, "Failed to wait for message %r\n", Status));
        while (Loop) {
        }
      }
    }
  }

  DEBUG ((DEBUG_INFO, "%a - Done!\n", __func__));
  return EFI_SUCCESS;
}
