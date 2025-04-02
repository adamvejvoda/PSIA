// UDP_Communication_Framework.cpp : Defines the entry point for the console application.
//
 
#pragma comment(lib, "ws2_32.lib")
#include "stdafx.h"
#include <winsock2.h>
#include "ws2tcpip.h"
#include <stdio.h>
#include <stdint.h>
#include <string>
 
#define TARGET_IP	"147.32.221.16"
 
#define BUFFERS_LEN 1024
 
//#define SENDER
#define RECEIVER
 
#ifdef SENDER
#define TARGET_PORT 5555
#define LOCAL_PORT 8888
#define FILENAME "file.txt"
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
	if (bind(socketS, (sockaddr*)&local, sizeof(local)) != 0) {
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
 
	FILE* file = fopen(FILENAME, "r");
	if (file == NULL) {
		printf("File error!\n");
		getchar();
		return 1;
	}
 
	fseek(file, 0, SEEK_END); // seek to end of file - 2
	int file_size = ftell(file);
	fseek(file, 0, SEEK_SET);
 
 
	// send file name
	strncpy(buffer_tx, "NAME=" FILENAME, BUFFERS_LEN);
	printf("Sending file name.\n");
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
 
	// send file size
	strncpy(buffer_tx, "SIZE=" + file_size, BUFFERS_LEN);
	printf("Sending file size.\n");
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
 
	// send START
	strncpy(buffer_tx, "START", BUFFERS_LEN);
	printf("Sending start.\n");
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
 
	// send file
	while (!feof(file)) {
		int data_len = fread(buffer_tx + 4, 1, BUFFERS_LEN - 4, file);
		if (data_len == 0) {
			break;
		}
 
		uint32_t offset = htonl(ftell(file));
		memcpy(buffer_tx, &offset, 4);
		sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
	}
 
	// send STOP
	strncpy(buffer_tx, "STOP", BUFFERS_LEN);
	printf("Sending stop.\n");
	sendto(socketS, buffer_tx, strlen(buffer_tx), 0, (sockaddr*)&addrDest, sizeof(addrDest));
 
	closesocket(socketS);
	fclose(file);
 
#endif // SENDER
 
#ifdef RECEIVER
 
	std::string filename;
	int file_size = 0;
	int file_offset = 0;
	FILE* file = NULL;
 
	while (true) {
 
		strncmp(buffer_rx, "No data received.\n", BUFFERS_LEN);
		printf("Waiting for data...\n");
		if (recvfrom(socketS, buffer_rx, sizeof(buffer_rx), 0, (sockaddr*)&from, &fromlen) == SOCKET_ERROR) {
			printf("Error receiving data!\n");
			getchar();
			return 1;
		}
		else {
			printf("Data: %s\n", buffer_rx);
		}
 
		if (strncmp(buffer_rx, "NAME=", 5) == 0) {
			filename = buffer_rx + 5;
			file = fopen(filename.c_str(), "w");
			if (file == NULL) {
				printf("File error!\n");
				getchar();
				return 1;
			}
		}
		else if (strncmp(buffer_rx, "SIZE=", 5) == 0) {
			file_size = atoi(buffer_rx + 5);
		}
		else if (strncmp(buffer_rx, "START", 5) == 0) {
			file_offset = 0;
		}
		else if (strncmp(buffer_rx, "STOP", 4) == 0) {
			fclose(file);
			break;
		}
		else {
			uint32_t offset;
			memcpy(&offset, buffer_rx, 4);
			offset = ntohl(offset);
			if (offset == file_offset) {
				fwrite(buffer_rx + 4, 1, strlen(buffer_rx) - 4, file);
				file_offset += strlen(buffer_rx) - 4;
			}
		}
	}
 
	closesocket(socketS);
	fclose(file);
 
#endif
	//**********************************************************************
 
	getchar(); //wait for press Enter
	return 0;
}
 