# tcp_server
A TCP server written in C++.
I plan to use it in my other projects.

## Usage

### Initialize
```
TcpServer server = TcpServer(PCSTR port);
// or
TcpServer server = TcpServer(PCSTR port, void (*callback)(int id, char* data, int length), SIZE_T max_request, SIZE_T buf_size);
```
The `callback` parameter is called when we receive data from client.

### Start server
```
server.start()
// or
server.start_async()
```
The `start_async` opens a thread to start server.

### Send data to client
```
server.send_data(int id, char* data, int length);
```

### Stop
```
server.stop_after_next_connection()
```
It is not a real stop function. The server doesn't stop until next connection.

## License
MIT License.
