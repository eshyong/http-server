#include "server.h"
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;
using std::fstream;
using std::string;
using std::to_string;

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
        perror("socket");
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
    error = listen(listening, 1);
    if (error < 0) {
        perror("listen");
        close(listening);
        exit(EXIT_FAILURE);
    }
}

HttpServer::~HttpServer() {
    // Close listening socket and any connections
    close(listening);
    close(connection);
}

void HttpServer::Run() {
    HttpRequest request;

    // Wait for a connection
    if (Connect()) {
        // Receive bytes from client and parse the request
        if (GetRequest() && ParseRequest(request)) {
            // Send response
            HandleRequest(request);
        }
        // Close connection
        close(connection);
    }

    // Reset request
    request.Reset();
}

bool HttpServer::Connect() {
    // Accept any incoming connections
    connection = accept(listening, NULL, NULL);
    if (connection < 0) {
        cerr << "accept: " << strerror(errno) << endl;
        return false;
    }
    return true;
}

bool HttpServer::GetRequest() {
    int count = recv(connection, recvbuf, BUFFER_LENGTH, 0);
    if (count < 0) {
        cerr << "recv: " << strerror(errno) << endl;
        return false;
    }
    recvbuf[BUFFER_LENGTH] = (char) NULL;
    cout << "Message: " << recvbuf << endl;
    return true;
}

bool HttpServer::ParseRequest(HttpRequest& request) {
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
            cerr << "open: " << strerror(errno) << endl;
            status = NOT_FOUND;
        }
    } else {
        cout << "Not implemented yet\n";
        status = NOT_IMPLEMENTED;
    }

    HttpResponse response(&file, method, version, status);
    SendResponse(response, file);
    return true;
}

bool HttpServer::SendResponse(HttpResponse& response, fstream& file) {
    int count = 0;
    int index = 0;
    char c;
    
    http_version_t version = response.get_version();
    http_method_t method = response.get_method();
    http_status_t status = response.get_status();

    // Create a buffer and fill with information from response
    string buffer = "";
    string body = "";

    // Check method type
    if (method == GET && status == OK) {
        // Get all characters in file
        while (file.good() && index <= BODY_LENGTH) {
            body += (char) file.get();
            index++;
        }
    }

    // Append status line to buffer
    buffer += versions[version];
    buffer += SPACE;
    buffer += statuses[status];
    buffer += CRLF;
    buffer += "Accept-Ranges: bytes";
    buffer += CRLF;
    buffer += "Content-Type: text/html";
    buffer += CRLF;
    buffer += "Content-Length: ";
    buffer += to_string(body.length() - 1);
    buffer += CRLF;
    buffer += CRLF;
    buffer += body;
    
    // Send buffer over socket, no need for NULL termination
    count = send(connection, buffer.c_str(), buffer.length() - 1, 0);
    if (count < 0) {
        cerr << "send: " << strerror(errno) << endl;
        return false;
    }
    return true;
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

HttpResponse::HttpResponse(fstream* file, http_method_t method, http_version_t version, http_status_t status) {
    this->file = file;
    this->method = method;
    this->version = version;
    this->status = status;
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

int main() {
    HttpServer server;
    while (server.IsGood()) {
        server.Run();
    }
    return 0;
}
