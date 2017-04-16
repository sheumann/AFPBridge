#pragma noroot

#include <stddef.h>
#include <ctype.h>
#include <appletalk.h>
#include <tcpip.h>
#include <orca.h>
#include <misctool.h>
#include "atipmapping.h"
#include "asmglue.h"
#include "aspinterface.h"
#include "installcmds.h"
#include "cmdproc.h"
#include "afpoptions.h"
#include "strncasecmp.h"

struct ATIPMapping atipMapping;

#define DEFAULT_DSI_PORT 548

static char ATIPTypeName[] = "\pAFPServer";
static char DefaultZoneName[] = "\p*";

// Next numbers to use for new mappings
static int nextNode = 1;
static int nextSocket = 1;

#define return_error(x) do {        \
    commandRec->result = (x);       \
    commandRec->actualMatch = 0;    \
    RestoreStateReg(stateReg);      \
    return 0;                       \
} while (0)

#pragma databank 1
LongWord DoLookupName(NBPLookupNameRec *commandRec) {
    cvtRec hostInfo;
    dnrBuffer dnrInfo;
    Byte *curr, *dest;
    unsigned int count, nameLen;
    NBPLUNameBufferRec *resultBuf;
    LongWord initialTime;
    Word stateReg;
    unsigned int flags;
    
    stateReg = ForceRomIn();
    
    if (OS_KIND != KIND_GSOS)
        goto passThrough;
    
    // Length needed for result, assuming the request is for our type/zone
    count = offsetof(NBPLUNameBufferRec, entityName) 
            + ((EntName*)commandRec->entityPtr)->buffer[0] + 1
            + ATIPTypeName[0] + 1 + DefaultZoneName[0] + 1;  
    if (count > commandRec->bufferLength)
        goto passThrough;

    resultBuf = (NBPLUNameBufferRec *)commandRec->bufferPtr;

    curr = &((EntName*)commandRec->entityPtr)->buffer[0];
    dest = &resultBuf->entityName.buffer[0];
    
    // Copy object name into result buffer
    nameLen = *curr;
    for (count = 0; count <= nameLen; count++) {
        *dest++ = *curr++;
    }
    
    // Check that entity type and zone are what we want,
    // and copy names to result buffer.
    nameLen = *curr;
    for (count = 0; count <= nameLen; count++) {
        if (toupper(*curr++) != toupper(ATIPTypeName[count]))
            goto passThrough;
        *dest++ = ATIPTypeName[count];
    }
    nameLen = *curr;
    flags = 0xFFFF;
    for (count = 0; afpOptions[count].zoneString != NULL; count++) {
        if (strncasecmp(afpOptions[count].zoneString, curr+1, nameLen) == 0) {
            flags = afpOptions[count].flags;
            break;
        }
    }
    if (flags == 0xFFFF)
        goto passThrough;

    nameLen = *DefaultZoneName;
    for (count = 0; count <= nameLen; count++) {
        *dest++ = DefaultZoneName[count];
    }
    
    if (TCPIPValidateIPString(&resultBuf->entityName.buffer[0])) {
        TCPIPConvertIPToHex(&hostInfo, &resultBuf->entityName.buffer[0]);
        resultBuf->enumerator = 0;
        
        // TCPIPConvertIPToHex seems not to give port, so get it this way
        hostInfo.cvtPort =
            TCPIPMangleDomainName(0, &resultBuf->entityName.buffer[0]);
    } else {
        hostInfo.cvtPort =
            TCPIPMangleDomainName(0xE000, &resultBuf->entityName.buffer[0]);
        TCPIPDNRNameToIP(&resultBuf->entityName.buffer[0], &dnrInfo);
        if (toolerror())
            return_error(nbpNameErr);

        initialTime = GetTick();
        while (dnrInfo.DNRstatus == DNR_Pending) {
            if (GetTick() - initialTime >= 15*60)
                break;
            TCPIPPoll();
        }

        // Re-copy object name into result buffer to undo mangling
        curr = &((EntName*)commandRec->entityPtr)->buffer[0];
        dest = &resultBuf->entityName.buffer[0];
        nameLen = *curr;
        for (count = 0; count <= nameLen; count++) {
            *dest++ = *curr++;
        }

        if (dnrInfo.DNRstatus == DNR_OK) {
            hostInfo.cvtIPAddress = dnrInfo.DNRIPaddress;
            resultBuf->enumerator = 1;
        } else if (dnrInfo.DNRstatus == DNR_NoDNSEntry) {
            return_error(0);    // not really an error, but 0 results
        } else {
            TCPIPCancelDNR(&dnrInfo);
            return_error(nbpNameErr);
        }   
    }
        
    if (hostInfo.cvtPort == 0)
        hostInfo.cvtPort = DEFAULT_DSI_PORT;
    
    for (count = 0; count < MAX_SESSIONS; count++) {
        if (sessionTbl[count].atipMapping.ipAddr == hostInfo.cvtIPAddress
            && sessionTbl[count].atipMapping.port == hostInfo.cvtPort)
        {
            atipMapping = sessionTbl[count].atipMapping;
            goto haveMapping;
        }
    }
    
    atipMapping.ipAddr = hostInfo.cvtIPAddress;
    atipMapping.port = hostInfo.cvtPort;
    atipMapping.networkNumber = 0xFFFF;     /* invalid/reserved */
    atipMapping.node = nextNode++;
    atipMapping.socket = nextSocket++;
    if (nextNode == 254)
        nextNode = 1;
    if (nextSocket == 255)
        nextSocket = 1;

haveMapping:
    atipMapping.flags = flags;
    resultBuf->netNum = atipMapping.networkNumber;
    resultBuf->nodeNum = atipMapping.node;
    resultBuf->socketNum = atipMapping.socket;
    commandRec->actualMatch = 1;
    commandRec->result = 0;
    if ((commandRec->async & AT_ASYNC) && commandRec->completionPtr != 0) {
        CallCompletionRoutine((void *)commandRec->completionPtr);
    }
    RestoreStateReg(stateReg);
    return 0;
    
passThrough:
    RestoreStateReg(stateReg);
    return oldCmds[commandRec->command];
}
#pragma databank 0

#undef return_error
