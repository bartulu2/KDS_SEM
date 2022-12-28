// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"
#include <string.h>
#define TARGET_IP	"127.0.0.1"

#define BUFFERS_LEN 1024

#define RECEIVER

#ifdef RECEIVER
#define TARGET_PORT 8888
#define LOCAL_PORT 5555
#endif // RECEIVER


void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}

void send_ack(SOCKET socket,char *buffer_tx, sockaddr_in addrDest) {
	char ack[10];
	snprintf(ack, 10, "ACK");
	strncpy(buffer_tx, ack, BUFFERS_LEN);
	sendto(socket, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	printf("ACK SEND\n");
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

	sockaddr_in addrDest;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);


	//strncpy(buffer_rx, "No data received.\n", BUFFERS_LEN);
	memset(buffer_rx, 0x00, sizeof buffer_rx);
	printf("Waiting for datagram ...\n");
	char fname[100];
	char *f_data = NULL;
	FILE* fptr;
	int index = 0;
	long data_size = 0;
	int total_read = 0;
	int read_amount = 0;
	bool com_started = false;
	memset(fname, 0x00, sizeof fname);
	while (1) {
		if (recvfrom(socketS, buffer_rx, sizeof(buffer_rx), 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR) {
			printf("Socket error!\n");
			getchar();
			return 1;
		}
		else {
			send_ack(socketS, buffer_tx, from);
		}
		char *pointer = NULL;
		// Waits for start msg
		if (strstr(buffer_rx, "ID=3") != NULL) {
			com_started = true;
			printf("COM STARTED\n");
		}
		if (com_started != true) {
			printf("Recieved msg without start\n");
			continue;
		}
		//Fetch name of file
		if (strstr(buffer_rx,"ID=1") != NULL) {
			pointer = strstr(buffer_rx, "ID=1");
			pointer = pointer + 5 * sizeof(char);
			strcpy(fname, pointer);
		}
		//Fetch size of file and allocate memory
		else if (strstr(buffer_rx, "ID=2")!=NULL){
			//printf("DEBUG:SIZE SAVING\n");
			pointer = strstr(buffer_rx, "ID=2");
			pointer += 5 * sizeof(char);
			//printf("ALLOCATED MEMORY, POINTER IS %s\n", pointer);		
			
			data_size = strtol(pointer, NULL, 10);
			f_data = (char*)calloc(data_size, sizeof(char));
			printf("DATA_SIZE: %d\n", data_size);
			memset(f_data, 0x00, data_size * sizeof(char));
			if (f_data == NULL) {
				printf("ERROR: cannot allocate memory.\n");
				exit(1);
			}
		}
		//Fetch data and save it to file_buffer
		else if (strstr(buffer_rx, "ID=4")) {
			
			//printf("DEBUG DATA\n");
			pointer = strstr(buffer_rx, "ID=4");
			char* num_bytes = (char*)calloc(10, sizeof(char));
			pointer += 5 * sizeof(char);
			int i = 0;
			while (pointer[0] != '|') {
				num_bytes[i] = pointer[0];
				i++;
				pointer += sizeof(char);
			}
			
			read_amount = strtol(num_bytes, NULL, 10);
			pointer += sizeof(char);
			for (int i = 0;i < read_amount;i++) {
				f_data[i+total_read] = pointer[i];
			}
			total_read += read_amount;
			index++;

			// PRINT KONTROLA
			if (read_amount == 0) {
				com_started = false;
			}
			else {
				printf("READ AMOUNT: %d, INDEX: %d\n", read_amount, index);
				//printf("DATA:\n");
				//for (i = 0; i < read_amount; i++) {
				//	printf("%c", pointer[i]);
				//}
				//printf("\n");
			}
		}
		if (total_read == data_size && data_size!=0 && read_amount ==0) {
			printf("KONEC, index je %d, velikost souboru je %d\n",index,data_size);
			fptr = fopen(fname, "wb");
			int ret = fwrite(f_data, sizeof(char), total_read, fptr);
			if (ret == NULL) {
				printf("You fucked up");
			}
			fclose(fptr);
			printf("Finished writting in file");
		}
		
		memset(buffer_rx, 0, sizeof buffer_rx);
	}
	closesocket(socketS);
	//**********************************************************************

	getchar(); //wait for press Enter
	return 0;
}
