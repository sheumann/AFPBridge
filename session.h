#ifndef SESSION_H
#define SESSION_H

#include <types.h>
#include "dsiproto.h"

/* Common portion of records for the various SP... commands */
typedef struct SPCommandRec {
    Byte async;
    Byte command;
    Word result;
    /* Below are in records for several ASP commands, but not all. */
    LongWord completionPtr;
    Byte refNum;
} SPCommandRec;

typedef enum DSISessionStatus {
    unused = 0,
    awaitingHeader,
    awaitingPayload,
    error
} DSISessionStatus;

typedef struct Session {
    /* Marinetti TCP connection status */
    Word ipid;
    Boolean tcpLoggedIn;
    
    DSISessionStatus dsiStatus;
    
    /* Information on command currently being processed, if any. */
    Boolean commandPending;
    SPCommandRec *spCommandRec;
    DSIRequestHeader request;
    
    DSIReplyHeader reply;
    
    Word nextRequestID;
    
    /* Buffer to hold reply payload data */
    void *replyBuf;
    Word replyBufLen;
    
    /* Buffer to hold unusable payload data that doesn't fit in replyBuf */
    void *junkBuf;
    
    /* ReadTCP status */
    LongWord readCount;
    Byte *readPtr;
    
    /* Set by DSIAttention */
    Word attentionCode;
    
    /* Marinetti error codes, both the tcperr* value and any tool error */
    Word tcperr;
    Word toolerr;
} Session;

#endif
