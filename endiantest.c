#include "endian.h"
#include <stdio.h>

int main(void)
{
	printf("%lx\n", htonl(0x12345678));
	printf("%x\n", htons(0xabcd));
}
