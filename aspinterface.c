#pragma noroot

#include <AppleTalk.h>
#include <misctool.h>
#include <string.h>

#include "session.h"
#include "aspinterface.h"
#include "dsi.h"
#include "tcpconnection.h"
#include "endian.h"
#include "readtcp.h"
#include "asmglue.h"
#include "cmdproc.h"
#include "installcmds.h"
#include "atipmapping.h"
#include "afpoptions.h"
#include "strncasecmp.h"

typedef struct FPReadRec {
    Word CommandCode;   /* includes pad byte */
    Word OForkRefNum;
    LongWord Offset;
    LongWord ReqCount;
    Byte NewLineMask;
    Byte NewLineChar;
} FPReadRec;

typedef struct FPWriteRec {
    Word CommandCode;   /* includes pad byte */
    Word OForkRecNum;
    LongWord Offset;
    LongWord ReqCount;
} FPWriteRec;

typedef struct FPSetFileDirParmsRec {
    Word CommandCode;   /* includes pad byte */
    Word VolumeID;
    LongWord DirectoryID;
    Word Bitmap;
    Byte PathType;
    /* Pathname and parameters follow */
} FPSetFileDirParmsRec;

#define kFPRead 27
#define kFPLogin 18
#define kFPZzzzz 122
#define kFPSetFileDirParms 35

#define kFPBitmapErr (-5004L)

#define kFPProDOSInfoBit 0x2000     /* In file/directory bitmaps */

#define ASPQuantumSize 4624

/* For forced AFP 2.2 login */
static Byte loginBuf[100];
static const Byte afp20VersionStr[] = "\pAFPVersion 2.0";
static const Byte afp22VersionStr[] = "\pAFP2.2";

static void DoSPGetStatus(Session *sess, ASPGetStatusRec *commandRec);
static void DoSPOpenSession(Session *sess);
static void DoSPCloseSession(Session *sess);
static void DoSPCommand(Session *sess, ASPCommandRec *commandRec);
static void DoSPWrite(Session *sess, ASPWriteRec *commandRec);

static void CompleteASPCommand(SPCommandRec *commandRec, Word result);
static void InitSleepRec(Session *sess, Byte sessRefNum);

Session sessionTbl[MAX_SESSIONS];

