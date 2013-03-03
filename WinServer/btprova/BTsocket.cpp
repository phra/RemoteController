#include "StdAfx.h"
#include "BTsocket.h"
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
#include "pacco.h"
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

// {B62C4E8D-62CC-404b-BBBF-BF3E3BBB1374}
DEFINE_GUID(g_guidServiceClass, 0xb62c4e8d, 0x62cc, 0x404b, 0xbb, 0xbf, 0xbf, 0x3e, 0x3b, 0xbb, 0x13, 0x74);
#define INSTANCE_STRING (L"BTsocket.cpp")


BTsocket::BTsocket(void)
{
	WSADATA WSAData = {0};
	if (WSAStartup(MAKEWORD(2, 2), &WSAData) != 0) {
            wprintf(L"-FATAL- | Unable to initialize Winsock version 2.2\n");
	}
	sock = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
	assert(sock > 0);
	memset(&bthaddr,0,sizeof(bthaddr));
	memset(&remotebthaddr,0,sizeof(remotebthaddr));
	bthaddr.addressFamily = AF_BTH;
    bthaddr.port = BT_PORT_ANY;
	remotebthaddr.addressFamily = AF_BTH;
    remotebthaddr.serviceClassId = g_guidServiceClass;
    remotebthaddr.port = 0;
	clientsock = INVALID_SOCKET;
	
}


