#pragma noroot

#include <AppleTalk.h>
#include <string.h>

#include "session.h"
#include "aspinterface.h"
#include "dsi.h"
#include "tcpconnection.h"
#include "endian.h"
#include "readtcp.h"
#include "asmglue.h"

static void CompleteCommand(Session *sess, Word result);
static void EndSession(Session *sess, Boolean callAttnRoutine);

static void DoSPGetStatus(Session *sess, ASPGetStatusRec *commandRec);
static void DoSPOpenSession(Session *sess, ASPOpenSessionRec *commandRec);
static void DoSPCloseSession(Session *sess, ASPCloseSessionRec *commandRec);
static void DoSPCommand(Session *sess, ASPCommandRec *commandRec);
static void DoSPWrite(Session *sess, ASPWriteRec *commandRec);


Session sessionTbl[MAX_SESSIONS];

#pragma databank 1
void DispatchASPCommand(SPCommandRec *commandRec) {
    Session *sess;
    unsigned int i;
    Word stateReg;
    
    stateReg = ForceRomIn();

    if (commandRec->command == aspGetStatusCommand 
        || commandRec->command==aspOpenSessionCommand)
    {
        for (i = 0; i < MAX_SESSIONS; i++) {
            if (sessionTbl[i].dsiStatus == unused)
                break;
        }
        if (i == MAX_SESSIONS) {
            CompleteCommand(sess, aspTooManySessions);
            goto ret;
        }
        sess = &sessionTbl[i];
        sess->spCommandRec = commandRec;
        
        if (!StartTCPConnection(sess)) {
            FlagFatalError(sess, 0);
            goto ret;
        }
        sess->dsiStatus = awaitingHeader;
        InitReadTCP(sess, DSI_HEADER_SIZE, &sess->reply);
    } else {
        if (commandRec->refNum < SESSION_NUM_START) {
            // TODO call original AppleTalk routine (or do it earlier)
            commandRec->result = atInvalidCmdErr;
            goto ret;
        }
        i = commandRec->refNum - SESSION_NUM_START;
        sess = &sessionTbl[i];
        sess->spCommandRec = commandRec;
    }

    // TODO properly handle all cases of getting a command while
    // one is in progress
    if (commandRec->command != aspCloseSessionCommand) {
        if (sess->commandPending) {
            CompleteCommand(sess, aspSessNumErr);
            goto ret;
        }
        sess->commandPending = TRUE;
    }
    
    switch (commandRec->command) {
    case aspGetStatusCommand:
        DoSPGetStatus(sess, (ASPGetStatusRec *)commandRec);
        break;
    case aspOpenSessionCommand:
        DoSPOpenSession(sess, (ASPOpenSessionRec *)commandRec);
        break;
    case aspCloseSessionCommand:
        DoSPCloseSession(sess, (ASPCloseSessionRec *)commandRec);
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
        FinishASPCommand(sess);
    } else {
        while (sess->commandPending) {
            PollForData(sess);
        }
    }

ret:    
    RestoreStateReg(stateReg);
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

static void DoSPOpenSession(Session *sess, ASPOpenSessionRec *commandRec) {
    sess->request.flags = DSI_REQUEST;
    sess->request.command = DSIOpenSession;
    sess->request.requestID = htons(sess->nextRequestID++);
    sess->request.writeOffset = 0;
    sess->request.totalDataLength = 0;
    sess->replyBuf = NULL;
    sess->replyBufLen = 0;
    
    SendDSIMessage(sess, &sess->request, NULL, NULL);
}

static void DoSPCloseSession(Session *sess, ASPCloseSessionRec *commandRec) {
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
    sess->request.flags = DSI_REQUEST;
    sess->request.command = DSICommand;
    sess->request.requestID = htons(sess->nextRequestID++);
    sess->request.writeOffset = 0;
    sess->request.totalDataLength = htonl(commandRec->cmdBlkLength);
    sess->replyBuf = (void*)(commandRec->replyBufferAddr & 0x00FFFFFF);
    sess->replyBufLen = commandRec->replyBufferLen;
    
    /* Mask off high byte of addresses because PFI (at least) may
     * put junk in them, and this can cause Marinetti errors. */
    SendDSIMessage(sess, &sess->request,
                   (void*)(commandRec->cmdBlkAddr & 0x00FFFFFF), NULL);
}

static void DoSPWrite(Session *sess, ASPWriteRec *commandRec) {
    sess->request.flags = DSI_REQUEST;
    sess->request.command = DSIWrite;
    sess->request.requestID = htons(sess->nextRequestID++);
    sess->request.writeOffset = htonl(commandRec->cmdBlkLength);
    sess->request.totalDataLength =
        htonl(commandRec->cmdBlkLength + commandRec->writeDataLength);
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
        CompleteCommand(sess, errorCode);
    }
    
    EndSession(sess, TRUE);
}


// Fill in any necessary data in the ASP command rec for a successful return
void FinishASPCommand(Session *sess) {
    LongWord dataLength;
    
    dataLength = ntohl(sess->reply.totalDataLength);
    if (dataLength > 0xFFFF) {
        // The IIgs ASP interfaces can't represent lengths over 64k.
        // This should be detected as an error earlier, but let's make sure.
        sess->spCommandRec->result = aspSizeErr;
        goto complete;
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
        break;
    case aspWriteCommand:
        ((ASPWriteRec*)(sess->spCommandRec))->cmdResult =
            sess->reply.errorCode;
        ((ASPWriteRec*)(sess->spCommandRec))->replyLength = dataLength;
        ((ASPWriteRec*)(sess->spCommandRec))->writtenLength =
            ((ASPWriteRec*)(sess->spCommandRec))->writeDataLength;
        break;
    }

complete:
    CompleteCommand(sess, 0);
}

/* Actions to complete a command, whether successful or not */
static void CompleteCommand(Session *sess, Word result) {
    SPCommandRec *commandRec;

    commandRec = sess->spCommandRec;

    if (sess->spCommandRec->command == aspGetStatusCommand 
        || sess->spCommandRec->command == aspCloseSessionCommand)
    {
        EndSession(sess, FALSE);
    } else {
        sess->commandPending = FALSE;
        if (sess->dsiStatus != error) {
            sess->dsiStatus = awaitingHeader;
            InitReadTCP(sess, DSI_HEADER_SIZE, &sess->reply);
        }
    }
    
    commandRec->result = result;
    
    // TODO call completion routine
}


static void EndSession(Session *sess, Boolean callAttnRoutine) {
    if (callAttnRoutine) {
        // TODO call the attention routine to report end of session
    }
    
    EndTCPConnection(sess);
    memset(sess, 0, sizeof(*sess));
}

void PollAllSessions(void) {
    unsigned int i;

    for (i = 0; i < MAX_SESSIONS; i++) {
        if (sessionTbl[i].dsiStatus != unused) {
            PollForData(&sessionTbl[i]);
        }
    }
}
