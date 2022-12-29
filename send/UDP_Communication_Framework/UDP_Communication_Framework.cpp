// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"
#include <iostream>
#include "src/crc16.c"

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
	memset(buffer_rx, 0, sizeof buffer_rx);
	printf("WAITING FOR ACK\n");
	if (recvfrom(socket, buffer_rx, sizeof(buffer_rx), 0, (sockaddr*)&dest, &destlen) == SOCKET_ERROR) {
		printf("Socket error\n");
		getchar();
		exit(1);
	}
	else {
		printf("Received ACK\n");
	}
}

void add_crc(char* buffer) {
	int temp = crc_16((unsigned char*)(buffer), strlen(buffer));
	sprintf(buffer, "%s|%04d", buffer,temp);
}


//**********************************************************************
int main()
{
	char *fname = "text.txt";

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
	char buffer_rx[BUFFERS_LEN*2];
	char buffer_tx[BUFFERS_LEN*2];
	char buffer[BUFFERS_LEN*2];

	// ID=1 START
	// ID=2 NAME
	// ID=3 SIZE
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
	char name[100] = "..\\";
	strcat(name, fname);
	printf("%s\n", name);
	if ((fptr = fopen(name, "rb")) == NULL) {
		printf("Error! opening file");

		// Program exits if the file pointer returns NULL.
		exit(1);
	}
	printf("Opened file succesfully\n");

	//SEND START
	snprintf(buffer, 10, "ID=%d", 1);
	add_crc(buffer);

	strncpy(buffer_tx, buffer, BUFFERS_LEN);
	printf("Sending START \n");
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	memset(buffer_tx, 0, sizeof buffer_tx);
	memset(buffer, 0x00, sizeof buffer);

	if (!recieve_ack(socketS, buffer_rx, addrDest)) {
		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Resending START \n");
		memset(buffer_tx, 0, sizeof buffer_tx);
		memset(buffer, 0x00, sizeof buffer);
	}

	snprintf(buffer, BUFFERS_LEN, "ID=%d|%s", 2, fname);
	printf("Sending file name %s\n", fname);
	add_crc(buffer);
	strncpy(buffer_tx, buffer, BUFFERS_LEN);
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	memset(buffer_tx, 0, sizeof buffer_tx);
	memset(buffer, 0x00, sizeof buffer);

	if (!recieve_ack(socketS, buffer_rx, addrDest)) {
		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Resending file name\n");
		memset(buffer_tx, 0, sizeof buffer_tx);
		memset(buffer, 0x00, sizeof buffer);
	}

	//SEND SIZE OF FILE

	fseek(fptr, 0L, SEEK_END);
	long int res = ftell(fptr) / sizeof(char);
	fseek(fptr, 0L, SEEK_SET);
	int f_size = snprintf(buffer, BUFFERS_LEN, "ID=%d|%d", 3, res);
	printf("Sending file size %dB\n", res);
	add_crc(buffer);
	strncpy(buffer_tx, buffer, BUFFERS_LEN);
	//printf("Buffer is %s\n", buffer_tx);
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	memset(buffer_tx, 0, sizeof buffer_tx);
	memset(buffer, 0x00, sizeof buffer);

	if (!recieve_ack(socketS, buffer_rx, addrDest)) {
		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Resending file size %dB\n", res);
		memset(buffer_tx, 0, sizeof buffer_tx);
		memset(buffer, 0x00, sizeof buffer);
	}


	//START SENDING FILE DATA

	char data_buffer[BUFFERS_LEN];
	long data_pointer = 0;
	long bytes_sent = 0;
	bool cont = true;
	while (cont) {
		int bytes_read = fread(data_buffer, sizeof(char), BUFFERS_LEN * sizeof(char), fptr);
		snprintf(buffer, BUFFERS_LEN, "ID=%d|%d|%s", 4, bytes_read, data_buffer);
		add_crc(buffer);
		strncpy(buffer_tx, buffer, BUFFERS_LEN+16);

		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Sending DATA, SIZE: %d bytes\n", bytes_read);
		memset(buffer_tx, 0, sizeof buffer_tx);
		memset(buffer, 0x00, sizeof buffer);

		if (!recieve_ack(socketS, buffer_rx, addrDest)) {
			sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
			printf("Resending DATA, SIZE %d bytes\n", bytes_read);
			memset(buffer_tx, 0, sizeof buffer_tx);
			memset(buffer, 0x00, sizeof buffer);
		}

		bytes_sent += bytes_read;
		memset(data_buffer, 0x00, sizeof data_buffer);
		if (bytes_read == 0) {
			cont = false;
		}
	}
	//SEND STOP TO FINISH
	printf("Sent total of %ldB", bytes_sent);
	fclose(fptr);
	closesocket(socketS);
}