#pragma databank 1
LongWord DispatchASPCommand(SPCommandRec *commandRec) {
    Session *sess;
    unsigned int i;
    Word stateReg;
    LongWord lastActivityTime;
    LongWord lastReadCount;
    
    stateReg = ForceRomIn();

top:
    if (commandRec->command == aspGetStatusCommand 
        || commandRec->command==aspOpenSessionCommand)
    {
        if (((ASPGetStatusRec*)commandRec)->slsNet != atipMapping.networkNumber
            || ((ASPGetStatusRec*)commandRec)->slsNode != atipMapping.node
            || ((ASPGetStatusRec*)commandRec)->slsSocket != atipMapping.socket)
        {
            goto callOrig;
        }
        
        if (OS_KIND != KIND_GSOS) {
            CompleteASPCommand(commandRec, aspNetworkErr);
            goto ret;
        }

        for (i = 0; i < MAX_SESSIONS; i++) {
            if (sessionTbl[i].dsiStatus == needsReset)
                EndASPSession(&sessionTbl[i], 0, TRUE);
        }
        for (i = 0; i < MAX_SESSIONS; i++) {
            if (sessionTbl[i].dsiStatus == unused)
                break;
        }
        if (i == MAX_SESSIONS) {
            CompleteASPCommand(commandRec, aspTooManySessions);
            goto ret;
        }
        sess = &sessionTbl[i];
        sess->spCommandRec = commandRec;
        sess->commandPending = TRUE;
        InitSleepRec(sess, i + SESSION_NUM_START);
        
        if ((i = StartTCPConnection(sess)) != 0) {
            FlagFatalError(sess, i);
            goto ret;
        }
        sess->dsiStatus = awaitingHeader;
        InitReadTCP(sess, DSI_HEADER_SIZE, &sess->reply);
        
        if (commandRec->command==aspOpenSessionCommand) {
            sess->attention = (ASPAttentionHeaderRec *)
                ((ASPOpenSessionRec*)commandRec)->attnRtnAddr;
        }
    } else {
        if (commandRec->refNum < SESSION_NUM_START) {
            goto callOrig;
        }
        
        if (OS_KIND != KIND_GSOS) {
            CompleteASPCommand(commandRec, aspNetworkErr);
            goto ret;
        }

        /*
         * Hack to avoid hangs/crashes related to the AppleShare FST's
         * asynchronous polling to check if volumes have been modified.
         * This effectively disables that mechanism (which is the only
         * real user of asynchronous ASP calls).
         * 
         * TODO Identify and fix the underlying issue.
         */
        if ((commandRec->async & AT_ASYNC)
            && commandRec->command == aspCommandCommand)
        {
            CompleteASPCommand(commandRec, atSyncErr);
            goto ret;
        }
        
        sess = &sessionTbl[commandRec->refNum - SESSION_NUM_START];
        
        if (sess->dsiStatus == unused) {
            CompleteASPCommand(commandRec, aspRefErr);
            goto ret;
        }

        if (sess->commandPending) {
            if (commandRec->command != aspCloseSessionCommand) {
                CompleteASPCommand(commandRec, aspSessNumErr);
                goto ret;
            } else {
                CompleteCurrentASPCommand(sess, aspSessionClosed);
            }
        }

        sess->spCommandRec = commandRec;
        if (commandRec->command != aspCloseSessionCommand)
            sess->commandPending = TRUE;
    }
    
    switch (commandRec->command) {
    case aspGetStatusCommand:
        DoSPGetStatus(sess, (ASPGetStatusRec *)commandRec);
        break;
    case aspOpenSessionCommand:
        DoSPOpenSession(sess);
        break;
    case aspCloseSessionCommand:
        DoSPCloseSession(sess);
        break;
    case aspCommandCommand:
        DoSPCommand(sess, (ASPCommandRec *)commandRec);
        break;
    case aspWriteCommand:
        DoSPWrite(sess, (ASPWriteRec *)commandRec);
        break;
    }
    
    if ((commandRec->async & AT_ASYNC) && sess->commandPending) {
        commandRec->result = aspBusyErr;  // indicate call in process
        goto ret;
    }
    
    // if we're here, the call is synchronous -- we must complete it
    
    if (commandRec->command == aspCloseSessionCommand) {
        /* We don't wait for a reply to close */
        memset(&sess->reply, 0, sizeof(sess->reply));
        FinishASPCommand(sess);
    } else {
        lastActivityTime = GetTick();
        lastReadCount = sess->readCount;
        i = 0;
        while (sess->commandPending) {
            PollForData(sess);
            if (sess->readCount != lastReadCount) {
                lastReadCount = sess->readCount;
                lastActivityTime = GetTick();
                i = 0;
            } else if (GetTick() - lastActivityTime > 30*60) {
                if (i < 5) {
                    i++;
                } else {
                    FlagFatalError(sess, aspNoRespErr);
                }
            }
        }
    }

ret:
    /*
     * If requested by user, we tell the server we are "going to sleep"
     * after every command we send.  This avoids having the server 
     * disconnect us because we can't send tickles for a while.
     *
     * This implementation differs from Apple's specifications in a couple
     * respects, but matches the actual behavior of the servers I've tried:
     *
     * -Apple's docs say FPZzzzz was introduced with AFP 2.3, but Netatalk and
     *  OS X 10.5 support it with AFP 2.2 and doesn't support AFP 2.3 at all.
     * -Apple's docs show no reply block for FPZzzzz, but Netatalk and 
     *  OS X 10.5 send a reply with four bytes of data.
     */
    if ((sess->atipMapping.flags & fFakeSleep)
        && sess->loggedIn && commandRec->result == 0 
        && (commandRec->command == aspCommandCommand
            || commandRec->command == aspWriteCommand)
        && commandRec != (SPCommandRec*)&sess->sleepCommandRec)
    {
        commandRec = (SPCommandRec*)&sess->sleepCommandRec;
        goto top;
    }
    
    RestoreStateReg(stateReg);
    return 0;

callOrig:
    RestoreStateReg(stateReg);
    return oldCmds[commandRec->command];
}
#pragma databank 0

static void DoSPGetStatus(Session *sess, ASPGetStatusRec *commandRec) {
    static const Word kFPGetSrvrInfo = 15;
    sess->request.flags = DSI_REQUEST;
    sess->request.command = DSIGetStatus;
    sess->request.requestID = htons(sess->nextRequestID++);
    sess->request.writeOffset = 0;
    sess->request.totalDataLength = htonl(2);
    sess->replyBuf = (void*)commandRec->bufferAddr;
    sess->replyBufLen = commandRec->bufferLength;
    
    SendDSIMessage(sess, &sess->request, &kFPGetSrvrInfo, NULL);
}

