#ifndef DSIPROTO_H
#define DSIPROTO_H

#include <types.h>

typedef struct DSIRequestHeader {
    Byte flags;
    Byte command;
    Word requestID;
    LongWord writeOffset;
    LongWord totalDataLength;
    LongWord reserved;
} DSIRequestHeader;

typedef struct DSIReplyHeader {
    Byte flags;
    Byte command;
    Word requestID;
    LongWord errorCode;
    LongWord totalDataLength;
    LongWord reserved;
} DSIReplyHeader;

#define DSI_HEADER_SIZE 16

/* flags values */
#define DSI_REQUEST 0
#define DSI_REPLY 1

/* DSI command codes */
#define DSICloseSession 1
#define DSICommand      2
#define DSIGetStatus    3
#define DSIOpenSession  4
#define DSITickle       5
#define DSIWrite        6
#define DSIAttention    8

/* The attention quantum supported by this implementation */
#define DSI_ATTENTION_QUANTUM 2

#endif
