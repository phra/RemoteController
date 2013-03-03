#define PKTDATA 1
#include <ctype.h> 
#include <stdio.h>
#include <initguid.h>
#include <winsock2.h>
#include <ws2bth.h>
#include <strsafe.h>
#include <intsafe.h>
#include <iostream>
#include <io.h>
#include <fcntl.h>
#include <assert.h>
#include <stddef.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#pragma pack(1)
typedef struct header {
	int size;
	int type;
} header_t;

#pragma once
class pacco
{
private:
	int type;
	char* data;
	int datalen;
public:
	pacco(int tipo, char* raw, int len);
	~pacco(void);
	char* getData(void);
	header_t* getSerializedHeader(void);
};

