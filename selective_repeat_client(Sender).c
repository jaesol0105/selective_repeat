/*	Visual Studio
	추가 종속성 : ws2_32.lib
	명령 인수 : 127.0.0.1 9000
*/
#pragma warning(disable : 4996)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <stdio.h>
#include <process.h>

#define BUF_SIZE 10
#define WIN_SIZE 4
#define SEQ_SIZE 9

void ErrorHandling(char* message);

WSADATA wsaData;
SOCKET sock[SEQ_SIZE];
SOCKADDR_IN servAdr[SEQ_SIZE];
HANDLE hThread[SEQ_SIZE];

CRITICAL_SECTION cs;

int seq_num[SEQ_SIZE] = { 0,1,2,3,4,5,6,7,8 }; // 순서 번호
int client_buf[SEQ_SIZE] = { 0, }; // 송신버퍼.
int win_pointer = 0; // SEND BASE

void display(int mode, int param) {
	EnterCriticalSection(&cs);

	for (int i = 0; i < 9; i++) {
		printf("%-4d", i);
	}
	printf("\t 현재윈도우:[%d %d %d %d]  ", win_pointer, (win_pointer + 1) % 9, (win_pointer + 2) % 9, (win_pointer + 3) % 9);

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
	printf("■");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf(":윈도우범위 ");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
	printf("■");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf(":보냄+ACK못받음 ");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	printf("■");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf(":ACK받음");
	printf("\n");
	for (int i = 0; i < 9; i++) {
		if (client_buf[i] == 0) {
			if (i == win_pointer || i == (win_pointer + 1) % 9 || i == (win_pointer + 2) % 9 || i == (win_pointer + 3) % 9) {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
				printf("■  ");
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
			}
			else
			{
				printf("■  ");
			}
		}
		else if (client_buf[i] == 1) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
			printf("■  ");
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
			printf("■  ");
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
		}
	}
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
	if (mode == 1) {
		printf("\tSEQ %d 전송\n", param);
	}
	else if (mode == 2) {
		printf("\tACK %d\n", param);
	}
	else if (mode == 3) {
		printf("\t윈도우 이동. SEND BASE: %d\n", param);
	}
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf("\n");

	LeaveCriticalSection(&cs);
}

unsigned WINAPI ThreadFunc(void* arg)
{
	int seq = *((int*)arg);
	char packet[BUF_SIZE];
	_itoa(seq, packet, 10);

	int retval;
	// 패킷을 전송 한다.
	retval = send(sock[seq], packet, strlen(packet), 0);
	if (retval == SOCKET_ERROR) {
		printf("send error\n");
		return 0;
	}
	display(1, seq); // printf("SEQ %d 전송\n", seq);

	char ack[BUF_SIZE];
	int ack_;

	// ACK를 받는다. TIME OUT 발생시 재전송.
	while (1) {
		if ((retval = recv(sock[seq], ack, sizeof(ack) - 1, 0)) < 0)
		{
			retval = sendto(sock[seq], packet, strlen(packet), 0, (SOCKADDR*)&servAdr[seq], sizeof(servAdr[seq]));
			printf("\n SEQ %d 패킷 timeout ! 재 전송 \n\n", seq);
			if (retval == SOCKET_ERROR) {
				printf("send error\n");
				return 0;
			}
			continue;
		}
		else {
			break;
		}
	}

	// ACK 받은 버퍼값을 수신 된 것으로 처리 (2)
	ack[retval] = '\0';
	ack_ = atoi(ack);
	client_buf[ack_] = 2; // 0:빈칸  1:송신완료+ACK못받음  2:ACK받음.
	display(2, ack_); // printf("ACK %d\n", ack_);

	int check_win_move = 0;
	// 윈도우의 SEND BASE(win_pointer) 값을 '가장 오래된 ACK를 받지못한 패킷'으로 이동
	while (1) {
		if (client_buf[win_pointer] == 2) {
			client_buf[win_pointer] = 0;
			win_pointer = (win_pointer + 1) % SEQ_SIZE;
			check_win_move = 1;
			// printf("SEND BASE: %d\n", win_pointer);
		}
		else {
			break;
		}
	}
	if (check_win_move == 1) {
		display(3, win_pointer);
	}
	return 1;
}

int main(int argc, char* argv[])
{
	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	for (int i = 0; i < SEQ_SIZE; i++) {
		sock[i] = socket(PF_INET, SOCK_DGRAM, 0);
		if (sock[i] == INVALID_SOCKET)
			ErrorHandling("socket() error");
	}

	for (int i = 0; i < SEQ_SIZE; i++) {
		memset(&servAdr[i], 0, sizeof(servAdr[i]));
		servAdr[i].sin_family = AF_INET;
		servAdr[i].sin_addr.s_addr = inet_addr(argv[1]);
		servAdr[i].sin_port = htons(atoi(argv[2]) + i);
	}

	for (int i = 0; i < SEQ_SIZE; i++) {
		connect(sock[i], (SOCKADDR*)&servAdr[i], sizeof(servAdr[i]));
		DWORD recvTimeout = 10000;  // 타임아웃 설정 10초.
		setsockopt(sock[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&recvTimeout, sizeof(recvTimeout));
	}

	InitializeCriticalSection(&cs);

	int idx;
	while (1)
	{
		// 윈도우 크기에 해당하는 SEQ 번호 패킷들에 대해
		for (int i = win_pointer; i < win_pointer + WIN_SIZE; i++) {
			if (i >= SEQ_SIZE) {
				idx = i % SEQ_SIZE;
			}
			else {
				idx = i;
			}
			// 아직 안보낸 패킷일 경우 쓰레드를 통해 패킷 전송
			if (client_buf[idx] == 0) {
				Sleep(100);
				client_buf[idx] = 1; // 1:송신완료+ACK못받음
				// 쓰레드는 해당 SEQ의 패킷에 대한 ACK를 recv하고 윈도우 이동까지 수행.
				hThread[idx] = (HANDLE)_beginthreadex(NULL, 0, ThreadFunc, (void*)&seq_num[idx], 0, NULL);
			}
		}
		WaitForMultipleObjects(SEQ_SIZE+1,hThread,FALSE,INFINITE);
    
	}
	
	DeleteCriticalSection(&cs);
	closesocket(sock);
	WSACleanup();
	return 0;
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