static void DoSPOpenSession(Session *sess) {
    sess->request.flags = DSI_REQUEST;
    sess->request.command = DSIOpenSession;
    sess->request.requestID = htons(sess->nextRequestID++);
    sess->request.writeOffset = 0;
    sess->request.totalDataLength = 0;
    sess->replyBuf = NULL;
    sess->replyBufLen = 0;
    
    SendDSIMessage(sess, &sess->request, NULL, NULL);
}

static void DoSPCloseSession(Session *sess) {
    sess->request.flags = DSI_REQUEST;
    sess->request.command = DSICloseSession;
    sess->request.requestID = htons(sess->nextRequestID++);
    sess->request.writeOffset = 0;
    sess->request.totalDataLength = 0;
    sess->replyBuf = NULL;
    sess->replyBufLen = 0;
    
    SendDSIMessage(sess, &sess->request, NULL, NULL);
}

static void DoSPCommand(Session *sess, ASPCommandRec *commandRec) {
    LongWord readSize;

    sess->request.flags = DSI_REQUEST;
    sess->request.command = DSICommand;
    sess->request.requestID = htons(sess->nextRequestID++);
    sess->request.writeOffset = 0;
    sess->request.totalDataLength = htonl(commandRec->cmdBlkLength);
    sess->replyBuf = (void*)(commandRec->replyBufferAddr & 0x00FFFFFF);
    sess->replyBufLen = commandRec->replyBufferLen;
    
    /*
     * If the client is requesting to read more data than will fit in its
     * buffer, reduce the amount of data requested and/or assume the buffer
     * is really larger than specified.
     * 
     * This is necessary because ASP request and reply bodies are limited
     * to a "quantum size" of 4624 bytes. Even if more data than that is
     * requested, only that amount will be returned in a single reply.
     * If the full read count requested is larger, it simply serves as a
     * hint about the total amount that will be read across a series of
     * FPRead requests.  The AppleShare FST relies on this behavior and
     * only specifies a buffer size of 4624 bytes for such requests.
     * However, it appears that it actually always has a buffer big enough
     * for the full read amount, and it can deal with receiving up to
     * 65535 bytes at once.
     *
     * DSI does not have this "quantum size" limitation, so the full
     * requested amount would be returned but then not fit in the specified
     * buffer size.  To deal with this, we can reduce the amount of data
     * requested from the server and/or just assume we actually have a 
     * bigger reply buffer than specified (up to 65535 bytes).  The former
     * approach is safer, but allowing larger reads gives significantly
     * better performance and seems to work OK with the AppleShare FST,
     * so we offer options for both approaches.
     *
     * A similar issue could arise with FPEnumerate requests, but it
     * actually doesn't in the case of the requests generated by the
     * AppleShare FST, so we don't try to address them for now.
     */
    if (commandRec->cmdBlkLength == sizeof(FPReadRec)
        && ((FPReadRec*)commandRec->cmdBlkAddr)->CommandCode == kFPRead)
    {
        readSize = ntohl(((FPReadRec*)commandRec->cmdBlkAddr)->ReqCount);
        if (readSize > sess->replyBufLen) {
            if (sess->atipMapping.flags & fLargeReads) {
                if (readSize <= 0xFFFF) {
                    sess->replyBufLen = readSize;
                } else {
                    sess->replyBufLen = 0xFFFF;
                    ((FPReadRec*)commandRec->cmdBlkAddr)->ReqCount = 
                        htonl(0xFFFF);
                }
            } else {
                ((FPReadRec*)commandRec->cmdBlkAddr)->ReqCount = 
                    htonl(sess->replyBufLen);
            }
        }
    }
    /*
     * If requested, replace AFP 2.0 login requests with otherwise-identical
     * AFP 2.2 login requests.  This provides compatibility with some servers
     * that don't support AFP 2.0 over TCP.  The protocols are similar enough
     * that it seems to work, although there could be issues.
     */ 
    else if ((sess->atipMapping.flags & fForceAFP22)
             && commandRec->cmdBlkLength >= sizeof(afp20VersionStr)
             && *((Byte*)commandRec->cmdBlkAddr) == kFPLogin
             && commandRec->cmdBlkLength <= sizeof(loginBuf)
             && strncasecmp(afp20VersionStr, (Byte*)commandRec->cmdBlkAddr + 1, 
                        sizeof(afp20VersionStr) - 1) == 0)
    {
        memcpy(loginBuf, (Byte*)commandRec->cmdBlkAddr,
               commandRec->cmdBlkLength);
        loginBuf[sizeof(afp20VersionStr)-sizeof(afp22VersionStr)] = kFPLogin;
        memcpy(&loginBuf[sizeof(afp20VersionStr)-sizeof(afp22VersionStr)+1],
               afp22VersionStr,
               sizeof(afp22VersionStr) - 1);

        sess->request.totalDataLength =
            htonl(commandRec->cmdBlkLength
                  -sizeof(afp20VersionStr)+sizeof(afp22VersionStr));
        SendDSIMessage(sess, &sess->request, 
                       loginBuf+sizeof(afp20VersionStr)-sizeof(afp22VersionStr),
                       NULL);
        return;
    }
    
    /* Mask off high byte of addresses because PFI (at least) may
     * put junk in them, and this can cause Marinetti errors. */
    SendDSIMessage(sess, &sess->request,
                   (void*)(commandRec->cmdBlkAddr & 0x00FFFFFF), NULL);
}

