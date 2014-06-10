#pragma once
#ifndef HTTP_H
#define HTTP_H

#include <iostream>
#include <vector>

#define CRLF      "\r\n"
#define SPACE     " "
#define DIRECTORY "test"
#define PREVDIR   "/.."
#define PLAINTEXT "text/html"

#define BODY_LENGTH    8191
#define BUFFER_LENGTH  8191
#define URI_MAX_LENGTH 4095
#define PORT           8000
#define SLEEP_MSEC     5000
#define TIME_OUT       2.5

using std::fstream;
using std::string;
using std::vector;

enum http_method_t {
    INVALID_METHOD = -1, GET, POST, PUT, DELETE,
};

enum http_version_t {
    INVALID_VERSION = -1, ONE_POINT_ZERO, ONE_POINT_ONE, TWO_POINT_ZERO,
};

enum http_status_t {
    CONTINUE = 0, OK, BAD_REQUEST, NOT_FOUND, REQUEST_ENTITY_TOO_LARGE, REQUEST_URI_TOO_LARGE, NOT_IMPLEMENTED,
};

const string versions[] = {
    "HTTP/1.0", "HTTP/1.1", "HTTP/2.0",
};

const string statuses[] = {
    "100 Continue", "200 OK", "400 Bad Request", "404 Not Found", "413 Request Entity Too Large", "414 Request URI Too Large", "501 Not Implemented",
};

class Option {
public:
    // Header options, e.g. name="Content-Length", value="10"
    string name;
    string value;
    
    // Constructor 
    Option(string name, string value) {
        this->name = name;
        this->value = value;
    }
};

class HttpMessage {
protected:
    vector<const Option*> options;
    http_method_t method;
    http_version_t version;
public:
    HttpMessage(http_method_t method, http_version_t version);
    HttpMessage();
    ~HttpMessage();
    void Initialize(http_method_t method, http_version_t version);
    void ParseOptions(const char* buffer, int index);
    virtual void Reset() = 0;
    
    // Getters
    vector<const Option*> get_options() { return options; }
    http_method_t get_method() { return method; }
    http_version_t get_version() { return version; }

    // Setters
    void set_method(http_method_t method) { this->method = method; }
    void set_version(http_version_t version) { this->version = version; }
};

class HttpRequest: public HttpMessage {
private:
    string path;
    string query;
    string type;
    bool toolong;
public:
    HttpRequest(http_method_t method, http_version_t version, string path, string query, string type);
    HttpRequest();
    ~HttpRequest();

    // Initialization method
    void Initialize(http_method_t method, http_version_t version, string path, string query, string type);
    void Reset();

    // Getters
    string get_path() { return path; }
    string get_query() { return query; }
    string get_content_type() { return type; }
    bool get_flag() { return toolong; }

    // Setters
    void set_path(string path) { this->path = path; }
    void set_query(string query) { this->query = query; }
    void set_flag(bool value) { toolong = value; }
};

class HttpResponse: public HttpMessage {
private:
    string stringrep;
    int contentlen;
    http_status_t status;
public:
    // Constructors and destructors
    HttpResponse(HttpRequest request, fstream* file, http_status_t status);
    HttpResponse();
    ~HttpResponse();

    // Initialization method
    void CreateResponseString(HttpRequest request, http_status_t status);
    void Reset();

    // Getters
    int get_content_length() { return contentlen; }
    http_status_t get_status() { return status; }

    // Setters
    void set_status(http_status_t status) { this->status = status; }

    // String representation
    string to_string() { return stringrep; }
};

#endif

// End of header
