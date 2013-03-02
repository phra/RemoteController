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
#include "pacco.h"
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#pragma once
class BTsocket
{
public:
	BTsocket(void);
	~BTsocket(void);
	int BTbind(void);
	int BTregister(void);
	int BTlisten(void);
	int BTaccept(void);
	char* BTrecv(int size);
	pacco* BTrecv(void);
	void BTclose(void);
	int BTfindbyname(wchar_t* name);
	int BTinsertMACtarget(wchar_t* name);
	int BTconnect(void);
	int BTsend(char* buf, int len);
	int BTsend(pacco* pkt);
	
private:
	SOCKET sock;
	SOCKET clientsock;
	SOCKADDR_BTH bthaddr;
	SOCKADDR_BTH remotebthaddr;
};

