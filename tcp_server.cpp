/* ************************************
* 参考
* 
*《精通Windows API》示例代码
* 14.1 socket通信 server.c
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
	// 为接收数据分配空间
	LPSTR szRequest = (LPSTR)HeapAlloc(GetProcessHeap(), 0, m_max_request);
	int iResult;

	for (;;)
	{
		ZeroMemory(szRequest, m_max_request);
		// 接收数据
		iResult = recv(socket, // socket
			szRequest, // 接收缓存
			m_max_request, // 缓存大小
			0);// 标志
		if (iResult == 0)// 接收数据失败，连接已经关闭
		{
			cout << "Connection closing..." << endl;
			HeapFree(GetProcessHeap(), 0, szRequest);
			closesocket(socket);
			return 1;
		}
		else if (iResult == SOCKET_ERROR)// 接收数据失败，socket错误
		{
			cout << "recv failed: " << WSAGetLastError() << endl;
			HeapFree(GetProcessHeap(), 0, szRequest);
			closesocket(socket);
			return 1;
		}
		else if (iResult > 0) // 接收数据成功
		{
			m_callback(connection_id, szRequest, iResult);
		}
	}
	// 释放接收数据缓存，关闭socket
	HeapFree(GetProcessHeap(), 0, szRequest);
	closesocket(socket);

	return 0;
}

bool TcpServer::start()
{
	WSADATA wsaData;
	struct addrinfo* result = NULL, hints;
	int iResult;

	// 初始化Winsock，保证Ws2_32.dll已经加载
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cout << "WSAStartup failed: " << iResult << endl;
		return false;
	}

	// 地址
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// 获取主机地址，保证网络协议可用等
	iResult = getaddrinfo(NULL, // 本机
		m_port, // 端口
		&hints, // 使用的网络协议，连接类型等
		&result);// 结果
	if (iResult != 0)
	{
		cout << "getaddrinfo failed: " << iResult << endl;
		WSACleanup();
		return false;
	}

	// 创建socket，用于监听
	ListenSocket = socket(
		result->ai_family,	// 网络协议，AF_INET，IPv4
		result->ai_socktype,	// 类型，SOCK_STREAM
		result->ai_protocol	// 通信协议，TCP
	);
	if (ListenSocket == INVALID_SOCKET)
	{
		cout << "socket failed: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return false;
	}

	// 绑定到端口
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		cout << "bind failed: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return false;
	}

	freeaddrinfo(result); // reuslt不再使用

	// 开始监听
	iResult = listen(ListenSocket, 5);	// 第二个参数是最大连接数，可以设置为SOMAXCONN
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
		// 接收客户端的连接，accept函数会等待，直到连接建立
		cout << "ready to accept" << endl;
		ClientSocket = accept(ListenSocket, NULL, NULL);
		// accept函数返回，说明已经有客户端连接
		// 返回连接socket
		cout << "accept a connetion" << endl;
		if (ClientSocket == INVALID_SOCKET)
		{
			cout << "accept failed: " << WSAGetLastError() << endl;
			closesocket(ListenSocket);
			break;// 等待连接错误，退出循环
		}
		// 保存
		ClientSockets.push_back(ClientSocket);
		// 为每一个连接创建一个数据发送的接收线程，
		// 使服务端又可以立即接收其他客户端的连接
		thread(&TcpServer::communication_thread, this, connections, ClientSocket).detach();
		connections++;
	}
	// 循环退出，释放DLL。
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

	int bytesSent;// 用于保存send的返回值，实际发送的数据的大小

	bytesSent = send(ClientSockets[id], data, length, 0);
	if (bytesSent == SOCKET_ERROR)
	{
		cout << "send error " << WSAGetLastError() << endl;
		return false;
	}

	// 显示发送数据的大小
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
