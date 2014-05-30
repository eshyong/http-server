#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT          80
#define BACKLOG       10
#define BUFFER_LENGTH 1023

enum http_method_t {
    INVALID_METHOD = 0, GET, POST, PUT, DELETE
};

enum http_version_t {
    INVALID_VERSION = 0, ONE, TWO, THREE
};

struct HttpRequest {
public:
    const char* path;
    http_method_t method;
    http_version_t version;
public:
    HttpRequest(char* path, http_method_t method, http_version_t version);
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
    void Connect();
    void GetRequest();
    HttpRequest* ParseRequest();
    void SendResponse();
    http_method_t GetMethod(const char *string);
    http_version_t GetVersion(const char *string);
    bool IsGood();
};

// End of header
