#ifndef HTTP_H
#define HTTP_H

#include <iostream>

#define CRLF      "\r\n"
#define SPACE     " "
#define DIRECTORY "test"

#define BACKLOG       10
#define BODY_LENGTH   1023
#define BUFFER_LENGTH 4095
#define PORT          8000

using std::fstream;
using std::string;

enum http_method_t {
    INVALID_METHOD = -1, GET, POST, PUT, DELETE
};

enum http_version_t {
    INVALID_VERSION = -1, ONE, TWO, THREE
};

enum http_status_t {
    CONTINUE = 0, OK, NOT_FOUND, NOT_IMPLEMENTED,
};

const string versions[] = {
    "HTTP/1.0", "HTTP/1.1", "HTTP/2.0",
};

const string statuses[] = {
    "100 Continue", "200 OK", "404 Not Found", "501 Not Implemented",
};

class HttpRequest {
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

class HttpResponse {
private:
    fstream* file;
    int contentlen;
    http_method_t method;
    http_version_t version;
    http_status_t status;
    string stringrep;
public:
    HttpResponse(fstream* file, http_method_t method, http_version_t version, http_status_t status);
    HttpResponse();
    ~HttpResponse();
    
    // Initialization method
    void Initialize();

    // Getters
    int get_content_length() { return contentlen; }
    http_method_t get_method() { return method; }
    http_version_t get_version() { return version; }
    http_status_t get_status() { return status; }

    // Setters
    void set_method(http_method_t method) { this->method = method; }
    void set_version(http_version_t version) { this->version = version; } 
    void set_status(http_status_t status) { this->status = status; }

    // String representation
    string to_string() { return stringrep; }
    void Reset();
};

#endif
// End of header