static void DoSPWrite(Session *sess, ASPWriteRec *commandRec) {
    LongWord dataLength;

    sess->request.flags = DSI_REQUEST;
    sess->request.command = DSIWrite;
    sess->request.requestID = htons(sess->nextRequestID++);
    sess->request.writeOffset = htonl(commandRec->cmdBlkLength);
    dataLength = commandRec->writeDataLength;
    
    /*
     * If requested, limit the data size of a single DSIWrite command to 4623.
     * This is necessary to make writes work on OS X (and others?).
     */
    if (!(sess->atipMapping.flags & fLargeWrites) 
        && dataLength > ASPQuantumSize - 1)
    {
        dataLength = ASPQuantumSize - 1;
        ((FPReadRec*)commandRec->cmdBlkAddr)->ReqCount = htonl(dataLength);
    }
    
    sess->request.totalDataLength = 
        htonl(dataLength + commandRec->cmdBlkLength);
    sess->replyBuf = (void*)(commandRec->replyBufferAddr & 0x00FFFFFF);
    sess->replyBufLen = commandRec->replyBufferLen;
    
    SendDSIMessage(sess, &sess->request,
                   (void*)(commandRec->cmdBlkAddr & 0x00FFFFFF),
                   (void*)(commandRec->writeDataAddr & 0x00FFFFFF));
}


void FlagFatalError(Session *sess, Word errorCode) {
    sess->dsiStatus = error;
    if (errorCode == 0) {
        // TODO deduce better error code from Marinetti errors?
        errorCode = aspNetworkErr;
    }

    if (sess->commandPending) {
        CompleteCurrentASPCommand(sess, errorCode);
    }
    
    EndASPSession(sess, aspAttenTimeout, TRUE);
}


// Fill in any necessary data in the ASP command rec for a successful return
void FinishASPCommand(Session *sess) {
    LongWord dataLength;
    
    dataLength = ntohl(sess->reply.totalDataLength);
    if (dataLength > 0xFFFF) {
        // The IIgs ASP interfaces can't represent lengths over 64k.
        // This should be detected as an error earlier, but let's make sure.
        CompleteCurrentASPCommand(sess, aspSizeErr);
        return;
    }

    switch(sess->spCommandRec->command) {
    case aspGetStatusCommand:
        ((ASPGetStatusRec*)(sess->spCommandRec))->dataLength = dataLength;
        break;
    case aspOpenSessionCommand:
        ((ASPOpenSessionRec*)(sess->spCommandRec))->refNum = 
            (sess - &sessionTbl[0]) + SESSION_NUM_START;
        break;
    case aspCloseSessionCommand:
        break;
    case aspCommandCommand:
        ((ASPCommandRec*)(sess->spCommandRec))->cmdResult =
            sess->reply.errorCode;
        ((ASPCommandRec*)(sess->spCommandRec))->replyLength = dataLength;
        
        /*
         * If requested by user, ignore errors when setting ProDOS-style
         * filetype info.  This allows files to be written to servers running
         * old versions of OS X, although filetypes are not set correctly.
         */
        if ((sess->atipMapping.flags & fIgnoreFileTypeErrors)
            && ((ASPCommandRec*)(sess->spCommandRec))->cmdBlkLength
                > sizeof(FPSetFileDirParmsRec)
            && *(Byte*)(((ASPCommandRec*)(sess->spCommandRec))->cmdBlkAddr)
                == kFPSetFileDirParms
            && (((FPSetFileDirParmsRec*)(((ASPCommandRec*)(sess->spCommandRec))
                ->cmdBlkAddr))->Bitmap & htons(kFPProDOSInfoBit))
            && sess->reply.errorCode == htonl((LongWord)kFPBitmapErr))
        {
            ((ASPCommandRec*)(sess->spCommandRec))->cmdResult = 0;
        }
        
        break;
    case aspWriteCommand:
        ((ASPWriteRec*)(sess->spCommandRec))->cmdResult =
            sess->reply.errorCode;
        ((ASPWriteRec*)(sess->spCommandRec))->replyLength = dataLength;
        ((ASPWriteRec*)(sess->spCommandRec))->writtenLength =
            ntohl(sess->request.totalDataLength)
            - ((ASPWriteRec*)(sess->spCommandRec))->cmdBlkLength;
        break;
    }

complete:
    CompleteCurrentASPCommand(sess, 0);
}

