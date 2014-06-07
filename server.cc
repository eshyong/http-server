#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <fcntl.h>
#include "http.h"
#include "server.h"

using std::cerr;
using std::cout;
using std::endl;
using std::fstream;
using std::string;
using std::to_string;

static bool running = true;

////////////////////////////////////////////////
//              Sig Handlers                  //
////////////////////////////////////////////////
void handleSigint(int signum) {
    // Turn off event loop
    running = false;
}

void handleSigchld(int signum) {
    // Prevent zombie processes
    int status;
    while (waitpid(-1, &status, WNOHANG) == 0);
}

////////////////////////////////////////////////
//              SocketServer                  //
////////////////////////////////////////////////

SocketServer::SocketServer() {
    int error = 0;
    int flags = 0;

    // Zero initialize buffers and socket addresses
    memset(recvbuf, (char) NULL, sizeof(recvbuf));
    memset(peername, (char) NULL, sizeof(peername));
    memset(&serveraddr, (char) NULL, sizeof(serveraddr));
    memset(&clientaddr, (char) NULL, sizeof(clientaddr));

    // Create a socket and bind to our host address
    listening = socket(AF_INET, SOCK_STREAM, 0);
    if (listening < 0) {
        perror("socket");
        close(listening);
        exit(EXIT_FAILURE);
    }

    // Set socket reuse and non-blocking options
    setsockopt(listening, SOL_SOCKET, SO_REUSEADDR, NULL, 0);
    flags = fcntl(listening, F_GETFL, 0);
    fcntl(listening, F_SETFL, flags | O_NONBLOCK);

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
    socklen_t length = sizeof(clientaddr);
    int error = 0;
    connection = accept(listening, NULL, NULL);
    if (connection < 0) {
        // Expected behavior since we are using non-blocking sockets
        return false;
    }
    // Get connecting client's network info
    error = getpeername(connection, (struct sockaddr *)&clientaddr, &length);
    if (error < 0) {
        perror("getpeername");
        memset(peername, (char) NULL, sizeof(peername));
    } else {
        inet_ntop(AF_INET, &(clientaddr.sin_addr), peername, length);
    }
    return true;
}

