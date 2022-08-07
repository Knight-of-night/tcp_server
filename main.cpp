#include <iostream>
#include "tcp_server.h"

using std::cout;
using std::endl;

const PCSTR PORT = "11111";

int main()
{
	TcpServer server = TcpServer(PORT);
	
	server.start();
	
	return 0;
}
