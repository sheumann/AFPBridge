#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <appletalk.h>
#include "endian.h"

#define ENTITY_FIELD_MAX 32

typedef struct NBPLookupResultBuf {
    Word networkNum;
    Byte nodeNum;
    Byte socketNum;
    // First enumerator/name only
    Byte enumerator;
    EntName entName;
} NBPLookupResultBuf;

NBPLookupResultBuf lookupResultBuf;
NBPLookupNameRec lookupNameRec;
EntName entName;

char *object, *zone, *type;

int main(int argc, char **argv) {
    int i, j;
    int count;
    
    if (argc < 3) {
        fprintf(stderr, "Usage: afpmounter name zone\n");
        return;
    }
    
    object = argv[1];
    type = "AFPServer";
    zone = argv[2];
    if (strlen(object) > ENTITY_FIELD_MAX || strlen(zone) > ENTITY_FIELD_MAX) {
        fprintf(stderr, "Entity name too long (max 32 chars)\n");
        return;
    }

    lookupNameRec.async = 0;
    lookupNameRec.command = nbpLookupNameCommand;
    lookupNameRec.completionPtr = 0;
    lookupNameRec.entityPtr = (LongWord)&entName;
    lookupNameRec.rInterval = 1;
    lookupNameRec.rCount = 20;
    lookupNameRec.reserved = 0;
    lookupNameRec.bufferLength = sizeof(lookupResultBuf);
    lookupNameRec.bufferPtr = (LongWord)&lookupResultBuf;
    lookupNameRec.maxMatch = 1;
    
    i = 0;
    count = strlen(object);
    entName.buffer[i++] = count;
    for (j = 0; j < count; j++) {
        entName.buffer[i++] = object[j];
    }
    count = strlen(type);
    entName.buffer[i++] = count;
    for (j = 0; j < count; j++) {
        entName.buffer[i++] = type[j];
    }
    count = strlen(zone);
    entName.buffer[i++] = count;
    for (j = 0; j < count; j++) {
        entName.buffer[i++] = zone[j];
    }
    
    if (_CALLAT(&lookupNameRec)) {
        fprintf(stderr, "NBP lookup error: %04x\n", lookupNameRec.result);
        return;
    }
    
    printf("%i matches\n", lookupNameRec.actualMatch);
    if (lookupNameRec.actualMatch > 0) {
        printf("network = %u, node = %u, socket = %u\n",
            ntohs(lookupResultBuf.networkNum),
            lookupResultBuf.nodeNum,
            lookupResultBuf.socketNum);
    }
    
}
