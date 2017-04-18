#include <stdio.h>
#include <appletalk.h>

typedef struct ListSessions2ResultRec {
    Byte sessionRefNum;
    Byte slotDrive;
    char volumeName[28];
    Word volumeID;
    char serverName[32];
    char zoneName[33];
} ListSessions2ResultRec;

ListSessions2ResultRec ls2Results[20];

int main(void)
{
    PFIListSessions2Rec listSessions2Rec;
    int i;
    
    listSessions2Rec.async = 0;
    listSessions2Rec.command = pfiListSessions2Command;
    listSessions2Rec.bufferLength = sizeof(ls2Results);
    listSessions2Rec.bufferPtr = (LongWord)&ls2Results;
    i = _CALLAT(&listSessions2Rec);
    if (i != 0) {
        fprintf(stderr, "Error %04x\n", listSessions2Rec.result);
        return;
    }
    
    for (i = 0; i < listSessions2Rec.entriesRtn; i++) {
        printf("Session=%i, Volume=%b, Server=%b, Zone=%b\n",
            ls2Results[i].sessionRefNum, ls2Results[i].volumeName,
            ls2Results[i].serverName, ls2Results[i].zoneName);
    }
}
