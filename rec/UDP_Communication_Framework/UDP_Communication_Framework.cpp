// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"
#include <string.h>
#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 1024

//#define SENDER
#define RECEIVER

#ifdef SENDER
#define TARGET_PORT 5555
#define LOCAL_PORT 8888
#endif // SENDER

#ifdef RECEIVER
#define TARGET_PORT 8888
#define LOCAL_PORT 5555
#endif // RECEIVER


void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

//**********************************************************************
int main()
{
	SOCKET socketS;
	
	InitWinsock();

	struct sockaddr_in local;
	struct sockaddr_in from;

	int fromlen = sizeof(from);
	local.sin_family = AF_INET;
	local.sin_port = htons(LOCAL_PORT);
	local.sin_addr.s_addr = INADDR_ANY;

	socketS = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0){
		printf("Binding error!\n");
	    getchar(); //wait for press Enter
		return 1;
	}
	//**********************************************************************
	char buffer_rx[BUFFERS_LEN];
	char buffer_tx[BUFFERS_LEN];

#ifdef SENDER
	
	sockaddr_in addrDest;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);

	
	strncpy(buffer_tx, "Hello world payload!\n", BUFFERS_LEN); //put some data to buffer
	printf("Sending packet.\n");
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));	

	closesocket(socketS);

#endif // SENDER

#ifdef RECEIVER

	//strncpy(buffer_rx, "No data received.\n", BUFFERS_LEN);
	memset(buffer_rx, 0x00, sizeof buffer_rx);
	printf("Waiting for datagram ...\n");
	char fname[100];
	char f_data;
	FILE* fptr;
	memset(fname, 0x00, sizeof fname);
	while (1) {
		if (recvfrom(socketS, buffer_rx, sizeof(buffer_rx), 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR) {
			printf("Socket error!\n");
			getchar();
			return 1;
		}
		else {
		
		}

			printf("Received: %s\n", buffer_rx);

		
		char *pointer;
		//Fetch name of file
		if (strstr(buffer_rx,"NAME=") != NULL) {
			pointer = strstr(buffer_rx, "NAME=");
			pointer = pointer + 5 * sizeof(char);
			strcpy(fname, pointer);
			printf(pointer);
			
		}
		//Fetch size of file
		else if (strstr(buffer_rx, "SIZE")!=NULL){
			pointer = strstr(buffer_rx, "SIZE=");
			char* ptr;
			pointer = pointer + 5 * sizeof(char);
			long data_size = strtol(pointer, &ptr, 10);
			
			printf("%ld", data_size);
		}
		//Fetch data
		else if (strstr(buffer_rx, "DATA=")) {
			pointer = strstr(buffer_rx, "DATA=");
		}	char amount[10];
			pointer = pointer + 5 * sizeof(char);
			fptr = fopen(fname, "wb");
			while (pointer[0]!='|') {
				strcat(amount, &pointer[0]);
			}
			char* data_ptr;
			int amount_of_data = strtol(amount, NULL, 10);
			int ret = fwrite(data_ptr, sizeof(int), amount_of_data * sizeof(int), fptr);
			memset(buffer_rx, 0, sizeof buffer_rx);
	}
	closesocket(socketS);
#endif
	//**********************************************************************

	getchar(); //wait for press Enter
	return 0;
}
