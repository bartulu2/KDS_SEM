// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"
#include <iostream>

#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 1024

#define SENDER

#define TARGET_PORT 5555
#define LOCAL_PORT 8888



void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

bool recieve_ack(SOCKET socket, char *buffer_rx, sockaddr_in dest ) {
	int destlen = sizeof(dest);
	//printf("WAITING FOR ACK\n");
	if (recvfrom(socket, buffer_rx, sizeof(buffer_rx), 0, (sockaddr*)&dest, &destlen) == SOCKET_ERROR) {
		printf("Socket error\n");
		getchar();
		return 1;
	}
	else {
		printf("Received ACK\n");
	}
}


//**********************************************************************
int main()
{
	SOCKET socketS;

	InitWinsock();

	struct sockaddr_in local;
	struct sockaddr_in from;

	
	local.sin_family = AF_INET;
	local.sin_port = htons(LOCAL_PORT);
	local.sin_addr.s_addr = INADDR_ANY;


	socketS = socket(AF_INET, SOCK_DGRAM, 0);
	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0) {
		printf("Binding error!\n");
		getchar(); //wait for press Enter
		return 1;
	}
	//**********************************************************************
	char buffer_rx[BUFFERS_LEN];
	char buffer_tx[BUFFERS_LEN];

	// ID=1 NAME
	// ID=2 SIZE
	// ID=3 START
	// ID=4 DATA
	// ID=5 STOP
	//
	//
	//
	sockaddr_in addrDest;
	FILE* fptr;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);
	char name[] = "..\\jpg.jpg";
	printf("%s\n", name);
	if ((fptr = fopen(name, "rb")) == NULL) {
		printf("Error! opening file");

		// Program exits if the file pointer returns NULL.
		exit(1);
	}
	printf("Opened file succesfully\n");
	//SEND START

	char start[10];
	snprintf(start, 10, "ID=%d", 3);
	strncpy(buffer_tx, start, BUFFERS_LEN);
	//printf("Buffer is %s\n", buffer_tx);
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	printf("Sending START \n");
	memset(buffer_tx, 0, sizeof buffer_tx);

	if (!recieve_ack(socketS, buffer_rx, addrDest)) {
		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Resending START \n");
		memset(buffer_tx, 0, sizeof buffer_tx);
	}

	char src[50];
	char fur[50];
	snprintf(src, 50, "ID=%d|%s", 1, "jpg.jpg");
	//SEND NAME=filename + type
	strncpy(buffer_tx, src, BUFFERS_LEN);
	printf("Sending file name %s\n", src);
	//printf("Buffer is %s\n", buffer_tx);
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	memset(buffer_tx, 0, sizeof buffer_tx);

	if (!recieve_ack(socketS, buffer_rx, addrDest)) {
		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Resending file name %s\n",src);
		memset(buffer_tx, 0, sizeof buffer_tx);
	}

	//SEND SIZE OF FILE

	fseek(fptr, 0L, SEEK_END);
	char buffer[100];
	long int res = ftell(fptr) / sizeof(char);
	fseek(fptr, 0L, SEEK_SET);
	int f_size = snprintf(buffer, 100, "ID=%d|%d", 2, res);

	strncpy(buffer_tx, buffer, BUFFERS_LEN);
	printf("Sending file size %sB\n", buffer);
	//printf("Buffer is %s\n", buffer_tx);
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	memset(buffer_tx, 0, sizeof buffer_tx);

	if (!recieve_ack(socketS, buffer_rx, addrDest)) {
		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Resending file size %sB\n", buffer);
		memset(buffer_tx, 0, sizeof buffer_tx);
	}


	//START SENDING FILE DATA

	char data_buffer[1000 * sizeof(char)];
	char packet_buffer[1024];
	char numbits[25];
	char header[25];
	long data_pointer = 0;
	long bytes_sent = 0;
	bool cont = true;
	int epoch = 5;
	memset(packet_buffer, 0x00, sizeof packet_buffer);
	memset(data_buffer, 0x00, sizeof data_buffer);
	while (cont) {
		int bytes_read = fread(data_buffer, sizeof(char), 900 * sizeof(char), fptr);
		snprintf(header, 23, "ID=%d|", 4);

		snprintf(numbits, 20, "%d", bytes_read);

		strcat(header, numbits);
		strcat(header, "|");
		strcat(packet_buffer, header);
		strcat(packet_buffer, data_buffer);


		sendto(socketS, packet_buffer, strlen(packet_buffer), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Sending DATA, SIZE: %d bytes\n", bytes_read);

		if (!recieve_ack(socketS, buffer_rx, addrDest)) {
			sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
			printf("Resending DATA, SIZE %d bytes\n", bytes_read);
			memset(buffer_tx, 0, sizeof buffer_tx);
		}

		bytes_sent += bytes_read;
		memset(data_buffer, 0x00, sizeof data_buffer);
		memset(packet_buffer, 0x00, sizeof packet_buffer);
		memset(header, 0x00, sizeof header);
		memset(numbits, 0x00, sizeof numbits);
		if (bytes_read == 0) {
			cont = false;
		}
		epoch -= 1;
	}
	//SEND STOP TO FINISH
	printf("Sent total of %ldB", bytes_sent);


	closesocket(socketS);
}

