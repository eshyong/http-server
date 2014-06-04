#include "server.h"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>

using std::cerr;
using std::cout;
using std::endl;
using std::fstream;

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
    Connect();

    // Receive bytes from client
    GetRequest();

    // Parse request
    ParseRequest(request);

    // Send response
    HandleRequest(request);

    // Close the connection
    close(connection);
}

bool HttpServer::Connect() {
    // Accept any incoming connections
    connection = accept(listening, NULL, NULL);
    if (connection < 0) {
        cerr << "accept" << strerror(errno);
        return false;
    }
    return true;
}

bool HttpServer::GetRequest() {
    int count = recv(connection, recvbuf, BUFFER_LENGTH, 0);
    if (count < 0) {
        cerr << "recv" << strerror(errno);
        return false;
    }
    recvbuf[BUFFER_LENGTH] = (char) NULL;
    cout << "Message: " << recvbuf;
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
        cout << path << endl;
        file.open(path, fstream::in);
        if (!file.is_open()) {
            cerr << "open" << strerror(errno);
            status = NOT_FOUND;
        }
    } else if (method == POST) {
        cout << "Not implemented yet\n"; 
        return true;
    } else {
        cout << "Not implemented yet\n";
        return false;
    }

    SendResponse(version, method, status, file);
    return true;
}

bool HttpServer::SendResponse(http_version_t version, http_method_t method, http_status_t status, fstream& file) {
    // Send version string
    send(connection, versions[version], strlen(versions[version]), 0);
    send(connection, SPACE, strlen(SPACE), 0);

    // Send status code
    send(connection, statuses[status], strlen(statuses[status]), 0);
    send(connection, CRLF, strlen(CRLF), 0);
    
    // Check method type
    if (method == GET && status == OK) {
        // Send file over socket
        while (file.good()) {
            char c = (char) file.get();
            send(connection, &c, 1, 0);
        }
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
    this->path = path;
    this->method = method;
    this->version = version;
}

HttpRequest::HttpRequest() {
    path = NULL;
    method = INVALID_METHOD;
    version = INVALID_VERSION;
}

HttpRequest::~HttpRequest() {
    delete path;
}

int main() {
    HttpServer server;
    while (server.IsGood()) {
        server.Run();
    }
    return 0;
}
