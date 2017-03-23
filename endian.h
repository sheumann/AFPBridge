#ifndef ENDIAN_H
#define ENDIAN_H

#include <types.h>

/*
 * Undefine these in case they're defined as macros.
 * (In particular, GNO has broken macro definitions for these.)
 */
#undef htons
#undef ntohs
#undef htonl
#undef ntohl

Word htons(Word);
Word ntohs(Word);
LongWord htonl(LongWord);
LongWord ntohl(LongWord);

#endif
