/*======================================================

FILE:  DSS_EventHandlerMCast.cpp

GENERAL DESCRIPTION:
   Implementation of DSS_EventHandlerMCast functions

=====================================================

Copyright (c) 2008 - 2010 Qualcomm Technologies, Inc.
All Rights Reserved.
Qualcomm Technologies Confidential and Proprietary

=====================================================*/
/*===========================================================================
  EDIT HISTORY FOR MODULE

  Please notice that the changes are listed in reverse chronological order.

  $Header: //source/qcom/qct/modem/datamodem/interface/dss/rel/11.03/src/DSS_EventHandlerMCast.cpp#1 $
  $DateTime: 2011/06/17 12:02:33 $$Author: zhasan $

  when       who what, where, why
  ---------- --- ------------------------------------------------------------
  2010-04-18 en  History added.

===========================================================================*/

#include "customer.h"
#include "DSS_Globals.h"
#include "DSS_Common.h"
#include "DSS_CritScope.h"

#include "DSS_EventHandlerMCast.h"
#include "DSS_MCast.h"
#include "DSS_EventHandlerNetApp.h"
#include "ds_Utils_CreateInstance.h"
#include "AEECCritSect.h"

void DSSEventHandlerMCast::DispatchCB(dss_iface_ioctl_event_enum_type event, EventData* ped,
                                 dss_iface_ioctl_event_info_union_type* eventInfo)
{
   LOG_MSG_INFO1("DispachCB of event number %d", event,0,0);
   dss_iface_id_type ifaceId = 0;
   parentNetApp->GetMCastHandle(&ifaceId);

   // take only the appID part of the iface ID (the middle 16 bits)
   sint15 appID =  (sint15)((ifaceId >> 8) & 0xFFFF);

   LOG_MSG_INFO1("DSS calld App callback, event: %d, ifaceId: %u, appID: %d", event, ifaceId, appID);
   ped->userCB(event, *eventInfo, ped->userData, appID, ifaceId);
}

AEEResult DSSEventHandlerMCast::Init(DSSMCast* parentNetAppParam)
{
   parentNetApp = parentNetAppParam;
   (void) parentNetApp->AddRefWeak();
   AEEResult res;

   ISignalFactory *piSignalFactory = 0;
   IDS_ERR_RET(DSSGlobals::Instance()->GetSignalFactory(&piSignalFactory));
   IDS_ERR_RET(piSignalFactory->CreateSignal(&signalHandler,
                                             reinterpret_cast <uint32> (SignalCB),
                                             reinterpret_cast <uint32> (this),
                                             &piSignal, &piSignalCtl));

   IDS_ERR_RET(DSSGlobals::Instance()->GetCritSect(&piCritSect));
   piCritSect->Enter();
   res = RegisterIDL();
   piCritSect->Leave();
   return res;
}

void DSSEventHandlerMCast::Destructor() throw()
{
   DS_UTILS_RELEASE_WEAKREF_IF(parentNetApp);

   DSSEventHandler::Destructor();
}

