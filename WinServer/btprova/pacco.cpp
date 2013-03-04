#include "StdAfx.h"
#include "pacco.h"


pacco::pacco(int tipo, char* raw, int len)
{
	//memcpy(data,raw,len);
	data = raw;
	type = tipo;
	datalen = len;
}

pacco::~pacco(void)
{
	HeapFree(GetProcessHeap(), 0, data);
}

char* pacco::getData(void){
	return data;
}

int pacco::getType(void){
	return type;
}

int pacco::getSize(void){
	return datalen;
}
header_t* pacco::getSerializedHeader(void){
	header_t* hdr = (header_t*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(header_t));
	hdr->size = htonl(datalen);
	hdr->type = htonl(type);
	return hdr;
}