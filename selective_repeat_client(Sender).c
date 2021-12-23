/*	Visual Studio
	�߰� ���Ӽ� : ws2_32.lib
	��� �μ� : 127.0.0.1 9000
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

int seq_num[SEQ_SIZE] = { 0,1,2,3,4,5,6,7,8 }; // ���� ��ȣ
int client_buf[SEQ_SIZE] = { 0, }; // �۽Ź���.
int win_pointer = 0; // SEND BASE

void display(int mode, int param) {
	EnterCriticalSection(&cs);

	for (int i = 0; i < 9; i++) {
		printf("%-4d", i);
	}
	printf("\t ����������:[%d %d %d %d]  ", win_pointer, (win_pointer + 1) % 9, (win_pointer + 2) % 9, (win_pointer + 3) % 9);

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
	printf("��");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf(":��������� ");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
	printf("��");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf(":����+ACK������ ");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
	printf("��");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf(":ACK����");
	printf("\n");
	for (int i = 0; i < 9; i++) {
		if (client_buf[i] == 0) {
			if (i == win_pointer || i == (win_pointer + 1) % 9 || i == (win_pointer + 2) % 9 || i == (win_pointer + 3) % 9) {
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
				printf("��  ");
				SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
			}
			else
			{
				printf("��  ");
			}
		}
		else if (client_buf[i] == 1) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 6);
			printf("��  ");
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
		}
		else {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 2);
			printf("��  ");
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
		}
	}
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
	if (mode == 1) {
		printf("\tSEQ %d ����\n", param);
	}
	else if (mode == 2) {
		printf("\tACK %d\n", param);
	}
	else if (mode == 3) {
		printf("\t������ �̵�. SEND BASE: %d\n", param);
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
	// ��Ŷ�� ���� �Ѵ�.
	retval = send(sock[seq], packet, strlen(packet), 0);
	if (retval == SOCKET_ERROR) {
		printf("send error\n");
		return 0;
	}
	display(1, seq); // printf("SEQ %d ����\n", seq);

	char ack[BUF_SIZE];
	int ack_;

	// ACK�� �޴´�. TIME OUT �߻��� ������.
	while (1) {
		if ((retval = recv(sock[seq], ack, sizeof(ack) - 1, 0)) < 0)
		{
			retval = sendto(sock[seq], packet, strlen(packet), 0, (SOCKADDR*)&servAdr[seq], sizeof(servAdr[seq]));
			printf("\n SEQ %d ��Ŷ timeout ! �� ���� \n\n", seq);
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

	// ACK ���� ���۰��� ���� �� ������ ó�� (2)
	ack[retval] = '\0';
	ack_ = atoi(ack);
	client_buf[ack_] = 2; // 0:��ĭ  1:�۽ſϷ�+ACK������  2:ACK����.
	display(2, ack_); // printf("ACK %d\n", ack_);

	int check_win_move = 0;
	// �������� SEND BASE(win_pointer) ���� '���� ������ ACK�� �������� ��Ŷ'���� �̵�
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
		DWORD recvTimeout = 10000;  // Ÿ�Ӿƿ� ���� 10��.
		setsockopt(sock[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&recvTimeout, sizeof(recvTimeout));
	}

	InitializeCriticalSection(&cs);

	int idx;
	while (1)
	{
		// ������ ũ�⿡ �ش��ϴ� SEQ ��ȣ ��Ŷ�鿡 ����
		for (int i = win_pointer; i < win_pointer + WIN_SIZE; i++) {
			if (i >= SEQ_SIZE) {
				idx = i % SEQ_SIZE;
			}
			else {
				idx = i;
			}
			// ���� �Ⱥ��� ��Ŷ�� ��� �����带 ���� ��Ŷ ����
			if (client_buf[idx] == 0) {
				Sleep(100);
				client_buf[idx] = 1; // 1:�۽ſϷ�+ACK������
				// ������� �ش� SEQ�� ��Ŷ�� ���� ACK�� recv�ϰ� ������ �̵����� ����.
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
