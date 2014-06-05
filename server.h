#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>

#define BACKLOG       10
#define BODY_LENGTH   1023
#define BUFFER_LENGTH 4095
#define PORT          8000

#define CRLF      "\r\n"
#define SPACE     " "
#define DIRECTORY "test"

using std::fstream;

enum http_method_t {
    INVALID_METHOD = -1, GET, POST, PUT, DELETE
};

enum http_version_t {
    INVALID_VERSION = -1, ONE, TWO, THREE
};

enum http_status_t {
    CONTINUE = 0, OK, NOT_FOUND, NOT_IMPLEMENTED,
};

const char* versions[] = {
    "HTTP/1.0", "HTTP/1.1", "HTTP/2.0",
};

const char* statuses[] = {
    "100 Continue", "200 OK", "404 Not Found", "501 Not Implemented",
};

struct HttpRequest {
private:
    char path[BUFFER_LENGTH + 1];
    http_method_t method;
    http_version_t version;
public:
    HttpRequest(char* path, http_method_t method, http_version_t version);
    HttpRequest();
    ~HttpRequest();

    // Getters
    const char* get_path() { return path; }
    http_method_t get_method() { return method; }
    http_version_t get_version() { return version; }

    // Setters
    void set_path(const char* buffer) { 
        strncpy(path, buffer, BUFFER_LENGTH); 
        path[BUFFER_LENGTH] = (char) NULL;
    }
    void set_method(http_method_t method) { this->method = method; }
    void set_version(http_version_t version) { this->version = version; } 
    void Reset();
};

struct HttpResponse {
private:
    const fstream* file;
    int contentlen;
    http_method_t method;
    http_version_t version;
    http_status_t status;
public:
    HttpResponse(fstream* file, http_method_t method, http_version_t version, http_status_t status);
    HttpResponse();
    ~HttpResponse();

    // Getters
    int get_content_length() { return contentlen; }
    http_method_t get_method() { return method; }
    http_version_t get_version() { return version; }
    http_status_t get_status() { return status; }

    // Setters
    void set_method(http_method_t method) { this->method = method; }
    void set_version(http_version_t version) { this->version = version; } 
    void set_status(http_status_t status) { this->status = status; }
    void Reset();
};

class HttpServer {
private:
    // Addresses for the server and the client
    struct sockaddr_in server;
    struct sockaddr_in client;
    socklen_t address_len;
    
    // Socket file descriptors
    int listening;
    int connection;
    char sendbuf[BUFFER_LENGTH + 1];
    char recvbuf[BUFFER_LENGTH + 1];
    bool good;

public:
    HttpServer();
    ~HttpServer();
    void Run();
    bool Connect();
    bool GetRequest();
    bool ParseRequest(HttpRequest& request);
    bool HandleRequest(HttpRequest& request);
    bool SendResponse(HttpResponse& response, fstream& file);
    http_method_t GetMethod(const char *string);
    http_version_t GetVersion(const char *string);
    bool IsGood();
};

// End of header
