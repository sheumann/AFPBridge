#ifndef TCPCONNECTION_H
#define TCPCONNECTION_H

#include "session.h"

Word StartTCPConnection(Session *sess);
void EndTCPConnection(Session *sess);

#endif
