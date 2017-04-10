#ifndef ATIPMAPPING_H
#define ATIPMAPPING_H

#include <types.h>
#include <appletalk.h>

typedef struct ATIPMapping {
    /* AppleTalk address/socket */
    Word networkNumber; /* in network byte order */
    Byte node;
    Byte socket;
    
    /* IP address/port */
    LongWord ipAddr;
    Word port;
} ATIPMapping;

extern struct ATIPMapping atipMapping;

LongWord DoLookupName(NBPLookupNameRec *commandRec);

#endif