int BTsocket::BTfindbyname(wchar_t* name){

	INT             iResult = 0;
    BOOL            bContinueLookup = FALSE, bRemoteDeviceFound = FALSE;
    ULONG           ulFlags = 0, ulPQSSize = sizeof(WSAQUERYSET);
    HANDLE          hLookup = NULL;
    PWSAQUERYSET    pWSAQuerySet = NULL;

    pWSAQuerySet = (PWSAQUERYSET) HeapAlloc(GetProcessHeap(),
                                            HEAP_ZERO_MEMORY,
                                            ulPQSSize);
    if ( NULL == pWSAQuerySet ) {
        iResult = STATUS_NO_MEMORY;
        wprintf(L"!ERROR! | Unable to allocate memory for WSAQUERYSET\n");
    }

    //
    // Search for the device with the correct name
    //
    if ( 0 == iResult) {

        for ( INT iRetryCount = 0;
            !bRemoteDeviceFound && (iRetryCount < 3);
            iRetryCount++ ) {
            //
            // WSALookupService is used for both service search and device inquiry
            // LUP_CONTAINERS is the flag which signals that we're doing a device inquiry.
            //
            ulFlags = LUP_CONTAINERS;

            //
            // Friendly device name (if available) will be returned in lpszServiceInstanceName
            //
            ulFlags |= LUP_RETURN_NAME;

            //
            // BTH_ADDR will be returned in lpcsaBuffer member of WSAQUERYSET
            //
            ulFlags |= LUP_RETURN_ADDR;

            if ( 0 == iRetryCount ) {
                wprintf(L"*INFO* | Inquiring device from cache...\n");
            } else {
                //
                // Flush the device cache for all inquiries, except for the first inquiry
                //
                // By setting LUP_FLUSHCACHE flag, we're asking the lookup service to do
                // a fresh lookup instead of pulling the information from device cache.
                //
                ulFlags |= LUP_FLUSHCACHE;

                //
                // Pause for some time before all the inquiries after the first inquiry
                //
                // Remote Name requests will arrive after device inquiry has
                // completed.  Without a window to receive IN_RANGE notifications,
                // we don't have a direct mechanism to determine when remote
                // name requests have completed.
                //
                wprintf(L"*INFO* | Unable to find device.  Waiting for %d seconds before re-inquiry...\n", 15);
                Sleep(15 * 1000);

                wprintf(L"*INFO* | Inquiring device ...\n");
            }

            //
            // Start the lookup service
            //
            iResult = 0;
            hLookup = 0;
            bContinueLookup = FALSE;
            ZeroMemory(pWSAQuerySet, ulPQSSize);
            pWSAQuerySet->dwNameSpace = NS_BTH;
            pWSAQuerySet->dwSize = sizeof(WSAQUERYSET);
            iResult = WSALookupServiceBegin(pWSAQuerySet, ulFlags, &hLookup);

            //
            // Even if we have an error, we want to continue until we
            // reach the CXN_MAX_INQUIRY_RETRY
            //
            if ( (NO_ERROR == iResult) && (NULL != hLookup) ) {
                bContinueLookup = TRUE;
            } else if ( 0 < iRetryCount ) {
                wprintf(L"=CRITICAL= | WSALookupServiceBegin() failed with error code %d, WSAGetLastError = %d\n", iResult, WSAGetLastError());
                break;
            }

            while ( bContinueLookup ) {
                //
                // Get information about next bluetooth device
                //
                // Note you may pass the same WSAQUERYSET from LookupBegin
                // as long as you don't need to modify any of the pointer
                // members of the structure, etc.
                //
                // ZeroMemory(pWSAQuerySet, ulPQSSize);
                // pWSAQuerySet->dwNameSpace = NS_BTH;
                // pWSAQuerySet->dwSize = sizeof(WSAQUERYSET);
                if ( NO_ERROR == WSALookupServiceNext(hLookup,
                                                      ulFlags,
                                                      &ulPQSSize,
                                                      pWSAQuerySet) ) {
                    
                    //
                    // Compare the name to see if this is the device we are looking for.
                    //
                    if ( ( pWSAQuerySet->lpszServiceInstanceName != NULL ) &&
                         ( 0 == _wcsicmp(pWSAQuerySet->lpszServiceInstanceName, name) ) ) {
                        //
                        // Found a remote bluetooth device with matching name.
                        // Get the address of the device and exit the lookup.
                        //
                        CopyMemory(&remotebthaddr,
                                   (PSOCKADDR_BTH) pWSAQuerySet->lpcsaBuffer->RemoteAddr.lpSockaddr,
                                   sizeof(remotebthaddr));
                        bRemoteDeviceFound = TRUE;
                        bContinueLookup = FALSE;
                    }
                } else {
                    iResult = WSAGetLastError();
                    if ( WSA_E_NO_MORE == iResult ) { //No more data
                        //
                        // No more devices found.  Exit the lookup.
                        //
                        bContinueLookup = FALSE;
                    } else if ( WSAEFAULT == iResult ) {
                        //
                        // The buffer for QUERYSET was insufficient.
                        // In such case 3rd parameter "ulPQSSize" of function "WSALookupServiceNext()" receives
                        // the required size.  So we can use this parameter to reallocate memory for QUERYSET.
                        //
                        HeapFree(GetProcessHeap(), 0, pWSAQuerySet);
                        pWSAQuerySet = (PWSAQUERYSET) HeapAlloc(GetProcessHeap(),
                                                                HEAP_ZERO_MEMORY,
                                                                ulPQSSize);
                        if ( NULL == pWSAQuerySet ) {
                            wprintf(L"!ERROR! | Unable to allocate memory for WSAQERYSET\n");
                            iResult = STATUS_NO_MEMORY;
                            bContinueLookup = FALSE;
                        }
                    } else {
                        wprintf(L"=CRITICAL= | WSALookupServiceNext() failed with error code %d\n", iResult);
                        bContinueLookup = FALSE;
                    }
                }
            }

            //
            // End the lookup service
            //
            WSALookupServiceEnd(hLookup);

            if ( STATUS_NO_MEMORY == iResult ) {
                break;
            }
        }
    }

    if ( NULL != pWSAQuerySet ) {
        HeapFree(GetProcessHeap(), 0, pWSAQuerySet);
        pWSAQuerySet = NULL;
    }

    if ( bRemoteDeviceFound ) {
        iResult = 0;
    } else {
        iResult = 1;
    }

    return iResult;

}

