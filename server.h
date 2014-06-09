#pragma once
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
    // Constructor/Destructor
    SocketServer();
    ~SocketServer();

    // Receiving buffer
    const char* get_buffer() { return recvbuf; }

    // Socket call wrapper methods
    bool Connect();
    bool Receive();
    bool SendResponse(string buffer);
    bool Close();
};

class HttpServer {
private: 
    double elapsedtime;
    SocketServer server;
public:
    // Constructor/Destructor
    HttpServer();
    ~HttpServer();

    // Event loop 
    void Run();

    // Request handling methods
    void ParseRequest(HttpRequest& request, const char* recvbuf);
    bool HandleRequest(HttpRequest& request);

    // Response creating method
    string CreateResponse(HttpResponse& response, fstream& file);
    
    // Helper methods
    http_method_t GetMethod(const string method);
    http_version_t GetVersion(const string version);
    void ParseUri(string& uri, string& path, string& query);
};

#endif

// End of header