bool SocketServer::Receive() {
    int count = recv(connection, recvbuf, BUFFER_LENGTH, 0);
    if (count <= 0) {
        return false;
    }
    recvbuf[count] = (char) NULL;
    cout << "Received " << count << " bytes from " << peername << ":\n";
    cout << recvbuf << endl;
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

////////////////////////////////////////////////
//              HttpServer                    //
////////////////////////////////////////////////
HttpServer::HttpServer() { elapsedtime = 0.0; }

HttpServer::~HttpServer() {}

void HttpServer::Run() {
    pid_t pid;
    time_t begin;
    time_t end;
    int status;
    cout << "Server starting...\n\n";

    // Add signal handlers
    signal(SIGINT, handleSigint);
    signal(SIGCHLD, handleSigchld);

    // Event loop
    while (running) {
        HttpRequest request;

        // Wait for a connection
        if (server.Connect()) {
            // Fork a new server process to handle client connection
            pid = fork();
            if (pid < 0) {
                // Error 
                perror("fork");
                continue;
            } else if (pid == 0) {
                // Child process
                // Begin timer
                time(&begin);
                time(&end);
                elapsedtime = difftime(end, begin);
                
                do {
                    if (server.Receive() && ParseRequest(request, server.get_buffer())) {
                        // Handle request and response
                        HandleRequest(request);
                    }
                    // Calculate elapsed time
                    time(&end);
                    elapsedtime = difftime(end, begin);
                } while (elapsedtime < TIME_OUT);

                // Close connection and exit
                server.Close();
                exit(EXIT_SUCCESS);
            } else {
                // Parent process
                server.Close();
            }
            // Reset request
            request.Reset();
        }
        // Sleep if not connected
        usleep(SLEEP_MSEC);
    }
    cout << "Server shutting down...\n";
}

bool HttpServer::ParseRequest(HttpRequest& request, const char* recvbuf) {
    int i = 0;
    http_method_t method;
    http_version_t version;
    string buffer = "";
    string path = "";

    // Get method string
    while (i <= BUFFER_LENGTH && !isspace(recvbuf[i])) {
        buffer += recvbuf[i];
        i++;
    }
    i++;

    // Parse method
    method = GetMethod(buffer);
    if (method == INVALID_METHOD) {
        cout << "Unrecognized HTTP method\n";
    }

    // Get folder path
    path = DIRECTORY;
    while (i <= BUFFER_LENGTH && !isspace(recvbuf[i])) {
        path += recvbuf[i];
        i++;
    }
    i++;

    // Get version number
    buffer = "";
    while (i <= BUFFER_LENGTH && !isspace(recvbuf[i])) {
        buffer += recvbuf[i];
        i++;
    }
    i++;

    // Parse version number
    version = GetVersion(buffer);
    if (version == INVALID_VERSION) {
        cout << "Invalid HTTP version.\n";
    }

    // Fill request struct
    request.Initialize(path, method, version, NULL);
    return true;
}

bool HttpServer::HandleRequest(HttpRequest& request) {
    HttpResponse response;
    bool flag = request.get_flag();
    http_status_t status = OK;
    http_method_t method = request.get_method();
    http_version_t version = request.get_version();
    string path = request.get_path();
    fstream file;
    
    if (flag) {
        // Request entity too large
        status = REQUEST_ENTITY_TOO_LARGE;
    } else if (version == INVALID_VERSION) {
        // Invalid request
        status = BAD_REQUEST;
    } else if (method == INVALID_METHOD) {
        // Unrecognized method
        status = NOT_IMPLEMENTED;
    } else {
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
            // Unimplemented methods
            cout << "Not implemented yet\n";
            status = NOT_IMPLEMENTED;
        }
    }

    // Send response
    response.Initialize(&file, method, version, status, NULL);
    server.SendResponse(response.to_string());
    return true;
}

http_method_t HttpServer::GetMethod(const string method) {
    if (method.compare("GET") == 0) {
        return GET;
    } else if (method.compare("POST") == 0) {
        return POST;
    } else if (method.compare("PUT") == 0) {
        return PUT;
    } else if (method.compare("DELETE") == 0) {
        return DELETE;
    }
    return INVALID_METHOD;
}

http_version_t HttpServer::GetVersion(const string version) {
    if (version.compare("HTTP/1.0") == 0) {
        return ONE;
    } else if (version.compare("HTTP/1.1") == 0) {
        return TWO;
    } else if (version.compare("HTTP/2.0") == 0) {
        return THREE;
    }
    return INVALID_VERSION;
}

////////////////////////////////////////////////
//              HttpRequest                   //
////////////////////////////////////////////////
HttpRequest::HttpRequest(string path, http_method_t method, http_version_t version, HeaderOptions* options) {
    Initialize(path, method, version, options);
}

HttpRequest::HttpRequest() {
    Initialize("", INVALID_METHOD, INVALID_VERSION, NULL);
}

void HttpRequest::Initialize(string path, http_method_t method, http_version_t version, HeaderOptions* options) {
    toolong = false;
    this->path = path;
    this->method = method;
    this->version = version;
    this->options = options;
}

void HttpRequest::Reset() {
    path = "";
    method = INVALID_METHOD;
    version = INVALID_VERSION;
}

HttpRequest::~HttpRequest() {
}

////////////////////////////////////////////////
//              HttpResponse                  //
////////////////////////////////////////////////
HttpResponse::HttpResponse(fstream* file, http_method_t method, http_version_t version, http_status_t status, HeaderOptions* options) {
    Initialize(file, method, version, status, options);
}

HttpResponse::HttpResponse() {
    stringrep = "";
    method = INVALID_METHOD;
    version = INVALID_VERSION;
}

void HttpResponse::Initialize(fstream* file, http_method_t method, http_version_t version, http_status_t status, HeaderOptions* options) {
    // Create the string representation 
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

void HttpResponse::Reset() {
    stringrep = "";
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
