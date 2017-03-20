#pragma noroot

#include <AppleTalk.h>
#include <string.h>

#include "session.h"
#include "aspinterface.h"
#include "dsi.h"
#include "tcpconnection.h"
#include "endian.h"
#include "readtcp.h"

void DoSPGetStatus(Session *sess, ASPGetStatusRec *commandRec);
void DoSPOpenSession(Session *sess, ASPOpenSessionRec *commandRec);
void DoSPCloseSession(Session *sess, ASPCloseSessionRec *commandRec);
void DoSPCommand(Session *sess, ASPCommandRec *commandRec);
void DoSPWrite(Session *sess, ASPWriteRec *commandRec);


Session sessionTbl[MAX_SESSIONS];

void DispatchASPCommand(SPCommandRec *commandRec) {
    Session *sess;
    unsigned int i;

    if (commandRec->command == aspGetStatusCommand 
        || commandRec->command==aspOpenSessionCommand)
    {
        for (i = 0; i < MAX_SESSIONS; i++) {
            if (sessionTbl[i].dsiStatus == unused)
                break;
        }
        if (i == MAX_SESSIONS) {
            commandRec->result = aspTooManySessions;
            CompleteCommand(sess);
            return;
        }
        sess = &sessionTbl[i];
        sess->spCommandRec = commandRec;
        
        if (!StartTCPConnection(sess)) {
            // Error code was set in TCPIPConnect
            CompleteCommand(sess);
            return;
        }
        sess->dsiStatus = awaitingHeader;
        InitReadTCP(sess, DSI_HEADER_SIZE, &sess->reply);
    } else {
        if (commandRec->refNum < SESSION_NUM_START) {
            // TODO call original AppleTalk routine (or do it earlier)
            return;
        }
        i = commandRec->refNum - SESSION_NUM_START;
        sess = &sessionTbl[i];
        sess->spCommandRec = commandRec;
    }

    // TODO properly handle all cases of getting a command while
    // one is in progress
    if (commandRec->command != aspCloseSessionCommand) {
        if (sess->commandPending) {
            commandRec->result = aspSessNumErr;
            CompleteCommand(sess);
            return;
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
        return;
    }
    
    // if we're here, the call is synchronous -- we must complete it
    
    if (commandRec->command == aspCloseSessionCommand) {
        FinishASPCommand(sess);
    } else {
        while (sess->commandPending) {
            PollForData(sess);
        }
    }
}

void DoSPGetStatus(Session *sess, ASPGetStatusRec *commandRec) {
    static const Word kFPGetSrvrInfo = 15;
    sess->request.flags = DSI_REQUEST;
    sess->request.command = DSIGetStatus;
    sess->request.requestID = htons(sess->nextRequestID++);
    sess->request.writeOffset = 0;
    sess->request.totalDataLength = htonl(2);
    sess->replyBuf = (void*)commandRec->bufferAddr;
    sess->replyBufLen = commandRec->bufferLength;
    
    SendDSIMessage(sess, &sess->request, &kFPGetSrvrInfo);
}

void DoSPOpenSession(Session *sess, ASPOpenSessionRec *commandRec) {
    sess->request.flags = DSI_REQUEST;
    sess->request.command = DSIOpenSession;
    sess->request.requestID = htons(sess->nextRequestID++);
    sess->request.writeOffset = 0;
    sess->request.totalDataLength = 0;
    sess->replyBuf = NULL;
    sess->replyBufLen = 0;
    
    SendDSIMessage(sess, &sess->request, NULL);
}

void DoSPCloseSession(Session *sess, ASPCloseSessionRec *commandRec) {
    sess->request.flags = DSI_REQUEST;
    sess->request.command = DSICloseSession;
    sess->request.requestID = htons(sess->nextRequestID++);
    sess->request.writeOffset = 0;
    sess->request.totalDataLength = 0;
    sess->replyBuf = NULL;
    sess->replyBufLen = 0;
    
    SendDSIMessage(sess, &sess->request, NULL);
}

void DoSPCommand(Session *sess, ASPCommandRec *commandRec) {
    // TODO
}

void DoSPWrite(Session *sess, ASPWriteRec *commandRec) {
    // TODO
}

// Fill in any necessary data in the ASP command rec for a successful return
void FinishASPCommand(Session *sess) {
    LongWord dataLength;
    
    dataLength = ntohl(sess->reply.totalDataLength);
    if (dataLength > 0xFFFF) {
        // The IIgs ASP interfaces can't represent lengths over 64k.
        // This should be detected as an error earlier, but let's make sure.
        sess->spCommandRec->result = aspSizeErr;
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
        break;
    case aspWriteCommand:
        // TODO
        break;
    }
    
    CompleteCommand(sess);
}

/* Actions to complete a command, whether successful or not */
void CompleteCommand(Session *sess) {
    if (sess->spCommandRec->command == aspGetStatusCommand 
        || sess->spCommandRec->command == aspCloseSessionCommand)
    {
        EndTCPConnection(sess);
    }
    
    // TODO call completion routine
    
    if (sess->spCommandRec->command == aspGetStatusCommand 
        || sess->spCommandRec->command == aspCloseSessionCommand)
    {
        memset(sess, 0, sizeof(*sess));
    } else {
        sess->commandPending = FALSE;
    }
}
