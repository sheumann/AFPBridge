#pragma noroot

#include <AppleTalk.h>
#include <stdlib.h>
#include <orca.h>
#include <tcpip.h>
#include "dsiproto.h"
#include "session.h"
#include "readtcp.h"
#include "dsi.h"
#include "endian.h"
#include "aspinterface.h"

static DSIRequestHeader tickleRequestRec = {
    DSI_REQUEST,    /* flags */
    DSITickle,      /* command */
    0,              /* requestID - set later */
    0,              /* writeOffset */
    0,              /* totalDataLength */
    0               /* reserved */
};

/* Actually a reply, but use DSIRequestHeader type so we can send it. */
static DSIRequestHeader attentionReplyRec = {
    DSI_REPLY,      /* flags */
    DSIAttention,   /* command */
    0,              /* requestID - set later */
    0,              /* errorCode */
    0,              /* totalDataLength */
    0               /* reserved */
};

void FlagFatalError(Session *sess, Word errorCode) {
    sess->dsiStatus = error;
    if (errorCode) {
        if (sess->commandStatus == commandPending) {
            sess->spCommandRec->result = errorCode;
        }
    } else {
        // TODO deduce error code from Marinetti errors
    }
    // TODO close TCP connection, anything else?

	// call completion routing if needed

    sess->commandStatus = commandDone;
}


void SendDSIMessage(Session *sess, DSIRequestHeader *header, void *payload) {
    Boolean hasData;
    
    hasData = header->totalDataLength != 0;

    sess->tcperr = TCPIPWriteTCP(sess->ipid, (void*)header,
                                 DSI_HEADER_SIZE,
                                 !hasData, FALSE);
    sess->toolerr = toolerror();
    if (sess->tcperr || sess->toolerr) {
        FlagFatalError(sess, 0);
        return;
    }
    
    if (hasData) {
        sess->tcperr = TCPIPWriteTCP(sess->ipid, payload,
                                     ntohl(header->totalDataLength),
                                     TRUE, FALSE);
        sess->toolerr = toolerror();
        if (sess->tcperr || sess->toolerr) {
            FlagFatalError(sess, 0);
            return;
        }
    }
}


void PollForData(Session *sess) {
    ReadStatus rs;
    LongWord dataLength;

    if (sess->dsiStatus != awaitingHeader && sess->dsiStatus != awaitingPayload)
        return;
 
top:
    rs = TryReadTCP(sess);
    if (rs != rsDone) {
        if (rs == rsError) FlagFatalError(sess, 0);
        return;
    }
    
    // If we're here, we successfully read something.
    
    if (sess->dsiStatus == awaitingHeader) {
        if (sess->reply.totalDataLength != 0) {
            dataLength = ntohl(sess->reply.totalDataLength);
            
            if (sess->commandStatus == commandPending
                && sess->reply.flags == DSI_REPLY
                && sess->reply.requestID == sess->request.requestID
                && sess->reply.command == sess->request.command) 
            {
                if (dataLength <= sess->replyBufLen) {
                    InitReadTCP(sess, dataLength, sess->replyBuf);
                    sess->dsiStatus = awaitingPayload;
                    goto top;
                } else {
                    // handle data too long
                    sess->junkBuf = malloc(dataLength);
                    if (sess->junkBuf == NULL) {
                        FlagFatalError(sess, atMemoryErr);
                        return;
                    } else {
                        InitReadTCP(sess, dataLength, sess->junkBuf);
                        sess->dsiStatus = awaitingPayload;
                        goto top;
                    }
                }
            }
            else if (sess->reply.flags == DSI_REQUEST
                     && sess->reply.command == DSIAttention
                     && dataLength <= DSI_ATTENTION_QUANTUM)
            {
                InitReadTCP(sess, DSI_ATTENTION_QUANTUM, &sess->attentionCode);
                sess->dsiStatus = awaitingPayload;
                goto top;
            }
            else
            {
                FlagFatalError(sess, aspSizeErr);
                return;
            }
        }
    }
    
    // If we're here, we got a full message.
    
    // Handle a command that is now done, if any.
    if (sess->commandStatus == commandPending
        && sess->reply.flags == DSI_REPLY
        && sess->reply.requestID == sess->request.requestID
        && sess->reply.command == sess->request.command)
    {
        if (sess->junkBuf != NULL) {
            free(sess->junkBuf);
            sess->junkBuf = NULL;
            if (sess->reply.command == DSIOpenSession) {
                // We ignore the DSIOpenSession options for now.
                // Maybe we should do something with them?
                FinishASPCommand(sess);
            } else {
                sess->spCommandRec->result = aspSizeErr;
            }
        }
    
        // TODO correct logic for all cases
        FinishASPCommand(sess);
        sess->commandStatus = commandDone;
        return;
    }
    //Handle a request from the server
    else if (sess->reply.flags == DSI_REQUEST)
    {
        if (sess->reply.command == DSIAttention) {
            attentionReplyRec.requestID = sess->reply.requestID;
            SendDSIMessage(sess, &attentionReplyRec, NULL);
            //TODO call attention routine.
        } else if (sess->reply.command == DSICloseSession) {
            // TODO handle close
        } else if (sess->reply.command == DSITickle) {
            tickleRequestRec.requestID = htons(sess->nextRequestID++);
            SendDSIMessage(sess, &tickleRequestRec, NULL);
        } else {
            FlagFatalError(sess, aspNetworkErr);
            return;
        }
    }
    else
    {
        FlagFatalError(sess, aspNetworkErr);
        return;
    }
}
