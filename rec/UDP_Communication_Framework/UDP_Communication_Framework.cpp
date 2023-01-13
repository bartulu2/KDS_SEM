// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//

#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "md5.h"
#include "ws2tcpip.h"
#include <string.h>
#include "..//..//libcrc-master/src/crc16.c"

#define RECEIVER

#define TARGET_IP	"127.0.0.1"

#define TARGET_PORT 8888
#define LOCAL_PORT 5555

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
void fillMsg(message* msg, int id, int data_num, int optional_int, char* opt_val, char* data) {
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
	if (data != NULL) {
		memcpy(pointer, data, optional_int);
	}
	msg->crc = crc_16((unsigned char*)msg, sizeof(message) - sizeof(int));

}
void send_ack(SOCKET socket, char* buffer_tx, sockaddr_in addrDest, message* msg) {
	char* pointer = (char*)msg;
	memset(buffer_tx, 0x00, sizeof(message));
	memcpy(buffer_tx, pointer, sizeof(message));
	
	if (FAULTY_COM) {
		int fault_error = rand() % 10;
		int fault_lost = rand() % 15;
		if (!fault_error) {
			int faulty_bit_1 = rand() % 3;
			int faulty_bit_2 = rand() % 3;
			buffer_tx[faulty_bit_1] = buffer_tx[faulty_bit_2];
			//printf("Faulty ACK send ----- WARNING\n", faulty_bit_1);
			//printf("ACK LOOKS LIKE THIS>>> %s\n", buffer_tx);
			sendto(socket, buffer_tx, sizeof(message), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		}
		else if (!fault_lost) {
			//printf("ACK lost ----- WARNING\n");
		}
		else {
			sendto(socket, buffer_tx, sizeof(message), 0, (sockaddr*)&addrDest, sizeof(addrDest));
			//printf("ACK SEND\n");
		}
	}else{
		int num_sent = sendto(socket, buffer_tx, sizeof(message), 0, (sockaddr*)&addrDest, sizeof(addrDest));
		std::cout << "DEBUG: SENDING ACK" << " ID: " << msg->id << " DATA_NUM: " << msg->data_num <<  std::endl;
	}
	memset(buffer_tx, 0x00, sizeof buffer_tx);
}


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
	

	//ADD OPTIONAL_INT
	memcpy(msg_pointer, buf_pointer, sizeof(int));
	msg_pointer += sizeof(int);
	buf_pointer += sizeof(int);
	//ADD OPTIONAL_VAL
	memcpy(msg_pointer, buf_pointer, sizeof(char)*OPT_VAL_SIZE);
	msg_pointer += sizeof(char) * OPT_VAL_SIZE;
	buf_pointer += sizeof(char) * OPT_VAL_SIZE;

	// ADD DATA
	memcpy(msg_pointer, buf_pointer, sizeof(char) * DATA_SIZE);
	msg_pointer += sizeof(char) * DATA_SIZE;
	buf_pointer += sizeof(char) * DATA_SIZE;

	// ADD CRC
	memcpy(msg_pointer, buf_pointer, sizeof(int));

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

	socketS = socket(AF_INET, SOCK_DGRAM,0);
	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0){
		printf("Binding error!\n");
	    getchar(); //wait for press Enter
		return 1;
	}
	//**********************************************************************
	char buffer_rx[BUFFERS_LEN *2];
	char buffer_tx[BUFFERS_LEN *2];

	sockaddr_in addrDest;
	addrDest.sin_family = AF_INET;
	addrDest.sin_port = htons(TARGET_PORT);
	InetPton(AF_INET, _T(TARGET_IP), &addrDest.sin_addr.s_addr);
	memset(buffer_tx, 0x00, sizeof buffer_tx);
	memset(buffer_rx, 0x00, sizeof buffer_rx);

	std::cout << "DEBUG: WAITING FOR DATA" << std::endl;
	char fname[100];
	FILE* fptr;
	int read_amount=0, data_size=0, index=0, packet_num=0, total_read = 0;
	char *f_data = NULL;
	
	bool com_started = false;
	bool name_sent = false;
	bool datasize_sent = false;
	memset(fname, 0x00, sizeof fname);
	message* send = (message*)malloc(sizeof(message));
	while (1) {
		if (recvfrom(socketS, buffer_rx, 1024, 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR) {
			printf("Socket error! %d\n", WSAGetLastError());
			getchar();
		}
		
		char* pointer = NULL;
		//printf("MSG type %c\n", msg_type);

		message* msg = (message*)malloc(sizeof(message));
		// TODO: Fill msg
		recMsg(buffer_rx, msg);
		//std::cout << "DEBUG: RECEIVED MESSAGE, ID: " << msg->id << " DATA_NUM: " << msg->data_num << " OPT_VAL: " << msg->optional_val << " CRC: " << msg->crc << std::endl;
		int control_crc = crc_16((unsigned char*)msg, sizeof(message) - sizeof(int));
		if (!(control_crc == msg->crc)) {
			std::cout << "DEBUG: CRC ARE NOT IDENTICAL, SKIP" << std::endl;
			continue;
		}


		//CRC IS CORRECT, DECOMPOSE MSG

		// Checks for necessary messages
		if (com_started != true && msg->id != 1) {
			std::cout << "DEBUG: MESSAGE RECEIVED, BUT EXPECTING START, SKIP" << std::endl;
			continue;
		}
		else if (com_started == true && msg->id == 1){
			fillMsg(send, 1, packet_num, 0, NULL, NULL);
			send_ack(socketS, buffer_tx, addrDest,send);
			continue;
		}
		else if (!name_sent && msg->id != 2 && com_started) {
			std::cout << "DEBUG: MESSAGE RECEIVED, BUT EXPECTING FILE NAME, SKIP" << std::endl;
			continue;
		}
		else if (com_started == true && msg->id == 2 && name_sent==true) {
			fillMsg(send, 2, packet_num, 0, NULL, NULL);
			send_ack(socketS, buffer_tx, addrDest,send);
			continue;
		}
		else if (!datasize_sent && msg->id != 3 && com_started && name_sent) {
			std::cout << "DEBUG: MESSAGE RECEIVED, BUT EXPECTING FILE SIZE, SKIP" << std::endl;
			continue;
		}
		else if (com_started == true && msg->id == 3 && name_sent == true && datasize_sent==true) {
			fillMsg(send, 3, packet_num, 0, NULL, NULL);
			send_ack(socketS, buffer_tx, addrDest,send);
			continue;
		}

		switch (msg->id) {

		case 1:
			std::cout << "DEBUG: START RECEIVED, BEGIN COMMUNICATION" << std::endl;
			fillMsg(send,1, 0, 0, NULL, NULL);
			send_ack(socketS, buffer_tx, addrDest,send);
			com_started = true;
			break;
		case 2:
			name_sent = true;
			memcpy(fname, msg->optional_val, OPT_VAL_SIZE);
			fillMsg(send, 2, 0, 0, NULL, NULL);
			send_ack(socketS, buffer_tx, addrDest,send);
			std::cout << "DEBUG: RECEIVED FILE NAME: " << msg->optional_val << std::endl;
			break;
		case 3:
			datasize_sent = true;
			fillMsg(send, 3, 0, 0, NULL, NULL);
			send_ack(socketS, buffer_tx, addrDest,send);
			std::cout << "DEBUG: RECEIVED FILE SIZE: " << msg->data_num << std::endl;
			data_size = msg->data_num;
			f_data = (char*)malloc(data_size * sizeof(char));
			break;
		case 4:
			pointer = f_data;
			pointer += total_read * sizeof(char);
			
			//std::cout<< " MY PACKET_NUM: " << packet_num <<" REC: " << msg->data_num << std::endl;
			if (packet_num == msg->data_num) {
				std::cout << "DEBUG: RECEIVED DATA OF NUMBER: " << msg->data_num << " OF SIZE : " << msg->optional_int << std::endl;
				fillMsg(send, 4, packet_num, 0, NULL, NULL);
				send_ack(socketS, buffer_tx, addrDest,send);
				memcpy(pointer, msg->data, sizeof(char) * msg->optional_int);
				total_read += msg->optional_int;
				packet_num++;
			}
			
			else {
				std::cout << "DEBUG: ALREADY RECEIVED THIS DATA PACKET, SKIP. PACKET_NUM: "<<packet_num << " MSG NUM: " << msg->data_num << std::endl;
				if (msg->data_num <= packet_num){
					send->data_num = msg->data_num;
					send_ack(socketS, buffer_tx, addrDest,send);
				}
			}

			break;
		case 5:
			std::cout << "TOTAL READ: " << total_read << " DATA_SIZE: " << data_size << std::endl;
			if (total_read < data_size) {
				std::cout << "DEBUG: RECEIVED HASH, BUT DID NOT RECEIVE ALL FILE DATA" << std::endl;
				

			}
			else {

				fptr = fopen(fname, "wb");
				std::string hash = md5(std::string(f_data, total_read));
				std::cout << "HASH IS " << hash << std::endl;
				if (std::strcmp(hash.c_str(), msg->optional_val) == 0) {
					fillMsg(send, 5, 0, 0, NULL, NULL);
					send_ack(socketS, buffer_tx, addrDest,send);
					std::cout << "DEBUG: HASHES ARE SAME, WRITING TO FILE" << std::endl;
					if (fptr != NULL) {
						fwrite(f_data, sizeof(char), data_size, fptr);

					}
					else {
						std::cout << "POINTER IS NULL" << std::endl;
					}
					
				}
				else {
					std::cout << "DEBUGL: HASHES ARE NOT SAME, SKIP" << std::endl;
				}
			}
			break;
		default:
			std::cout << "DEBUG: RECEIVED UNDEFINED MESSAGE TYPE, SKIP" << std::endl;
			break;


		}
	}
	
	closesocket(socketS);
	//**********************************************************************

	getchar(); //wait for press Enter
	return 0;
}
