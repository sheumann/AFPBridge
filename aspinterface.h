#ifndef ASPINTERFACE_H
#define ASPINTERFACE_H

#include "session.h"

/* async flag values */
#define AT_SYNC     0
#define AT_ASYNC    0x80

#define aspBusyErr 0x07FF   /* temp result code for async call in process */

#define SESSION_NUM_START 0xF8
#define MAX_SESSIONS (256 - SESSION_NUM_START)

extern Session sessionTbl[MAX_SESSIONS];

LongWord DispatchASPCommand(SPCommandRec *commandRec);
void CompleteCurrentASPCommand(Session *sess, Word result);
void FinishASPCommand(Session *sess);
void FlagFatalError(Session *sess, Word errorCode);
void EndASPSession(Session *sess, Byte attentionCode);
void CallAttentionRoutine(Session *sess, Byte attenType, Word atten);
void PollAllSessions(void);
void CloseAllSessions(Byte attentionCode);
void ResetAllSessions(void);

#endif