int BTsocket::BTconnect(void){
	//
    // Connect the socket (pSocket) to a given remote socket represented by address (pServerAddr)
    //
	clientsock = sock;  //VERY IMPORTANT!
	printf("connect\n");
    if ( SOCKET_ERROR == connect(sock, (struct sockaddr *) &remotebthaddr, sizeof(SOCKADDR_BTH)) ) {
        wprintf(L"=CRITICAL= | connect() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
        return -1;
    } else return 0;
}

int BTsocket::BTsend(char* buf, int len){
	//
    // send() call indicates winsock2 to send the given data
    // of a specified length over a given connection.
    //
	int tmp1 = 0, tmp2 = 0;
    wprintf(L"*INFO* | Sending following data string:\n%s\n", buf);
	while (tmp1 < len) {
		switch (tmp2 = send(clientsock, buf+tmp1, len-tmp1, 0)) {
		case 0: // socket connection has been closed gracefully
			wprintf(L"send returns 0.\n");
		case SOCKET_ERROR: //or error
			wprintf(L"=CRITICAL= | send() call failed w/socket = [0x%I64X], szData = [%p], dataLen = [%I64u]. WSAGetLastError=[%d]\n", (ULONG64)sock, buf, (ULONG64) len, WSAGetLastError());
			return -1;
		default:
			tmp1 += tmp2;
			break;
		}
	}
	return 0;
}

//#fixme
int BTsocket::BTsend(pacco* pkt){
	int tmp1 = 0, tmp2 = 0;
	header_t* hdr = pkt->getSerializedHeader();
	while (tmp1 < sizeof(header_t)) {
		switch (tmp2 = send(clientsock, (char*)hdr, sizeof(header_t)-tmp1, 0)) {
		case 0: // socket connection has been closed gracefully
			wprintf(L"send returns 0.\n");
		case SOCKET_ERROR: //or error
			wprintf(L"=CRITICAL= | send() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
			return -1;
		default:
			tmp1 += tmp2;
			break;
		}
	}
	
	tmp1 = tmp2 = 0;
	//send the rest.
	while (tmp1 < hdr->size) {
		switch (tmp2 = send(clientsock, pkt->getData()+tmp1, hdr->size-tmp1, 0)) {
		case 0:
		case SOCKET_ERROR:
			wprintf(L"=CRITICAL= | send() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
			HeapFree(GetProcessHeap(), 0, hdr);
			return -1;
		default:
			tmp1 += tmp2;
			break;
		}
	}
	return 0;
}


int BTsocket::BTinsertMACtarget(wchar_t* name){
	int len = sizeof(remotebthaddr);
	if (WSAStringToAddressW(name, AF_BTH, NULL, (LPSOCKADDR)&remotebthaddr,&len)) 
		return -1;
	else return 0;
}

BTsocket::~BTsocket(void)
{
	if (sock != INVALID_SOCKET) closesocket(sock);
	if (clientsock != INVALID_SOCKET) closesocket(clientsock);
}

int BTsocket::BTbind(void){
	if ( SOCKET_ERROR == bind(sock, (struct sockaddr *) &bthaddr,
                                  sizeof(SOCKADDR_BTH) ) ) {
            wprintf(L"=CRITICAL= | bind() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)sock, WSAGetLastError());
            return SOCKET_ERROR;
        }
	return 0;
}



int BTsocket::BTregister(void){
	int iAddrLen = sizeof(SOCKADDR_BTH);
	LPCSADDR_INFO lpCSAddrInfo = {0};
	WSAQUERYSET wsaQuerySet;
	wchar_t szThisComputerName[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD dwLenComputerName = MAX_COMPUTERNAME_LENGTH + 1;
	size_t cbInstanceNameSize = 0;
	wchar_t* pszInstanceName = NULL;

	if ( !GetComputerName(szThisComputerName, &dwLenComputerName) ) {
            wprintf(L"=CRITICAL= | GetComputerName() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
            return SOCKET_ERROR;
    }

    if (SOCKET_ERROR == getsockname(sock, (struct sockaddr *)&bthaddr, &iAddrLen)){
        wprintf(L"=CRITICAL= | getsockname() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)sock, WSAGetLastError());
        return SOCKET_ERROR;
    }
	lpCSAddrInfo = (LPCSADDR_INFO) HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CSADDR_INFO));
    lpCSAddrInfo->LocalAddr.iSockaddrLength = sizeof( SOCKADDR_BTH );
    lpCSAddrInfo->LocalAddr.lpSockaddr = (LPSOCKADDR)&bthaddr;
    lpCSAddrInfo->RemoteAddr.iSockaddrLength = sizeof( SOCKADDR_BTH );
    lpCSAddrInfo->RemoteAddr.lpSockaddr = (LPSOCKADDR)&bthaddr;
    lpCSAddrInfo->iSocketType = SOCK_STREAM;
    lpCSAddrInfo->iProtocol = BTHPROTO_RFCOMM;

    //
    // If we got an address, go ahead and advertise it.
    //
    memset(&wsaQuerySet, 0, sizeof(WSAQUERYSET));
    wsaQuerySet.dwSize = sizeof(WSAQUERYSET);
    wsaQuerySet.lpServiceClassId = (LPGUID) &g_guidServiceClass;

    //
    // Adding a byte to the size to account for the space in the
    // format string in the swprintf call. This will have to change if converted
    // to UNICODE
    //
    if( FAILED(StringCchLength(szThisComputerName, sizeof(szThisComputerName), &cbInstanceNameSize)) ) {
        wprintf(L"-FATAL- | ComputerName specified is too large\n");
        return SOCKET_ERROR;
    }

    cbInstanceNameSize += sizeof(INSTANCE_STRING) + 1;
    pszInstanceName = (LPWSTR)HeapAlloc(GetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        cbInstanceNameSize);
    if ( NULL == pszInstanceName ) {
        wprintf(L"-FATAL- | HeapAlloc failed | out of memory | gle = [%d] \n", GetLastError());
        return SOCKET_ERROR;
    }

    StringCbPrintf(pszInstanceName, cbInstanceNameSize, L"%s %s", szThisComputerName, INSTANCE_STRING);
    wsaQuerySet.lpszServiceInstanceName = pszInstanceName;
    wsaQuerySet.lpszComment = L"Example Service instance registered in the directory service through RnR";
    wsaQuerySet.dwNameSpace = NS_BTH;
    wsaQuerySet.dwNumberOfCsAddrs = 1;      // Must be 1.
    wsaQuerySet.lpcsaBuffer = lpCSAddrInfo; // Req'd.

    //
    // As long as we use a blocking accept(), we will have a race
    // between advertising the service and actually being ready to
    // accept connections.  If we use non-blocking accept, advertise
    // the service after accept has been called.
    //
    if ( SOCKET_ERROR == WSASetService(&wsaQuerySet, RNRSERVICE_REGISTER, 0) ) {
        wprintf(L"=CRITICAL= | WSASetService() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
        return SOCKET_ERROR;
    }
	return 0;
}


int BTsocket::BTlisten(void){
    //
    // listen() call indicates winsock2 to listen on a given socket for any incoming connection.
    //
    if ( SOCKET_ERROR == listen(sock, 4) ) {
        wprintf(L"=CRITICAL= | listen() call failed w/socket = [0x%I64X]. WSAGetLastError=[%d]\n", (ULONG64)sock, WSAGetLastError());
        return SOCKET_ERROR;
    }
	return 0;
}

int BTsocket::BTaccept(void){
	//
    // accept() call indicates winsock2 to wait for any
    // incoming connection request from a remote socket.
    // If there are already some connection requests on the queue,
    // then accept() extracts the first request and creates a new socket and
    // returns the handle to this newly created socket. This newly created
    // socket represents the actual connection that connects the two sockets.
    //
    clientsock = accept(sock, NULL, NULL);
    if ( INVALID_SOCKET == clientsock ) {
        wprintf(L"=CRITICAL= | accept() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
        return SOCKET_ERROR;
    }
	return 0;
}

//remember to call free()
char* BTsocket::BTrecv(int size){
	int tmp1 = 0, tmp2 = 0;
	char* buf = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
	assert(buf);
	while (tmp1 < size) {
		switch (tmp2 = recv(clientsock, buf+tmp1, size-tmp1, 0)) {
		case 0: // socket connection has been closed gracefully
			wprintf(L"recv returns 0.\n");
		case SOCKET_ERROR: //or error
			wprintf(L"=CRITICAL= | recv() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
			return NULL;
		default:
			tmp1 += tmp2;
			break;
		}
	}
	return buf;
}


//remember to call free()
pacco* BTsocket::BTrecv(void){
	char size2read[sizeof(header_t)];
	int received = 0;
	int tmp1 = 0, tmp2 = 0, tmp3 = 0;
	char* buf;
	header_t* hdr;
	//read the header first.

	printf("sizeof(header_t) = %d.\n",sizeof(header_t));

	while (tmp1 < sizeof(header_t)) {
		switch (tmp2 = recv(clientsock, size2read+tmp1, sizeof(header_t)-tmp1, 0)) {
		case 0: // socket connection has been closed gracefully
			wprintf(L"recv returns 0.\n");
		case SOCKET_ERROR: //or error
			wprintf(L"=CRITICAL= | recv() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
			return NULL;
		default:
			tmp1 += tmp2;
			break;
		}
	}
	hdr = (header_t*) size2read;
	hdr->type = ntohl(hdr->type);
	hdr->size = ntohl(hdr->size);
	printf("hdr->size = %d\n",hdr->size);
	buf = (char *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, hdr->size);
	assert(buf);
	tmp1 = tmp2 = 0;
	//read the rest.
	while (tmp1 < hdr->size) {
		switch (tmp2 = recv(clientsock, buf+tmp1, hdr->size-tmp1, 0)) {
		case 0:
		case SOCKET_ERROR:
			wprintf(L"=CRITICAL= | recv() call failed. WSAGetLastError=[%d]\n", WSAGetLastError());
			HeapFree(GetProcessHeap(), 0, buf);
			return NULL;
		default:
			tmp1 += tmp2;
			break;
		}
	}
	return new pacco(hdr->type,buf,hdr->size);
}

void BTsocket::BTclose(void){
	if (clientsock == INVALID_SOCKET) closesocket(clientsock);
	clientsock = INVALID_SOCKET;
}