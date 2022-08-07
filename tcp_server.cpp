/* ************************************
* �ο�
* 
*����ͨWindows API��ʾ������
* 14.1 socketͨ�� server.c
**************************************/
#include "tcp_server.h"


TcpServer::TcpServer(PCSTR port, void (*callback)(int id, char* data, int length), SIZE_T max_request, SIZE_T buf_size)
{
	m_port = port;
	m_callback = callback;
	m_max_request = max_request;
	m_buf_size = buf_size;
}

TcpServer::~TcpServer()
{
	if (m_running)
	{
		m_running = false;
	}
}

int TcpServer::communication_thread(int id, SOCKET client_socket)
{
	int connection_id = id;
	SOCKET socket = client_socket;
	// Ϊ�������ݷ���ռ�
	LPSTR szRequest = (LPSTR)HeapAlloc(GetProcessHeap(), 0, m_max_request);
	int iResult;

	for (;;)
	{
		ZeroMemory(szRequest, m_max_request);
		// ��������
		iResult = recv(socket, // socket
			szRequest, // ���ջ���
			m_max_request, // �����С
			0);// ��־
		if (iResult == 0)// ��������ʧ�ܣ������Ѿ��ر�
		{
			cout << "Connection closing..." << endl;
			HeapFree(GetProcessHeap(), 0, szRequest);
			closesocket(socket);
			return 1;
		}
		else if (iResult == SOCKET_ERROR)// ��������ʧ�ܣ�socket����
		{
			cout << "recv failed: " << WSAGetLastError() << endl;
			HeapFree(GetProcessHeap(), 0, szRequest);
			closesocket(socket);
			return 1;
		}
		else if (iResult > 0) // �������ݳɹ�
		{
			m_callback(connection_id, szRequest, iResult);
		}
	}
	// �ͷŽ������ݻ��棬�ر�socket
	HeapFree(GetProcessHeap(), 0, szRequest);
	closesocket(socket);

	return 0;
}

bool TcpServer::start()
{
	WSADATA wsaData;
	struct addrinfo* result = NULL, hints;
	int iResult;

	// ��ʼ��Winsock����֤Ws2_32.dll�Ѿ�����
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cout << "WSAStartup failed: " << iResult << endl;
		return false;
	}

	// ��ַ
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// ��ȡ������ַ����֤����Э����õ�
	iResult = getaddrinfo(NULL, // ����
		m_port, // �˿�
		&hints, // ʹ�õ�����Э�飬�������͵�
		&result);// ���
	if (iResult != 0)
	{
		cout << "getaddrinfo failed: " << iResult << endl;
		WSACleanup();
		return false;
	}

	// ����socket�����ڼ���
	ListenSocket = socket(
		result->ai_family,	// ����Э�飬AF_INET��IPv4
		result->ai_socktype,	// ���ͣ�SOCK_STREAM
		result->ai_protocol	// ͨ��Э�飬TCP
	);
	if (ListenSocket == INVALID_SOCKET)
	{
		cout << "socket failed: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	// �󶨵��˿�
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		cout << "bind failed: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}

	freeaddrinfo(result); // reuslt����ʹ��

	// ��ʼ����
	iResult = listen(ListenSocket, 5);	// �ڶ����������������������������ΪSOMAXCONN
	cout << "start listen..." << endl;
	if (iResult == SOCKET_ERROR)
	{
		cout << "listen failed: " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}

	m_running = true;
	while (m_running)
	{
		SOCKET ClientSocket = INVALID_SOCKET;
		// ���տͻ��˵����ӣ�accept������ȴ���ֱ�����ӽ���
		cout << "ready to accept" << endl;
		ClientSocket = accept(ListenSocket, NULL, NULL);
		// accept�������أ�˵���Ѿ��пͻ�������
		// ��������socket
		cout << "accept a connetion" << endl;
		if (ClientSocket == INVALID_SOCKET)
		{
			cout << "accept failed: " << WSAGetLastError() << endl;
			closesocket(ListenSocket);
			break;// �ȴ����Ӵ����˳�ѭ��
		}
		// ����
		ClientSockets.push_back(ClientSocket);
		// Ϊÿһ�����Ӵ���һ�����ݷ��͵Ľ����̣߳�
		// ʹ������ֿ����������������ͻ��˵�����
		thread(&TcpServer::communication_thread, this, connections, ClientSocket).detach();
		connections++;
	}
	// ѭ���˳����ͷ�DLL��
	WSACleanup();

	return true;
}

bool TcpServer::start_async()
{
	if (m_running)
	{
		cout << "already running!" << endl;
		return false;
	}
	thread(&TcpServer::start, this).detach();
	return true;
}

bool TcpServer::send_data(int id, char* data, int length)
{
	if ((id < 0) || (ClientSockets.size() <= id) || (ClientSockets[id] == INVALID_SOCKET))
	{
		cout << "no connection on " << id << endl;
		return false;
	}

	int bytesSent;// ���ڱ���send�ķ���ֵ��ʵ�ʷ��͵����ݵĴ�С

	bytesSent = send(ClientSockets[id], data, length, 0);
	if (bytesSent == SOCKET_ERROR)
	{
		cout << "send error " << WSAGetLastError() << endl;
		return false;
	}

	// ��ʾ�������ݵĴ�С
	cout << "send " << bytesSent << " bytes to " << id << endl;

	return true;
}

bool TcpServer::stop_after_next_connection()
{
	if (!m_running)
	{
		cout << "server is not running or is waiting to stop." << endl;
		return false;
	}
	m_running = false;
	return true;
}
