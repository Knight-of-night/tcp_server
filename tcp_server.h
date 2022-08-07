#pragma once

#include <iostream>
#include <vector>
#include <thread>
#include <WinSock2.h>
#include <WS2tcpip.h>

#pragma comment(lib,"Ws2_32.lib")

using namespace std;


class TcpServer {
private:
	PCSTR m_port; // �˿�
	SIZE_T m_max_request; // �������ݵĻ����С
	SIZE_T m_buf_size; // �������ݵĻ����С

	SOCKET ListenSocket = INVALID_SOCKET; // ����Socket
	//SOCKET ClientSocket = INVALID_SOCKET; // ����Socket
	vector<SOCKET> ClientSockets;
	bool m_running = false;
	int connections = 0;
	void (*m_callback)(int id, char* data, int length);

	int communication_thread(int id, SOCKET client_socket);

public:
	TcpServer(PCSTR port, void (*callback)(int id, char* data, int length) = [](int id, char* data, int length) { cout << data << endl; }, SIZE_T max_request = 1024, SIZE_T buf_size = 4096);
	~TcpServer();

	bool start();
	bool start_async();
	bool send_data(int id, char* data, int length);
	bool stop_after_next_connection();
};
