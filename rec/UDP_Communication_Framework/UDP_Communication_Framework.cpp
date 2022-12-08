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
	char *f_data = NULL;
	long current_index = 0;
	FILE* fptr;
	int index = 0;
	long data_size = 0;
	memset(fname, 0x00, sizeof fname);
	while (1) {
		if (recvfrom(socketS, buffer_rx, sizeof(buffer_rx), 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR) {
			printf("Socket error!\n");
			getchar();
			return 1;
		}
		else {
			//printf("Received: %s\n", buffer_rx);
		}
		char *pointer = NULL;
		//Fetch name of file
		if (strstr(buffer_rx,"ID=1") != NULL) {
			pointer = strstr(buffer_rx, "ID=1");
			pointer = pointer + 5 * sizeof(char);
			strcpy(fname, pointer);
		}
		//Fetch size of file and allocate memory
		else if (strstr(buffer_rx, "ID=2")!=NULL){
			printf("DEBUG:SIZE SAVING\n");
			pointer = strstr(buffer_rx, "ID=2");
			char* ptr;
			char* num_size =(char*) calloc(10, sizeof(char));
			int i = 0;
			pointer += 5 * sizeof(char);
			
			//printf("ALLOCATED MEMORY, POINTER IS %s\n", pointer);		
			
			data_size = strtol(pointer, &ptr, 10);
			f_data = (char*)calloc(data_size, sizeof(char));
			//printf("%d", data_size);
			memset(f_data, 0x00, data_size * sizeof(char));
			if (f_data == NULL) {
				printf("ERROR: cannot allocate memory.\n");
				exit(1);
			}
			free(num_size);
		}
		//Fetch data and save it to file_buffer
		else if (strstr(buffer_rx, "ID=4")) {
			//printf("DEBUG DATA\n");
			pointer = strstr(buffer_rx, "ID=4");
			char* num_bytes = (char*)calloc(10, sizeof(char));
			pointer = pointer + 5 * sizeof(char);
			int i = 0;
			while (pointer[0] != '|') {
				num_bytes[i] = pointer[0];
				i++;
				pointer += sizeof(char);
			}
			
			int read_amount = strtol(num_bytes, NULL, 10);
			char* data_ptr = (char*)calloc(read_amount, sizeof(char));
			//printf("READ AMOUNT OF DATA %d\n",read_amount);
			pointer += sizeof(char);
			//printf("Received %d amount, index at %d\n",read_amount,index);
			for (int i = 0;i < read_amount;i=i+1) {
				data_ptr[i] = pointer[i];
			}
			//printf("Received %s data of size %dB\n", data_ptr,read_amount);
			for (int i = 0;i < read_amount;i+=1) {
				//printf("saving %s to index %d\n", &data_ptr[i], index + i);
				f_data[index+i] = data_ptr[i];
			}
			
			index += 1;
			
			free(num_bytes);
			free(data_ptr);
		}
		if (index * sizeof(char) == data_size && data_size!=0 ) {
			//printf("KONEC, index je %d, velikost souboru je %d\n",index,index*sizeof(char));
			fptr = fopen(fname, "wb+");
			//printf("last char is %s", &f_data[index - 2]);
			int ret = fwrite(f_data, sizeof(char),index*sizeof(char), fptr);
			fclose(fptr);
		}
		
		memset(buffer_rx, 0, sizeof buffer_rx);
	}
	closesocket(socketS);
#endif
	//**********************************************************************

	getchar(); //wait for press Enter
	return 0;
}
