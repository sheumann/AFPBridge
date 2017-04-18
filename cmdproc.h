#ifndef CMDPROC_H
#define CMDPROC_H

void cmdProc(void);
void nbpCmdProc(void);
void pfiLoginCmdProc(void);
extern LongWord jslOldPFILogin;
void pfiLogin2CmdProc(void);
extern LongWord jslOldPFILogin2;
void pfiListSessions2CmdProc(void);
extern LongWord jslOldPFIListSessions2;
void CallCompletionRoutine(void *);

#endif
