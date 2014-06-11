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

enum server_type {
    MPROCESS = 0, MTHREADED, EVENTED,
};

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
    bool Receive(bool verbose);
    bool SendResponse(string buffer);
    bool Close();
};

class HttpServer {
private: 
    SocketServer server;
    double elapsedtime;
    vector<HttpRequest*> requestcache;
    vector<string*> responsecache;
public:
    // Constructor/Destructor
    HttpServer();
    ~HttpServer();

    // Event loop methods
    void Run(server_type type, bool verbose);
    void RunMultiProcessed(bool verbose);
    void RunMultiThreaded(bool verbose);
    void RunEvented(bool verbose);

    // Request handling methods
    void ParseRequest(HttpRequest& request, const char* recvbuf);
    string HandleRequest(HttpRequest& request, bool verbose);

    // Response creating method
    string CreateResponse(HttpRequest request, http_status_t status);
    
    // Helper methods
    http_method_t GetMethod(const string method);
    http_version_t GetVersion(const string version);
    string GetMimeType(string extension);
    void ParseUri(string& uri, string& path, string& query, string& type);
};

#endif

// End of header
