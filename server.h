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

using std::pair;

enum server_type {
    MPROCESS = 0, MTHREADED, EVENTED,
};

struct mthreaded_request_args {
    pair<int, string> client;
    void* ptr;
    bool verbose;
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
    char recvbuf[BUFFER_LENGTH + 1];
public:
    // Constructor/Destructor
    SocketServer();
    ~SocketServer();

    // Receiving buffer
    const char* get_buffer() { return recvbuf; }

    // Socket call wrapper methods
    pair<int, string> Connect();
    bool Receive(bool verbose, pair<int, string> client);
    bool SendResponse(string buffer, int connection);
    bool Close(int connection);
};

class HttpServer {
private: 
    SocketServer server;
    vector<pair<HttpRequest*, string> > cache;
    double elapsedtime;
    pthread_attr_t attr;
    pthread_mutex_t cachemutex;
    bool verbose;
public:
    // Constructor/Destructor
    HttpServer();
    ~HttpServer();

    // Multi-process request handling
    void Run(server_type type, bool verbose);
    void RunMultiProcessed(bool verbose);
    void DispatchRequestToChild(bool verbose, pair<int, string> client);

    // Multi-threaded request handling
    void RunMultiThreaded(bool verbose);
    void* DispatchRequestToThread(bool verbose, pair<int, string> client);
    static void* CallDispatchRequestToThread(void* args); 
    
    // Evented request handling
    void RunEvented(bool verbose);

    // Request handling methods
    void ParseRequest(HttpRequest& request, const char* recvbuf);
    string HandleRequestThreaded(HttpRequest& request, bool verbose, bool& cached);
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
