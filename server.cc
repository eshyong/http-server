#include "server.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;

HttpServer::HttpServer() {
    int error = 0;
    good = true;

    // Zero initialize buffers and socket addresses
    memset(sendbuf, 0, sizeof(sendbuf));
    memset(recvbuf, 0, sizeof(recvbuf));
    memset(&server, 0, sizeof(server));
    memset(&client, 0, sizeof(client));

    // Create a socket and bind to our host address
    listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening < 0) {
        cerr << "Socket creation failed.\n";
        close(listening);
        exit(EXIT_FAILURE);
    }

    // Set socket reuse options
    setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, NULL, 0);

    // Server socket information
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(PORT);

    // Bind socket to a local address
    error = bind(listening, (const struct sockaddr *) &server, sizeof(server));
    if (error < 0) {
        perror("bind");
        close(listening);
        exit(EXIT_FAILURE);
    }

    // Listen for connections (1 for now)
    listen(listening, 1);
}

HttpServer::~HttpServer() {
    // Close listening socket and any connections
    close(listening);
    close(connection);
}

void HttpServer::Run() {
    HttpRequest* request;

    // Wait for a connection
    Connect();

    // Receive bytes from client
    GetRequest();

    // Parse request
    request = ParseRequest();

    // Send response
    SendResponse();

    // Close the connection
    close(connection);

    // Clean up
    delete request;
}

void HttpServer::Connect() {
    // Accept any incoming connections
    connection = accept(listening, NULL, NULL);
    if (connection < 0) {
        cerr << "Error accepting connection.\n";
        close(listening);
        exit(EXIT_FAILURE);
    }
}

void HttpServer::GetRequest() {
    int count = recv(connection, recvbuf, BUFFER_LENGTH, 0);
    bool status;
    recvbuf[BUFFER_LENGTH] = (char) NULL;
    cout << "Message: " << recvbuf;
}

HttpRequest* HttpServer::ParseRequest() {
    char buffer[BUFFER_LENGTH + 1];
    int i = 0;
    int j = 0;
    http_method_t method;
    http_version_t version;
    char* path;

    // Get method string
    memset(buffer, 0, sizeof(buffer));
    while (i <= BUFFER_LENGTH && j <= BUFFER_LENGTH && !isspace(recvbuf[i])) {
        buffer[j] = recvbuf[i];
        i++;
        j++;
    }
    i++;

    // Parse method
    buffer[j] = (char) NULL;
    method = GetMethod(buffer);
    if (method == INVALID_METHOD) {
        cout << "Invalid HTTP method\n";
        return NULL;
    }

    // Get folder path
    j = 0;
    while (i <= BUFFER_LENGTH && j <= BUFFER_LENGTH && !isspace(recvbuf[i])) {
        buffer[j] = recvbuf[i];
        i++;
        j++;
    }
    i++;
    path = strndup(buffer, BUFFER_LENGTH);

    // Get version number
    j = 0;
    while (i <= BUFFER_LENGTH && j <= BUFFER_LENGTH && !isspace(recvbuf[i])) {
        buffer[j] = recvbuf[i];
        i++;
        j++;
    }
    version = GetVersion(buffer);
    if (version == INVALID_VERSION) {
        cout << "Invalid HTTP version.\n";
        return NULL;
    }

    return new HttpRequest(path, method, version);
}

void HttpServer::SendResponse() {
    strncpy(sendbuf, "HTTP/1.1 200 OK\r\n", BUFFER_LENGTH);
    sendbuf[BUFFER_LENGTH] = (char) NULL;
    int count = send(connection, sendbuf, strlen(sendbuf) + 1, 0);
}

http_method_t HttpServer::GetMethod(const char *string) {
    if (strcmp(string, "GET") == 0) {
        return GET;
    } else if (strcmp(string, "POST") == 0) {
        return POST;
    } else if (strcmp(string, "PUT") == 0) {
        return PUT;
    } else if (strcmp(string, "DELETE") == 0) {
        return DELETE;
    }
    return INVALID_METHOD;
}

http_version_t HttpServer::GetVersion(const char *string) {
    if (strcmp(string, "HTTP/1.0") == 0) {
        return ONE;
    } else if (strcmp(string, "HTTP/1.1") == 0) {
        return TWO;
    } else if (strcmp(string, "HTTP/2.0") == 0) {
        return THREE;
    }
    return INVALID_VERSION;
}

bool HttpServer::IsGood() {
    return good;
}

HttpRequest::HttpRequest(char* path, http_method_t method, http_version_t version) {
    this->path = path;
    this->method = method;
    this->version = version;
}

int main() {
    HttpServer server;
    while (server.IsGood()) {
        server.Run();
    }
    return 0;
}
