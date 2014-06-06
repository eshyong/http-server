#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "http.h"
#include "server.h"

using std::cerr;
using std::cout;
using std::endl;
using std::fstream;
using std::string;
using std::to_string;

////////////////////////////////////////////////
//              SocketServer                  //
////////////////////////////////////////////////

// Constructor zero initializes buffer, 
SocketServer::SocketServer() {
    int error = 0;
    good = true;

    // Zero initialize buffers and socket addresses
    memset(recvbuf, 0, sizeof(recvbuf));
    memset(&serveraddr, 0, sizeof(serveraddr));
    memset(&client, 0, sizeof(client));

    // Create a socket and bind to our host address
    listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening < 0) {
        perror("socket");
        close(listening);
        exit(EXIT_FAILURE);
    }

    // Set socket reuse options
    setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, NULL, 0);

    // Server socket information
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(PORT);

    // Bind socket to a local address
    error = bind(listening, (const struct sockaddr *) &serveraddr, sizeof(serveraddr));
    if (error < 0) {
        perror("bind");
        close(listening);
        exit(EXIT_FAILURE);
    }

    // Listen for connections (1 for now)
    error = listen(listening, 1);
    if (error < 0) {
        perror("listen");
        close(listening);
        exit(EXIT_FAILURE);
    }
}

SocketServer::~SocketServer() {
    // Close listening socket and any connections
    close(listening);
    close(connection);
}

bool SocketServer::Connect() {
    // Accept any incoming connections
    connection = accept(listening, NULL, NULL);
    if (connection < 0) {
        perror("accept");
        return false;
    }
    return true;
}

bool SocketServer::Receive() {
    int count = recv(connection, recvbuf, BUFFER_LENGTH, 0);
    if (count < 0) {
        perror("recv");
        return false;
    }
    recvbuf[BUFFER_LENGTH] = (char) NULL;
    cout << "Message: " << recvbuf << endl;
    return true;
}

bool SocketServer::Close() {
    int error = close(connection);
    if (error < 0) {
        perror("close");
        return false;
    }
    return true;
}

bool SocketServer::SendResponse(string buffer) {
    // Send buffer over socket, no need for NULL termination
    int count = send(connection, buffer.c_str(), buffer.length() - 1, 0);
    if (count < 0) {
        perror("send");
        return false;
    }
    return true;
}

bool SocketServer::IsGood() {
    return good;
}

////////////////////////////////////////////////
//              HttpServer                    //
////////////////////////////////////////////////
HttpServer::HttpServer() {}

HttpServer::~HttpServer() {}

void HttpServer::Run() {
    while (server.IsGood()) {
        HttpRequest request;

        // Wait for a connection
        if (server.Connect()) {
            // Receive bytes from client and parse the request
            if (server.Receive() && ParseRequest(request, server.get_buffer())) {
                // Send response
                HandleRequest(request);
            }
            // Close connection
            server.Close();
        }

        // Reset request
        request.Reset();
    }
}

bool HttpServer::ParseRequest(HttpRequest& request, const char* recvbuf) {
    char buffer[BUFFER_LENGTH + 1];
    char pathbuf[BUFFER_LENGTH + 1];
    char* path;
    int i = 0;
    int j = 0;
    http_method_t method;
    http_version_t version;

    // Zero out previous memory
    memset(pathbuf, 0, sizeof(pathbuf));
    memset(buffer, 0, sizeof(buffer));

    // Get method string
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
        return false;
    }

    // Get folder path
    j = 0;
    while (i <= BUFFER_LENGTH && j <= BUFFER_LENGTH && !isspace(recvbuf[i])) {
        buffer[j] = recvbuf[i];
        i++;
        j++;
    }
    i++;
    
    // Add requested path to current working directory
    buffer[j] = (char) NULL;
    strncpy(pathbuf, DIRECTORY, strlen(DIRECTORY));
    strncat(pathbuf, buffer, strlen(buffer));
    path = strndup(pathbuf, BUFFER_LENGTH);

    // Get version number
    j = 0;
    while (i <= BUFFER_LENGTH && j <= BUFFER_LENGTH && !isspace(recvbuf[i])) {
        buffer[j] = recvbuf[i];
        i++;
        j++;
    }

    // Parse version number
    buffer[j] = (char) NULL;
    version = GetVersion(buffer);
    if (version == INVALID_VERSION) {
        cout << "Invalid HTTP version.\n";
        return false;
    }

    // Fill request struct
    request.set_path(path);
    request.set_version(version);
    request.set_method(method);
    return true;
}

