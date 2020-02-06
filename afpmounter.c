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

char *object, *zone, *type;

NBPLookupResultBuf lookupResultBuf;
NBPLookupNameRec lookupNameRec;
EntName entName;

#define kFPLogin 18
char *afpVersionStr, *uamStr;
unsigned char fpLoginCmdRec[128];
unsigned char replyBuffer[1024];

PFILogin2Rec login2Rec;

#define VOL_NAME_MAX 27
#define VOL_PASSWORD_MAX 8
PFIMountvolRec mountVolRec;
unsigned char volName[VOL_NAME_MAX  + 1];
unsigned char volPassword[VOL_PASSWORD_MAX] = {0};

PFILogoutRec logoutRec;

int main(int argc, char **argv) {
    int i, j;
    int count;
    int zoneNameOffset;
    Word atRetCode;
    
    if (argc < 4 || argc > 5) {
        fprintf(stderr, "Usage: afpmounter name zone volume [volPassword]\n");
        return EXIT_FAILURE;
    }
    
    object = argv[1];
    type = "AFPServer";
    zone = argv[2];
    if (strlen(object) > ENTITY_FIELD_MAX || strlen(zone) > ENTITY_FIELD_MAX) {
        fprintf(stderr, "Entity name too long (max 32 chars)\n");
        return EXIT_FAILURE;
    }

    count = strlen(argv[3]);
    if (count > VOL_NAME_MAX) {
        fprintf(stderr, "Volume name too long\n");
        return EXIT_FAILURE;
    }
    volName[0] = count;
    strncpy(volName+1, argv[3], count);
    
    if (argc >= 5) {
        count = strlen(argv[4]);
        if (count > VOL_PASSWORD_MAX) {
            fprintf(stderr, "Volume password too long\n");
            return EXIT_FAILURE;
        }
        strncpy(volPassword, argv[4], count);
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
    zoneNameOffset = i;
    count = strlen(zone);
    entName.buffer[i++] = count;
    for (j = 0; j < count; j++) {
        entName.buffer[i++] = zone[j];
    }
    
    atRetCode = _CALLAT(&lookupNameRec);
    if (atRetCode != 0) {
        fprintf(stderr, "NBP lookup error: %04x\n", lookupNameRec.result);
        return EXIT_FAILURE;
    }
    
    if (lookupNameRec.actualMatch == 0) {
        fprintf(stderr, "The specified server could not be found\n");
        return EXIT_FAILURE;
    }
    
#if 0
    printf("%i matches\n", lookupNameRec.actualMatch);
    printf("network = %u, node = %u, socket = %u\n",
        ntohs(lookupResultBuf.networkNum),
        lookupResultBuf.nodeNum,
        lookupResultBuf.socketNum);
#endif
    
    i = 0;
    fpLoginCmdRec[i++] = kFPLogin;
    afpVersionStr = "AFPVersion 2.0";
    count = strlen(afpVersionStr);
    fpLoginCmdRec[i++] = count;
    for(j = 0; j < count; j++) {
        fpLoginCmdRec[i++] = afpVersionStr[j];
    }
    uamStr = "No User Authent";
    count = strlen(uamStr);
    fpLoginCmdRec[i++] = count;
    for(j = 0; j < count; j++) {
        fpLoginCmdRec[i++] = uamStr[j];
    }
    
    login2Rec.async = 0;
    login2Rec.command = pfiLogin2Command;
    login2Rec.networkID = lookupResultBuf.networkNum;
    login2Rec.nodeID = lookupResultBuf.nodeNum;
    login2Rec.socketID = lookupResultBuf.socketNum;
    login2Rec.cmdBufferLength = i;
    login2Rec.cmdBufferPtr = (LongWord)&fpLoginCmdRec;
    login2Rec.replyBufferLen = sizeof(replyBuffer);
    login2Rec.replyBufferPtr = (LongWord)&replyBuffer;
    login2Rec.attnRtnAddr = 0;
    login2Rec.serverName = &entName.buffer[0];
    login2Rec.zoneName = &entName.buffer[zoneNameOffset];
    login2Rec.afpVersionNum = pfiAFPVersion20;
    
    atRetCode = _CALLAT(&login2Rec);
    if (atRetCode != 0) {
        fprintf(stderr, "Login failure: %04x\n", login2Rec.result);
        if (login2Rec.result == pfiLoginContErr)
            goto logout_and_exit;
        return EXIT_FAILURE;
    }
    
    mountVolRec.async = 0;
    mountVolRec.command = pfiMountVolCommand;
    mountVolRec.sessRefID = login2Rec.sessRefID;
    mountVolRec.mountflag = pfiMountMask | pfiPasswordMask;
    mountVolRec.volNamePtr = (LongWord)&volName;
    mountVolRec.passwordPtr = (LongWord)&volPassword;
    
    atRetCode = _CALLAT(&mountVolRec);
    if (atRetCode != 0) {
        fprintf(stderr, "Failed to mount volume: %04x\n", mountVolRec.result);
        goto logout_and_exit;
    }
    
    /* success - leave the volume mounted */
    return EXIT_SUCCESS;
    
logout_and_exit:
    logoutRec.async = 0;
    logoutRec.command = pfiLogOutCommand;
    logoutRec.sessRefID = login2Rec.sessRefID;

    _CALLAT(&logoutRec);
    return EXIT_FAILURE;
}
