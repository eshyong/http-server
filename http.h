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
#define SLEEP_MSEC    5000
#define TIME_OUT      5.0

using std::fstream;
using std::string;

enum http_method_t {
    INVALID_METHOD = -1, GET, POST, PUT, DELETE
};

enum http_version_t {
    INVALID_VERSION = -1, ONE, TWO, THREE
};

enum http_status_t {
    CONTINUE = 0, OK, BAD_REQUEST, NOT_FOUND, REQUEST_ENTITY_TOO_LARGE, NOT_IMPLEMENTED,
};

const string versions[] = {
    "HTTP/1.0", "HTTP/1.1", "HTTP/2.0",
};

const string statuses[] = {
    "100 Continue", "200 OK", "400 Bad Request", "404 Not Found", "413 Request Entity Too Large", "501 Not Implemented",
};

class HeaderOptions {

};

class HttpRequest {
private:
    HeaderOptions* options;
    string path;
    http_method_t method;
    http_version_t version;
    bool toolong;
public:
    HttpRequest(string path, http_method_t method, http_version_t version, HeaderOptions* options);
    HttpRequest();
    ~HttpRequest();

    // Initialization method
    void Initialize(string path, http_method_t method, http_version_t version, HeaderOptions* options);

    // Getters
    string get_path() { return path; }
    http_method_t get_method() { return method; }
    http_version_t get_version() { return version; }
    bool get_flag() { return toolong; }

    // Setters
    void set_path(string path) { this->path = path; }
    void set_method(http_method_t method) { this->method = method; }
    void set_version(http_version_t version) { this->version = version; } 
    void set_flag(bool value) { toolong = value; }
    void Reset();
};

class HttpResponse {
private:
    HeaderOptions* options;
    string stringrep;
    int contentlen;
    http_method_t method;
    http_version_t version;
    http_status_t status;
public:
    // Constructors and destructors
    HttpResponse(fstream* file, http_method_t method, http_version_t version, http_status_t status, HeaderOptions* options);
    HttpResponse();
    ~HttpResponse();

    // Initialization method
    void Initialize(fstream* file, http_method_t method, http_version_t version, http_status_t status, HeaderOptions* options);

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
