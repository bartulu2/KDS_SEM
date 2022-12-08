// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"

#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 1024

#define SENDER
//#define RECEIVER

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
	FILE* fptr;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);
	char name[] = "..\\headlol.png";
	printf("%s\n", name);
	if ((fptr = fopen(name, "rb")) == NULL) {
		printf("Error! opening file");

		// Program exits if the file pointer returns NULL.
		exit(1);
	}
	printf("Opened file succesfully\n");
	char src[50];
	char fur[50];
	strcpy(src, "NAME=");
	strcpy(fur, "head.png");
	//SEND NAME=filename + type
	strncpy(buffer_tx, strcat(src,fur), BUFFERS_LEN);
	printf("Sending file name %s\n",src);
	//printf("Buffer is %s\n", buffer_tx);
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	memset(buffer_tx, 0, sizeof buffer_tx);
	
	//SEND SIZE OF FILE
	
	fseek(fptr, 0L, SEEK_END);
	char buffer[100];
	long int res = ftell(fptr);
	fseek(fptr, 0L, SEEK_SET);
	int f_size = snprintf(buffer, 100, "SIZE=%d", res);

	strncpy(buffer_tx, buffer , BUFFERS_LEN);
	printf("Sending file size %s\n",buffer);
	//printf("Buffer is %s\n", buffer_tx);
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	memset(buffer_tx, 0, sizeof buffer_tx);
	
	//SEND START
	
	char start[10];
	strcpy(start, "START");
	strncpy(buffer_tx,start, BUFFERS_LEN);
	//printf("Buffer is %s\n", buffer_tx);
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	printf("Sending START \n");
	memset(buffer_tx, 0, sizeof buffer_tx);
	
	//START SENDING FILE DATA

	char data_buffer[30*32];
	char packet_buffer[1024];
	char numbits[15];
	char header[23];
	long data_pointer = 0;
	bool cont = true;
	memset(packet_buffer, 0x00, sizeof packet_buffer);
	memset(data_buffer, 0x00, sizeof data_buffer);
	while (cont) {
		int bytes_read = fread(data_buffer,sizeof(int), 32, fptr);
		strcpy(header, "DATA=");
		snprintf(numbits, 20, "%d", data_pointer);
		
		strcat(header, numbits);
		strcat(header, "|");
		strcat(packet_buffer, header);
		printf("%d, %d\n",strlen(packet_buffer),strlen(data_buffer));
		strcat(packet_buffer, data_buffer);
		
		data_pointer = data_pointer + bytes_read*sizeof(int);
		sendto(socketS, packet_buffer, strlen(packet_buffer), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Sending DATA starting at %d, of length %d, with data %s\n",data_pointer,bytes_read);
		memset(data_buffer, 0x00, sizeof data_buffer);
		memset(packet_buffer, 0x00, sizeof packet_buffer);
		memset(header, 0x00, sizeof header);
		memset(numbits, 0x00, sizeof numbits);
		printf("%d, %d\n", strlen(packet_buffer), strlen(data_buffer));
		if (bytes_read == 0){
			cont = false;
		}
	}
	//SEND STOP TO FINISH
	
	strncpy(buffer_tx, "Hello world payload!\n", BUFFERS_LEN); //put some data to buffer
	
	printf("Sending packet.\n");
		

	closesocket(socketS);

#endif // SENDER

#ifdef RECEIVER

	strncpy(buffer_rx, "No data received.\n", BUFFERS_LEN);
	printf("Waiting for datagram ...\n");
	if(recvfrom(socketS, buffer_rx, sizeof(buffer_rx), 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR){
		printf("Socket error!\n");
		getchar();
		return 1;
	}
	else
		printf("Datagram: %s", buffer_rx);

	closesocket(socketS);
#endif
	//**********************************************************************

	getchar(); //wait for press Enter
	return 0;
}
