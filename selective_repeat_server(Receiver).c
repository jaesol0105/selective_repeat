/*	Visual Studio
	�߰� ���Ӽ� : ws2_32.lib
	��� �μ� : 9000
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>

#define BUF_SIZE 10
#define WIN_SIZE 4
#define SEQ_SIZE 9

void ErrorHandling(char* message);

int server_buf[SEQ_SIZE] = { 0, }; // ���� ����
int win_pointer = 0; // RECV BASE

void display(int mode, int param) {
	for (int i = 0; i < 9; i++) {
		printf("%-4d", i);
	}
	printf("\t ����������:[%d %d %d %d]  ", win_pointer, (win_pointer + 1) % 9, (win_pointer + 2) % 9, (win_pointer + 3) % 9);

	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 9);
	printf("��");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf(":��������� ");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 13);
	printf("��");
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf(":��Ŷ�̼��ŵ� ");
	printf("\n");
	for (int i = 0; i < 9; i++) {
		if (server_buf[i] == 0) {
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
		else if (server_buf[i] == 1) {
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 13);
			printf("��  ");
			SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
		}
	}
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 4);
	if (mode == 1) {
		printf("\tSEQ %d ����\n", param);
	}
	else if (mode == 2) {
		printf("\t������ �̵�. RECV BASE: %d\n", param);
	}
	SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), 7);
	printf("\n");
}

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET servSock[SEQ_SIZE];
	SOCKADDR_IN servAdr[SEQ_SIZE], clntAdr;

	if (argc != 2) {
		printf("Usage : %s <port>\n", argv[0]);
		exit(1);
	}
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	for (int i = 0; i < SEQ_SIZE; i++) {
		servSock[i] = socket(PF_INET, SOCK_DGRAM, 0);
		if (servSock[i] == INVALID_SOCKET)
			ErrorHandling("UDP socket creation error");
	}
	for (int i = 0; i < SEQ_SIZE; i++) {
		memset(&servAdr[i], 0, sizeof(servAdr[i]));
		servAdr[i].sin_family = AF_INET;
		servAdr[i].sin_addr.s_addr = htonl(INADDR_ANY);
		servAdr[i].sin_port = htons(atoi(argv[1]) + i);
	}

	for (int i = 0; i < SEQ_SIZE; i++) {
		if (bind(servSock[i], (SOCKADDR*)&servAdr[i], sizeof(servAdr[i])) == SOCKET_ERROR)
			ErrorHandling("bind() error");
		DWORD recvTimeout = 100;
		setsockopt(servSock[i], SOL_SOCKET, SO_RCVTIMEO, (char*)&recvTimeout, sizeof(recvTimeout));
	}

	int count = 0; // ��Ŷ �ս� ������ ���� count
	int packet;
	int win_range[WIN_SIZE] = { 0, }; // ������ ���� üũ��
	int win_before_range[WIN_SIZE] = { 0, }; // ���� ��������� üũ��

	char message[BUF_SIZE];
	int strLen;

	int clntAdrSz;
	clntAdrSz = sizeof(clntAdr);

	while (1)
	{
		for (int i = 0; i < SEQ_SIZE; i++) {
			if ((strLen = recvfrom(servSock[i], message, BUF_SIZE, 0, (SOCKADDR*)&clntAdr, &clntAdrSz)) > 0) {
				
				Sleep(1000); // RTT�� 1�ʷ� ����.

				count++;
				// ���� 3ȸ �ֱ�� �۽����� ���� ��Ŷ�� �ս��� ����.
				if (count % 3 != 0)
				{
					packet = atoi(message);

					// ������ ���� üũ��.
					for (int j = 0; j < WIN_SIZE; j++) {
						win_range[j] = (win_pointer + j) % SEQ_SIZE;
						win_before_range[j] = (win_pointer - WIN_SIZE + j);
						if (win_before_range[j] < 0) {
							// [-1] -> [8],  [-2] -> [7],  [-3] -> [6],  [-4] -> [5]
							win_before_range[j] = SEQ_SIZE + win_before_range[j];
						}
					}
					// ��Ŷ�� ������ ���� ���� ���
					if (packet == win_range[0] ||
						packet == win_range[1] ||
						packet == win_range[2] ||
						packet == win_range[3])
					{
						// ���ۿ� ����
						server_buf[packet] = 1;
						display(1, packet); // printf("[%d��° ����] SEQ %d ��Ŷ ����\n", count, packet);

						// ACK ����
						sendto(servSock[i], message, strLen, 0, (SOCKADDR*)&clntAdr, sizeof(clntAdr));
						
						int check_win_move = 0;
						// ������ �������, '���� ������ ���� ���� ���� SEQ ��ȣ'���� �����츦 �̵�.
						while (1) {
							if (server_buf[win_pointer] == 1) {
								// ���ø����̼����� ����.
								server_buf[win_pointer] = 0;
								// printf("SEQ %d Application���� ����\n", win_pointer);
								win_pointer = (win_pointer + 1) % SEQ_SIZE;
								// printf("RECV BASE: %d\n", win_pointer);
								check_win_move = 1;
							}
							else {
								break;
							}
						}
						if (check_win_move == 1) {
							display(2, win_pointer);
						}
					}
					// ��Ŷ�� ���� ������ ������ ��� : ACK�� ����
					else if (packet == win_before_range[0] ||
						packet == win_before_range[1] ||
						packet == win_before_range[2] ||
						packet == win_before_range[3])
					{
						// ACK ����
						sendto(servSock[i], message, strLen, 0, (SOCKADDR*)&clntAdr, sizeof(clntAdr));
					}
					// �� ��
					else {
						// ����
					}
				}
				else
				{
					packet = atoi(message);
					printf("\n [%d��° ����] SEQ %d ��Ŷ �����߻�. ���. \n\n", count ,packet);
				}
			}
		}
	}
	closesocket(servSock);
	WSACleanup();
	return 0;
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}