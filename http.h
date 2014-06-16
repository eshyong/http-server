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

#define BACKLOG        128
#define BODY_LENGTH    8191
#define BUFFER_LENGTH  8191
#define URI_MAX_LENGTH 4095
#define PORT           8000
#define SLEEP_MSEC     1000
#define TIME_OUT       1.0

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

class Header {
public:
    // Request headers, e.g. name="Content-Length", value="10"
    string name;
    string value;
    
    // Constructor 
    Header(string name, string value) {
        this->name = name;
        this->value = value;
    }
};

class HttpRequest {
private:
    vector<const Header*> headers;
    http_method_t method;
    http_version_t version;
    string copy;
    string path;
    string query;
    string type;
    bool toolong;
public:
    HttpRequest(http_method_t method, http_version_t version, string copy, string path, string query, string type);
    HttpRequest();
    ~HttpRequest();

    // Equality operator
    bool Equals(HttpRequest& other) {
        return (method == other.get_method() && version == other.get_version() && path.compare(other.get_path()) == 0);
    }

    // Initialization and reset method
    void Initialize(http_method_t method, http_version_t version, string copy, string path, string query, string type);
    void Reset();

    // Header parsing 
    void ParseHeaders(const char* buffer, int index);

    // Getters
    vector<const Header*> get_headers() { return headers; }
    http_method_t get_method() { return method; }
    http_version_t get_version() { return version; }
    string get_copy() { return copy; }
    string get_path() { return path; }
    string get_query() { return query; }
    string get_content_type() { return type; }
    bool get_flag() { return toolong; }

    // Setters
    void set_method(http_method_t method) { this->method = method; }
    void set_version(http_version_t version) { this->version = version; }
    void set_copy(string copy) { this->copy = copy; }
    void set_path(string path) { this->path = path; }
    void set_query(string query) { this->query = query; }
    void set_flag(bool value) { toolong = value; }
};

#endif

// End of header
