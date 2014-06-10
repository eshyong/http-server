#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <iostream>
#include <sstream>
#include <fcntl.h>
#include "http.h"
#include "server.h"

using std::cerr;
using std::cout;
using std::endl;
using std::fstream;
using std::streambuf;
using std::string;
using std::stringstream;
using std::system;
using std::to_string;
using std::vector;

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
//              Misc Helpers                  //
////////////////////////////////////////////////
char hexToAscii(string hex) {
    int ascii = 0;
    int num;
    if (isalpha(hex[0])) {
        num = 16 * (hex[0] - 55);
    } else {
        num = 16 * (hex[0] - 48);
    }
    ascii += num;

    if (isalpha(hex[1])) {
        num = hex[1] - 55;
    } else {
        num = hex[1] - 48;
    }
    ascii += num;
    return (char) ascii;
}

void getArgsFromQuery(string& query, vector<const char*>& args) {
    // Queries come in the following form:
    // query = "name1=value1&name2=value2 ... nameN=valueN"
    // We will parse value1 and value2, and append it to args
    int index = 0;
    int begin = 0;
    int length = query.length();
    bool equals = false;
    string* arg;

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

bool SocketServer::Receive(bool verbose) {
    int count = recv(connection, recvbuf, BUFFER_LENGTH, 0);
    if (count <= 0) {
        return false;
    }
    if (verbose) {
        cout << "Received " << count << " bytes from " << peername << ":\n";
        cout << recvbuf << endl;
    }
    recvbuf[count] = (char) NULL;
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

void HttpServer::Run(server_type type, bool verbose) {
    // Add signal handlers
    signal(SIGINT, handleSigint);
    signal(SIGCHLD, handleSigchld);

    if (type == MPROCESS) {
        RunMultiProcessed(verbose);
    } else if (type == MTHREADED) {
        RunMultiThreaded(verbose);
    } else if (type == EVENTED) {
        RunEvented(verbose);
    }
}

void HttpServer::RunMultiProcessed(bool verbose) {
    pid_t pid;
    time_t begin;
    time_t end;
    int status;
    if (verbose) {
        cout << "Server starting...\n\n";
    }

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
                    if (server.Receive(verbose)) { 
                        // Handle request and send response
                        ParseRequest(request, server.get_buffer());
                        HandleRequest(request, verbose);
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
    if (verbose) {
        cout << "Server shutting down...\n";
    }
}

void HttpServer::RunMultiThreaded(bool verbose) {
    cout << "Not implemented yet\n";
    exit(EXIT_SUCCESS);
}

void HttpServer::RunEvented(bool verbose) {
    cout << "Not implemented yet\n";
    exit(EXIT_SUCCESS);
}

void HttpServer::ParseRequest(HttpRequest& request, const char* recvbuf) {
    int i = 0;
    http_method_t method;
    http_version_t version;
    string buffer = "";
    string path = "";
    string query = "";
    string type = "";
    string uri = "";

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

    // Get URI
    while (i <= BUFFER_LENGTH && !isspace(recvbuf[i])) {
        uri += recvbuf[i];
        i++;
    }
    path = DIRECTORY;

    // Parse URI, sanitizing output
    ParseUri(uri, path, query, type);
    i++;
    if (path.length() > URI_MAX_LENGTH) {
        cout << "Request URI too long.\n";
    }

    // Get version number
    buffer = "";
    while (i <= BUFFER_LENGTH && !isspace(recvbuf[i])) {
        buffer += recvbuf[i];
        i++;
    }

    // Skip to next line
    while (isspace(recvbuf[i])) {
        i++;
    }

    // Parse version number
    version = GetVersion(buffer);
    if (version == INVALID_VERSION) {
        cout << "Invalid HTTP version.\n";
    }

    // Fill request struct
    request.Initialize(method, version, path, query, type);
    request.ParseOptions(recvbuf, i);
}

bool HttpServer::HandleRequest(HttpRequest& request, bool verbose) {
    HttpResponse response;
    bool toolong = request.get_flag();
    http_status_t status = OK;
    http_method_t method = request.get_method();
    http_version_t version = request.get_version();
    string path = request.get_path();
    string type = request.get_content_type();
    fstream file;
    
    // HTTP flow diagram starts here
    if (toolong) {
        // Request entity too large
        status = REQUEST_ENTITY_TOO_LARGE;
    } else if (version == INVALID_VERSION) {
        // Invalid request
        status = BAD_REQUEST;
    } else if (method == INVALID_METHOD) {
        // Unrecognized method
        status = NOT_IMPLEMENTED;
    } else if (path.length() > URI_MAX_LENGTH) {
        // Request URI too large
        status = REQUEST_URI_TOO_LARGE;
    } else {
        // Handle each HTTP method
        if (method == GET) {
            // Get URI resource by opening file
            file.open(path, fstream::in);
            if (!file.is_open()) {
                // Not found
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
    response.CreateResponseString(request, status);
    if (verbose) {
        cout << endl << "Response: " << response.to_string() << endl << endl;
    }
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
        return ONE_POINT_ZERO;
    } else if (version.compare("HTTP/1.1") == 0) {
        return ONE_POINT_ONE;
    } else if (version.compare("HTTP/2.0") == 0) {
        return TWO_POINT_ZERO;
    }
    return INVALID_VERSION;
}

string HttpServer::GetMimeType(string extension) {
    if (extension.compare("txt") == 0) {
        return "text/plain";
    } else if (extension.compare("html") == 0) {
        return "text/html";
    } else if (extension.compare("js") == 0) {
        return "application/js";
    } else if (extension.compare("php") == 0) {
        return "application/php";
    } else if (extension.compare("css") == 0) {
        return "text/css";
    } else if (extension.compare("png") == 0) {
        return "image/png";
    } else if (extension.compare("jpg") == 0) {
        return "image/jpeg";
    } else if (extension.compare("gif") == 0) {
        return "image/gif";
    } else {
        return "unknown";
    }
}

void HttpServer::ParseUri(string& uri, string& path, string& query, string& type) {
    int relpath = uri.find(PREVDIR);
    int length;
    int i = 0;
    string hex = "";
    string extension = "";
    char c;

    // Sanitize string, checking for weird relative paths such as "/.."
    // "/." is ok
    while (relpath != string::npos) {
        uri.erase(relpath, strlen(PREVDIR));
        relpath = uri.find(PREVDIR);
    }
    length = uri.length();
    
    // Get path right before query, while sanitizing unsafe ascii characters
    // "%HEX" is interpreted as the character with ascii value HEX
    // "+" is interpreted as a space character
    while (i <= length && uri[i] != '?') {
        if (uri[i] == '%') {
            // Unsafe ascii conversion
            i++;
            hex += uri[i];
            i++;
            hex += uri[i];
            c = hexToAscii(hex);
        } else if (uri[i] == '+') {
            // Space
            c = ' ';
        } else {
            // Safe ascii
            c = uri[i];   
        }
        path += c;
        i++;
    }

    // Skip past '?'
    i++;

    // Get query
    while (i <= length) {
        query += uri[i];
        i++;
    }

    // Get type from path
    i = path.length();
    while (i > 0 && path[i] != '.') {
        i--;
    }
    i++;
    while (i <= path.length() && !isspace(path[i]) && path[i] != (char) NULL) {
        extension += path[i];
        i++;
    }

    // Interpret MIME type using extension string
    type = GetMimeType(extension);
}

////////////////////////////////////////////////
//              HttpMessage                   //
////////////////////////////////////////////////

HttpMessage::HttpMessage(http_method_t method, http_version_t version) {
    Initialize(method, version);
}

HttpMessage::HttpMessage() {
    Initialize(INVALID_METHOD, INVALID_VERSION);
}

HttpMessage::~HttpMessage() {
    while (!options.empty()) {
        delete options.back();
        options.pop_back();
    }
}

void HttpMessage::Initialize(http_method_t method, http_version_t version) {
    this->method = method;
    this->version = version;
}

void HttpMessage::ParseOptions(const char* buffer, int index) {
    int i = index;
    int length = strlen(buffer);
    string name = "";
    string value = "";
    const Option* option;

    while (i <= length) {
        // Get name of header option
        while (i <= length && buffer[i] != ':') {
            name += buffer[i];
            i++;
        }
        // Increment twice for : 
        i += 2;

        // Get value of header option
        while (i <= length && buffer[i] != '\r') {
            value += buffer[i];
            i++;
        }
        // Increment twice for \r\n
        i += 2;
        
        // Add new option
        if (!isspace(name.c_str()[0])) {
            option = new Option(name, value);
            options.push_back(option);
        }
        
        // Reset fields
        name = "";
        value = "";
    }
}

////////////////////////////////////////////////
//              HttpRequest                   //
////////////////////////////////////////////////
HttpRequest::HttpRequest(http_method_t method, http_version_t version, string path, string query, string type):
             HttpMessage(method, version) {
    Initialize(method, version, path, query, type);
}

HttpRequest::HttpRequest(): HttpMessage() {
    Initialize(INVALID_METHOD, INVALID_VERSION, "", "", "");
}

void HttpRequest::Initialize(http_method_t method, http_version_t version, string path, string query, string type) {
    // Call parent initialization
    HttpMessage::Initialize(method, version);

    toolong = false;
    this->path = path;
    this->query = query;
    this->type = type;
}

void HttpRequest::Reset() {
    path = "";
    type = "";
    method = INVALID_METHOD;
    version = INVALID_VERSION;
    while (!options.empty()) {
        delete options.back();
        options.pop_back();
    }
}

HttpRequest::~HttpRequest() {}

////////////////////////////////////////////////
//              HttpResponse                  //
////////////////////////////////////////////////
HttpResponse::HttpResponse(HttpRequest request, fstream* file, http_status_t status):
              HttpMessage(request.get_method(), request.get_version()) {
}

HttpResponse::HttpResponse():
              HttpMessage(INVALID_METHOD, INVALID_VERSION) {
    status = OK;
    stringrep = "";
}

void HttpResponse::CreateResponseString(HttpRequest request, http_status_t status) {
    // Create the string representation 
    fstream file;
    streambuf* oldcout;
    stringstream buffer;
    vector<const char*> args;
    string body = "";
    string path = request.get_path();
    string type = request.get_content_type();
    string query = request.get_query();
    string command = "";
    http_method_t method = request.get_method();
    http_version_t version = request.get_version();
    pid_t pid;
    int begin = 0;
    int index = 0;
    int querylen = query.length();
    int contentlen;
    int error;
    char c;

    // Call parent initialization
    HttpMessage::Initialize(method, version);

    // Create a buffer and fill with information from response
    stringrep = "";

    // Check method type
    if (method == GET && status == OK) {
        if (strcmp(type.c_str(), "application/php") == 0) {
            // Run a PHP program while redirecting stdout to a stream
            // Keep a reference to old cout and redirect stdout
            //oldcout = cout.rdbuf();
            //cout.rdbuf(buffer.rdbuf());

            // Create a php command
            command = "php ";
            command += path;
            command += SPACE;
            
            // Parse query to get php arguments
            while (index < querylen) {
                // Start of valueN substr
                if (query[index] == '=') {
                    begin = index + 1;
                }

                // end of valueN substr
                if (query[index] == '&') {
                    command += query.substr(begin, index - begin);
                    command += SPACE;
                }
                index++;
            }
            command += query.substr(begin, string::npos);

            // Fork and execute php code
            system(command.c_str());

            //body += buffer.str();
            //cout.rdbuf(oldcout);
        } else {
            // Get all characters in file
            file.open(path, fstream::in);
            while (file.good() && index <= BODY_LENGTH) {
                body += (char) file.get();
                index++;
            }
        }
    }
    contentlen = body.length() == 0 ? 0 : body.length() - 1;

    // Append status line, options, and body to buffer
    stringrep += versions[version];
    stringrep += SPACE;
    stringrep += statuses[status];
    stringrep += CRLF;

    // For GET only
    if (status == OK && method == GET) {
        stringrep += "Accept-Ranges: bytes";
        stringrep += CRLF;
        stringrep += "Content-Type: ";
        stringrep += type;
        stringrep += CRLF;
        stringrep += "Content-Length: ";
        stringrep += std::to_string(contentlen);
        stringrep += CRLF;
    }

    stringrep += CRLF;
    stringrep += body;
}

void HttpResponse::Reset() {
    stringrep = "";
    method = INVALID_METHOD;
    version = INVALID_VERSION;
    while (!options.empty()) {
        delete options.back();
        options.pop_back();
    }
}

HttpResponse::~HttpResponse() {
    while (!options.empty()) {
        // Clean up options
        delete options.back();
        options.pop_back();
    }
}

// End of file