/* Actions to complete a command, whether successful or not */
void CompleteCurrentASPCommand(Session *sess, Word result) {
    SPCommandRec *commandRec;

    commandRec = sess->spCommandRec;

    if (sess->spCommandRec->command == aspGetStatusCommand 
        || sess->spCommandRec->command == aspCloseSessionCommand)
    {
        EndASPSession(sess, 0, TRUE);
    } else {
        sess->commandPending = FALSE;
        if (sess->dsiStatus != error) {
            sess->dsiStatus = awaitingHeader;
            InitReadTCP(sess, DSI_HEADER_SIZE, &sess->reply);
        }
    }
    
    commandRec->result = result;
    
    if ((commandRec->async & AT_ASYNC) && commandRec->completionPtr != NULL) {
        CallCompletionRoutine((void *)commandRec->completionPtr);
    }
}

/* For use in error cases not involving the session's current command */
static void CompleteASPCommand(SPCommandRec *commandRec, Word result) {
    commandRec->result = result;
    
    if ((commandRec->async & AT_ASYNC) && commandRec->completionPtr != NULL) {
        CallCompletionRoutine((void *)commandRec->completionPtr);
    }
}


void EndASPSession(Session *sess, Byte attentionCode, Boolean doLogout) {
    if (attentionCode != 0) {
        CallAttentionRoutine(sess, attentionCode, 0);
    }
    
    if (doLogout)
        EndTCPConnection(sess);
    memset(sess, 0, sizeof(*sess));
}

void CallAttentionRoutine(Session *sess, Byte attenType, Word atten) {
    if (sess->attention == NULL)
        return;
    
    sess->attention->sessionRefNum = (sess - sessionTbl) + SESSION_NUM_START;
    sess->attention->attenType = attenType;
    sess->attention->atten = atten;
    
    /* Call attention routine like completion routine */
    CallCompletionRoutine((void *)(sess->attention + 1));
}

static void InitSleepRec(Session *sess, Byte sessRefNum) {
    sess->sleepCommandRec.async = 0;
    sess->sleepCommandRec.command = aspCommandCommand;
    sess->sleepCommandRec.completionPtr = 0;
    sess->sleepCommandRec.refNum = sessRefNum;
    sess->sleepCommandRec.cmdBlkLength = sizeof(sess->fpZzzzzRec);
    sess->sleepCommandRec.cmdBlkAddr = (LongWord)&sess->fpZzzzzRec;
    sess->sleepCommandRec.replyBufferLen = sizeof(sess->fpZzzzzResponseRec);
    sess->sleepCommandRec.replyBufferAddr = (LongWord)&sess->fpZzzzzResponseRec;
    sess->fpZzzzzRec.CommandCode = kFPZzzzz;
    sess->fpZzzzzRec.Flag = 0;
}

void PollAllSessions(void) {
    unsigned int i;

    for (i = 0; i < MAX_SESSIONS; i++) {
        switch (sessionTbl[i].dsiStatus) {
        case awaitingHeader:
        case awaitingPayload:
            PollForData(&sessionTbl[i]);
            break;
        
        case needsReset:
            EndASPSession(&sessionTbl[i], 0, TRUE);
            break;
        }
    }
}

/* Close all sessions -- used when we're shutting down */
void CloseAllSessions(Byte attentionCode, Boolean doLogout) {
    unsigned int i;
    Session *sess;

    for (i = 0; i < MAX_SESSIONS; i++) {
        sess = &sessionTbl[i];
        if (sess->dsiStatus != unused) {
            if (doLogout)
                DoSPCloseSession(sess);
            EndASPSession(sess, attentionCode, doLogout);
        }
    }
}

/*
 * Reset the state of all sessions.
 * Used when control-reset is pressed in P8 mode.  We can't call Marinetti
 * in P8 mode, but we also don't want to just leak any ipids in use, so
 * we flag sessions that should be reset when we're back in GS/OS.
 */
#pragma databank 1
void ResetAllSessions(void) {
    unsigned int i;

    for (i = 0; i < MAX_SESSIONS; i++) {
        if (sessionTbl[i].dsiStatus != unused) {
            sessionTbl[i].dsiStatus = needsReset;
        }
    }
}
#pragma databank 0
