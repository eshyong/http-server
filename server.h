#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>

#define PORT          80
#define BACKLOG       10
#define BUFFER_LENGTH 4095

#define CRLF      "\r\n"
#define SPACE     " "
#define DIRECTORY "test"

using std::fstream;

enum http_method_t {
    INVALID_METHOD = 0, GET, POST, PUT, DELETE
};

enum http_version_t {
    INVALID_VERSION = 0, ONE, TWO, THREE
};

enum http_status_t {
    CONTINUE = 0, OK, NOT_FOUND,
};

const char* versions[] = {
    "HTTP/1.0", "HTTP/1.1", "HTTP/2.0",
};

const char* statuses[] = {
    "100 Continue", "200 OK", "404 Not Found",
};

struct HttpRequest {
private:
    const char* path;
    http_method_t method;
    http_version_t version;
public:
    HttpRequest(char* path, http_method_t method, http_version_t version);
    HttpRequest();
    ~HttpRequest();

    // Getters
    const char* get_path() {
        return path;
    }
    http_method_t get_method() {
        return method;
    }
    http_version_t get_version() {
        return version;
    }

    // Setters
    void set_path(const char* newpath) {
        path = newpath;
    }
    void set_method(http_method_t newmethod) {
        method = newmethod;
    }
    void set_version(http_version_t newversion) {
        version = newversion;
    }
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
    bool SendResponse(http_version_t version, http_method_t method, http_status_t status, fstream& file);
    http_method_t GetMethod(const char *string);
    http_version_t GetVersion(const char *string);
    bool IsGood();
};

// End of header
