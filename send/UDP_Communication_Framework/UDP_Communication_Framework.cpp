// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"
#include <iostream>
#include "md5.h"
#include <sstream>
#include <iostream>
#include "..//..//libcrc-master/src/crc16.c"


#define SENDER

#define TARGET_IP	"127.0.0.1"

#define TARGET_PORT 5555
#define LOCAL_PORT 8888

#define FAULTY_COM TRUE


#define BUFFERS_LEN 1024
#define DATA_SIZE 950
#define OPT_VAL_SIZE 34
void InitWinsock()
{
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
}
typedef struct {
	int id;
	int data_num;
	int optional_int;
	char optional_val[OPT_VAL_SIZE];
	char data[DATA_SIZE];
	int crc;


}message;
int packet_num = 0;
void recMsg(char buffer[], message* msg) {
	char* msg_pointer = (char*)msg;
	char* buf_pointer = buffer;
	// ADD ID
	memcpy(msg_pointer, buf_pointer, sizeof(int));
	msg_pointer += sizeof(int);
	buf_pointer += sizeof(int);

	// ADD DATA_NUM
	memcpy(msg_pointer, buf_pointer, sizeof(int));
	msg_pointer += sizeof(int);
	buf_pointer += sizeof(int);

	// ADD OPTIONAL_INT
	memcpy(msg_pointer, buf_pointer, sizeof(int));
	msg_pointer += sizeof(int);
	buf_pointer += sizeof(int);

	//ADD OPTIONAL_VAL
	memcpy(msg_pointer, buf_pointer, sizeof(char) * OPT_VAL_SIZE);
	msg_pointer += sizeof(char) * OPT_VAL_SIZE;
	buf_pointer += sizeof(char) * OPT_VAL_SIZE;

	// ADD DATA
	memcpy(msg_pointer, buf_pointer, sizeof(char) * DATA_SIZE);
	msg_pointer += sizeof(char) * DATA_SIZE;
	buf_pointer += sizeof(char) * DATA_SIZE;

	// ADD CRC
	memcpy(msg_pointer, buf_pointer, sizeof(int));

}
void fillMsg(message*msg,int id, int data_num, int optional_int, char* opt_val,char* data) {
	char* pointer = (char*)msg;
	memset(msg, 0x00, sizeof(message));
	msg->id = id;
	msg->data_num = data_num;
	msg->optional_int = optional_int;
	pointer += 3 * sizeof(int);
	if (opt_val != NULL) {
		memcpy(pointer, opt_val, OPT_VAL_SIZE);
	}
	pointer += OPT_VAL_SIZE;
	if (data != NULL){
		memcpy(pointer, data, optional_int);
	}
	msg->crc = crc_16((unsigned char*)msg, sizeof(message) - sizeof(int));

}
void sendMsg(message* msg, char* buffer,SOCKET S, sockaddr_in addrDest) {
	char* pointer = (char*)msg;
	memcpy(buffer, pointer, sizeof(message));
	sendto(S, buffer, sizeof(message), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	memset(buffer, 0x00, sizeof(message));
}
bool recieve_ack(SOCKET socket, char *buffer_rx, sockaddr_in dest,message *msg ) {
	fd_set readfds, masterfds;
	struct timeval timeout;
	timeout.tv_sec =0.1;
	timeout.tv_usec = 0;
	FD_ZERO(&masterfds);
	FD_SET(socket, &masterfds);
	memcpy(&readfds, &masterfds, sizeof(fd_set));

	int destlen = sizeof(dest);
	memset(buffer_rx, 0, sizeof buffer_rx);

	if (select(socket + 1, &readfds, NULL, NULL, &timeout) < 0)
	{
		perror("Error in select");
		exit(1);
	}
	if (FD_ISSET(socket, &readfds))
	{
		if (recvfrom(socket, buffer_rx, sizeof(message), 0, (sockaddr*)&dest, &destlen) == SOCKET_ERROR) {
			printf("Socket error\n");
			getchar();
			exit(1);
		}
		else {
			message* rec = (message*)malloc(sizeof(message));
			recMsg(buffer_rx, rec);
			int crc = crc_16((unsigned char*)rec, sizeof(message) - sizeof(int));
			std::cout << "DEBUG: RECEIVED ACK ID:" << rec->id << " PACKET_ID: " << rec->data_num << std::endl;
			if (rec->crc == crc) {
				switch (rec->id) {

				case 0:
				case 1:
					if (msg->id == rec->id) {
						
						return 0;
					}
					break;
				case 2:
					if (msg->id == rec->id) {
						
						return 0;
					}
					break;
				case 3:
					if (msg->id == rec->id) {
						
						return 0;
					}
					break;
				case 4:
					if ((msg->id == rec->id) && (rec->data_num == msg->data_num)) {
						packet_num++;
						
						return 0;
					}
					break;
				case 5:
					if (msg->id == rec->id) {
						
						return 0;
					}
					break;
				default:
					break;
				}
				
				memset(buffer_rx, 0, sizeof buffer_rx);
			}
			else {
				std::cout<< "DEBUG: WRONG ACK" << std::endl;
				memset(buffer_rx, 0, sizeof buffer_rx);
				return 2;
			}
		}
	}
	else
	{
		printf("Socket timeout\n");
		return 1;
	}
}


//**********************************************************************
int main()
{
	char* fname = "pizafile.jpg";

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
		getchar(); //wait for pfile_sizes Enter
		return 1;
	}
	//**********************************************************************
	char buffer_rx[BUFFERS_LEN * 2];
	char buffer_tx[BUFFERS_LEN * 2];
	char buffer[BUFFERS_LEN * 2];
	char* pointer = NULL;
	// ID=1 START
	// ID=2 NAME
	// ID=3 SIZE
	// ID=4 DATA
	// ID=5 HASH
	// ID=6 STOP

	sockaddr_in addrDest;
	FILE* fptr;
	int sent = 0;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);
	char name[100] = "..\\";
	strcat(name, fname);

	if ((fptr = fopen(name, "rb")) == NULL) {
		printf("Error! opening file");

		// Program exits if the file pointer returns NULL.
		exit(1);
	}
	memset(buffer_tx, 0x00, BUFFERS_LEN*2);

	//SEND START

	message* msg = (message*)malloc(sizeof(message));
	fillMsg(msg, 1, 0, 0, NULL, NULL);
	sendMsg(msg, buffer_tx, socketS, addrDest);
	std::cout << "DEBUG: START MESSAGE PREPARED. ID: " << msg->id << ", CRC: " << msg->crc << std::endl;
	

	while (recieve_ack(socketS, buffer_rx, addrDest,msg)) {
		sendto(socketS, buffer_tx, sizeof(message), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Resending START \n");
	}

	// SEND NAME OF FILE
	

	fillMsg(msg, 2, 0, 0, fname, NULL);
	sendMsg(msg, buffer_tx, socketS, addrDest);
	std::cout << "DEBUG: FILE NAME MESSAGE PREPARED. ID: " << msg->id<< ", NAME: "<< msg->optional_val << ", CRC: " << msg->crc << std::endl;

	while (recieve_ack(socketS, buffer_rx, addrDest,msg)) {
		sendto(socketS, buffer_tx, sizeof(message), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		printf("Resending file name\n");
	}

	//SEND SIZE OF FILE
	
	memset(buffer_tx, 0x00, BUFFERS_LEN *2);
	fseek(fptr, 0L, SEEK_END);
	long int file_size = ftell(fptr) / sizeof(char);
	fseek(fptr, 0L, SEEK_SET);

	fillMsg(msg,3, file_size, 0, NULL, NULL);
	sendMsg(msg, buffer_tx, socketS, addrDest);
	std::cout << "DEBUG: FILE SIZE MESSAGE PREPARED. ID: " << msg->id << ", SIZE: " << msg->data_num << ", CRC: " << msg->crc << std::endl;

	while (recieve_ack(socketS, buffer_rx, addrDest,msg)) {
		sendMsg(msg, buffer_tx, socketS, addrDest);
		printf("Resending file size %dB\n", file_size);
	}

	//START SENDING FILE DATA
	bool cont = true;
	int iter = 0;
	while (cont) {
		iter++;
		char* data_buffer = (char*)calloc(DATA_SIZE, sizeof(char));
		memset(data_buffer, 0x00, DATA_SIZE);
		int bytes_read = fread(data_buffer, sizeof(char), DATA_SIZE, fptr);
		
		
		fillMsg(msg, 4, packet_num, bytes_read, NULL, data_buffer);
		std::cout << "DEBUG: FILE DATA MESSAGE PREPARED. ID: " << msg->id << ", PACKET_NUM: " << msg->data_num << ", SIZE: " << msg->optional_int << ", CRC: " << msg->crc << std::endl;
		/*std::cout << "DATA: " << std::endl;
		pointer = (char*)msg;
		for (int i = 0;i < msg->optional_int;i++) {
			std::cout << data_buffer[i];
		}*/
		if (FAULTY_COM){
			if (packet_num % 7 == 0) {
				int faulty_bit_1 = rand()%bytes_read;
				int faulty_bit_2 = rand() % bytes_read;
				pointer = (char*)msg;
				memset(buffer_tx, 0x00, sizeof(message));
				memcpy(buffer_tx, pointer, sizeof(message));
				char temp = buffer_tx[faulty_bit_1];
				buffer_tx[faulty_bit_1] = buffer_tx[faulty_bit_2];
				std::cout << "DEBUG: FAULTY BIT" << std::endl;
				buffer_tx[faulty_bit_2] = temp;
				sendto(socketS, buffer_tx, sizeof(message), 0, (sockaddr*)&addrDest, sizeof(addrDest));
				
			}
			else if(packet_num % 10 == 0) {
				std::cout << "DEBUG: PACKET LOSS, NUMBER: " << packet_num << std::endl;
			}
			else {
				sendMsg(msg, buffer_tx, socketS, addrDest);
			}
		}
		else {
			sendMsg(msg, buffer_tx, socketS, addrDest);
			
		}



		while (recieve_ack(socketS, buffer_rx, addrDest,msg)!=0) {
			sendMsg(msg, buffer_tx, socketS, addrDest);
			std::cout << "DEBUG: RESENDING MESSAGE" << std::endl;
			//std::cout << "DEBUG: ID: " << msg->id << ", PACKET_NUM: " << msg->data_num << ", SIZE: " << msg->optional_int << ", CRC: " << msg->crc << std::endl;
			
		}

		//bytes_sent += bytes_read;

		if (bytes_read == 0) {
			cont = false;
		}
		if (iter >25) {
			//cont = false;
		}
		free(data_buffer);
	}
	//SEND STOP TO FINISH
	
	std::cout << "DEBUG: FINISHED SENDING FILE" << std::endl;
	fclose(fptr);
	FILE* pepe_file = fopen(name, "rb");
	char* hash_file = (char*)calloc(file_size, sizeof(char));
	// Pole alokovano
	int byte_read = fread(hash_file, sizeof(char), file_size, pepe_file);

	if (byte_read != file_size || hash_file == NULL || pepe_file == NULL) {
		std::cout << byte_read << " " << file_size << std::endl;
		printf("Cannot read file to compute hash, bye.\n");
	}
	else {
		std::string hash = md5(std::string(hash_file, file_size));
		fillMsg(msg, 5, 0, 0, (char*)hash.c_str(), NULL);
		sendMsg(msg, buffer_tx, socketS, addrDest);
		std::cout << "DEBUG: SENDING HASH: " << msg->optional_val << std::endl;

		while (recieve_ack(socketS, buffer_rx, addrDest,msg)) {
			sendto(socketS, buffer_tx, sizeof(message), 0, (sockaddr*)&addrDest, sizeof(addrDest));
			printf("Resending HASH");
		}

	}
	free(hash_file);
	fclose(pepe_file);
	
	closesocket(socketS);
	exit(1);
}