bool HttpServer::HandleRequest(HttpRequest& request) {
    http_status_t status = OK;
    http_method_t method = request.get_method();
    http_version_t version = request.get_version();
    const char* path = request.get_path();
    fstream file;
    
    // Handle each HTTP method
    if (method == GET) {
        // Get URI resource by opening file
        file.open(path, fstream::in);
        if (!file.is_open()) {
            // 404 Error
            perror("open");
            status = NOT_FOUND;
        }
    } else {
        cout << "Not implemented yet\n";
        status = NOT_IMPLEMENTED;
    }

    HttpResponse response(&file, method, version, status);
    server.SendResponse(response.to_string());
    return true;
}

http_method_t HttpServer::GetMethod(const char* string) {
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

http_version_t HttpServer::GetVersion(const char* string) {
    if (strcmp(string, "HTTP/1.0") == 0) {
        return ONE;
    } else if (strcmp(string, "HTTP/1.1") == 0) {
        return TWO;
    } else if (strcmp(string, "HTTP/2.0") == 0) {
        return THREE;
    }
    return INVALID_VERSION;
}

////////////////////////////////////////////////
//              HttpRequest                   //
////////////////////////////////////////////////
HttpRequest::HttpRequest(char* path, http_method_t method, http_version_t version) {
    set_path(path);
    this->method = method;
    this->version = version;
}

HttpRequest::HttpRequest() {
    method = INVALID_METHOD;
    version = INVALID_VERSION;
}

void HttpRequest::Reset() {
    method = INVALID_METHOD;
    version = INVALID_VERSION;
}

HttpRequest::~HttpRequest() {
}

////////////////////////////////////////////////
//              HttpResponse                  //
////////////////////////////////////////////////
HttpResponse::HttpResponse(fstream* file, http_method_t method, http_version_t version, http_status_t status) {
    this->file = file;
    this->method = method;
    this->version = version;
    this->status = status;
    Initialize();
}

void HttpResponse::Initialize() {
    // Create the string representation 
    int count = 0;
    int index = 0;
    char c;
    string body;

    // Create a buffer and fill with information from response
    stringrep = "";
    body = "";

    // Check method type
    if (method == GET && status == OK) {
        // Get all characters in file
        while (file->good() && index <= BODY_LENGTH) {
            body += (char) file->get();
            index++;
        }
    }

    // Append status line, options, and body to buffer
    stringrep += versions[version];
    stringrep += SPACE;
    stringrep += statuses[status];
    stringrep += CRLF;
    stringrep += "Accept-Ranges: bytes";
    stringrep += CRLF;
    stringrep += "Content-Type: text/html";
    stringrep += CRLF;
    stringrep += "Content-Length: ";
    stringrep += ::to_string(body.length() - 1);
    stringrep += CRLF;
    stringrep += CRLF;
    stringrep += body;
}

HttpResponse::HttpResponse() {
    method = INVALID_METHOD;
    version = INVALID_VERSION;
}

void HttpResponse::Reset() {
    method = INVALID_METHOD;
    version = INVALID_VERSION;
}

HttpResponse::~HttpResponse() {
}

////////////////////////////////////////////////
//                  Main                      //
////////////////////////////////////////////////
int main() {
    HttpServer http;
    http.Run();
    return 0;
}
