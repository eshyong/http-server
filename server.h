#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include "http.h"

class SocketServer {
private:
    // Addresses for the server and the client
    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;
    socklen_t address_len;
    char peername[INET_ADDRSTRLEN];
    
    // Socket file descriptors
    int listening;
    int connection;
    char recvbuf[BUFFER_LENGTH + 1];
public:
    SocketServer();
    ~SocketServer();
    const char* get_buffer() { return recvbuf; }
    bool Connect();
    bool SendResponse(string buffer);
    bool Receive();
    bool Close();
    bool IsGood();
};

class HttpServer {
private: 
    double elapsedtime;
    SocketServer server;
public:
    HttpServer();
    ~HttpServer();
    void Run();
    bool ParseRequest(HttpRequest& request, const char* recvbuf);
    bool HandleRequest(HttpRequest& request);
    string CreateResponse(HttpResponse& response, fstream& file);
    http_method_t GetMethod(const string method);
    http_version_t GetVersion(const string version);
};

#endif
// End of header
