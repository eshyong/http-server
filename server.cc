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
using std::make_pair;
using std::pair;
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

    // Listen for connections
    error = listen(listening, BACKLOG);
    if (error < 0) {
        perror("listen");
        close(listening);
        exit(EXIT_FAILURE);
    }
}

SocketServer::~SocketServer() {
    // Close listening socket 
    close(listening);
}

std::pair<int, string> SocketServer::Connect() {
    // Accept any incoming connections
    socklen_t length = sizeof(clientaddr);
    int error = 0;
    int connection = accept(listening, NULL, NULL);
    string peer = "";

    if (connection > 0) {
        // Get connecting client's network info
        error = getpeername(connection, (struct sockaddr *)&clientaddr, &length);
        if (error < 0) {
            perror("getpeername");
            memset(peername, (char) NULL, sizeof(peername));
        } else {
            inet_ntop(AF_INET, &(clientaddr.sin_addr), peername, length);
            peer = string(peername);
        }
    }
    return make_pair(connection, peer);
}

bool SocketServer::Receive(bool verbose, pair<int, string> client) {
    int connection = client.first;
    string peer = client.second;

    // Receive bytes from client connection
    int count = recv(connection, recvbuf, BUFFER_LENGTH, 0);
    if (count <= 0) {
        return false;
    }
    
    // Logging and NULL termination
    if (verbose) {
        cout << "Received " << count << " bytes from " << peer << ":\n";
        cout << recvbuf << endl;
    }
    recvbuf[count] = (char) NULL;
    return true;
}

bool SocketServer::SendResponse(string buffer, int connection) {
    // Send buffer over socket, no need for NULL termination
    int count = send(connection, buffer.c_str(), buffer.length() - 1, 0);
    if (count < 0) {
        perror("send");
        return false;
    }
    return true;
}

bool SocketServer::Close(int connection) {
    // Close connection specified by file descriptor
    int error = close(connection);
    if (error < 0) {
        perror("close");
        return false;
    }
    return true;
}

////////////////////////////////////////////////
//              HttpServer                    //
////////////////////////////////////////////////
HttpServer::HttpServer() { elapsedtime = 0.0; }

HttpServer::~HttpServer() {
    // Clean up allocated memory from cache
    while (!cache.empty()) {
        HttpRequest* request = cache.back().first;
        delete request;
        cache.pop_back();
    }
}

void HttpServer::Run(server_type type, bool verbose) {
    // Add signal handlers
    signal(SIGINT, handleSigint);
    signal(SIGCHLD, handleSigchld);

    // Run with flag options
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
    pair<int, string> client;
    int connection;
    if (verbose) {
        cout << "Server starting...\n\n";
    }

    // Event loop
    while (running) {

        // Wait for a connection
        client = server.Connect();
        connection = client.first;
        if (connection > 0) {
            // Fork a new server process to handle client connection
            pid = fork();
            if (pid < 0) {
                // Error 
                perror("fork");
                continue;
            } else if (pid == 0) {
                // Child process
                DispatchRequestToChild(verbose, client);
            } else {
                // Parent process
                server.Close(connection);
            }
        }
        // Sleep if not connected
        usleep(SLEEP_MSEC);
    }
    if (verbose) {
        cout << "Server shutting down...\n";
    }
}

void HttpServer::DispatchRequestToChild(bool verbose, pair<int, string> client) {
    HttpRequest request;
    string response;
    time_t begin;
    time_t end;
    int connection = client.first;

    // End process when elapsed time exceeds time out interval
    time(&begin);
    time(&end);
    elapsedtime = difftime(end, begin);

    do {
        if (server.Receive(verbose, client)) { 
            // Handle request and send response
            ParseRequest(request, verbose, server.get_buffer());
            response = HandleRequest(request, verbose);
            server.SendResponse(response, connection);
        }
        // Calculate elapsed time
        time(&end);
        elapsedtime = difftime(end, begin);
    } while (elapsedtime < TIME_OUT);

    // Close connection and exit
    server.Close(connection);
    exit(EXIT_SUCCESS);
}

void HttpServer::RunMultiThreaded(bool verbose) {
    vector<pthread_t> threadlist;
    pthread_t newthread;
    mthreaded_request_args args;
    pair<int, string> client;
    int connection;
    int error;
    void* status;

    // Initialize thread attributes and mutex
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_mutex_init(&cachemutex, NULL);
    if (verbose) {
        cout << "Server starting...\n\n";
    }

    // Event loop waits for any new connections
    while (running) {
        client = server.Connect();
        connection = client.first;
        if (connection > 0) {
            // Create a new thread and dispatch thread
            args.verbose = verbose;
            args.client = client;
            args.ptr = this;

            // Add new thread to threadlist
            error = pthread_create(&newthread, &attr, HttpServer::CallDispatchRequestToThread, &args);
            if (error < 0) {
                perror("pthread_create");
            }
            threadlist.push_back(newthread);
        }
        // Sleep if no connections
        usleep(SLEEP_MSEC);
    }

    if (verbose) {
        cout << "Server shutting down...\n";
    }

    // Join all finished threads
    while (!threadlist.empty()) {
        error = pthread_join(threadlist.back(), NULL);
        if (error < 0) {
            perror("pthread_join");
        }
        threadlist.pop_back();
    }

    // Clean up mutex and attributes
    pthread_attr_destroy(&attr);
    pthread_mutex_destroy(&cachemutex);

    // Allow main thread to exit while waiting for any threads to finish
    pthread_exit(NULL);
}

void* HttpServer::DispatchRequestToThread(bool verbose, pair<int, string> client) {
    HttpRequest *request = new HttpRequest;
    string response;
    time_t begin;
    time_t end;
    int connection = client.first;
    bool cached = false;
        
    // End process when elapsed time exceeds time out interval
    time(&begin);
    time(&end);
    elapsedtime = difftime(end, begin);

    do {
        if (server.Receive(verbose, client)) { 
            // Handle request and send response
            ParseRequest(*request, verbose, server.get_buffer());

            // Lock mutex before call
            response = HandleRequestThreaded(*request, verbose, cached);
            server.SendResponse(response, connection);
        }
        // Calculate elapsed time
        time(&end);
        elapsedtime = difftime(end, begin);
    } while (elapsedtime < TIME_OUT);

    // Add item to cache
    if (!cached) {
        // Lock cache while updating
        pthread_mutex_lock(&cachemutex);
        cache.push_back(make_pair(request, response));
        pthread_mutex_unlock(&cachemutex);
    }

    // Close connection and exit
    server.Close(connection);
    pthread_exit(NULL);
}

void* HttpServer::CallDispatchRequestToThread(void* args) {
    // Unpack arguments and return a function pointer
    mthreaded_request_args* arguments = (mthreaded_request_args*) args;
    bool verbose = arguments->verbose;
    pair<int, string> client = arguments->client;
    void* ptr = arguments->ptr;
    return ((HttpServer*) ptr)->DispatchRequestToThread(verbose, client); 
}

void HttpServer::RunEvented(bool verbose) {
    cout << "Not implemented yet\n";
    exit(EXIT_SUCCESS);
}

void HttpServer::ParseRequest(HttpRequest& request, bool verbose, const char* recvbuf) {
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
    if (verbose && method == INVALID_METHOD) {
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
    if (verbose && path.length() > URI_MAX_LENGTH) {
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
    if (verbose && version == INVALID_VERSION) {
        cout << "Invalid HTTP version.\n";
    }

    // Fill request struct
    request.Initialize(method, version, path, query, type);
}

string HttpServer::HandleRequestThreaded(HttpRequest& request, bool verbose, bool& cached) {
    HttpRequest* cachedreq;
    string response = "";

    // Lock cache and try to read
    if (verbose) {
        cout << "Searching cache...\n";
    }
    for (auto item = cache.begin(); item != cache.end(); item++) {
        // Check cache for saved response
        cachedreq = (*item).first;
        response = (*item).second;
        if (request.Equals(*cachedreq)) {
            if (verbose) {
                cout << "Serving from cache\n\n";
            }
            cached = true;
        }
    }
    // Release lock
    pthread_mutex_unlock(&cachemutex);

    if (!cached) {
        // Not found, create a new response
        if (verbose) {
            cout << "Not found in cache.\n";
        }
        response = HandleRequest(request, verbose);
    }
    if (verbose) {
        cout << endl << "Response: " << response << endl << endl;
    }

    return response;
}

string HttpServer::HandleRequest(HttpRequest& request, bool verbose) {
    bool toolong = request.get_flag();
    http_status_t status = OK;
    http_method_t method = request.get_method();
    http_version_t version = request.get_version();
    string path = request.get_path();
    string type = request.get_content_type();
    string response;
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
    response = CreateResponse(request, status);
    if (verbose) {
        cout << endl << "Response: " << response << endl << endl;
    }
    return response;
}

string HttpServer::CreateResponse(HttpRequest request, http_status_t status) {
    // Create the string representation 
    fstream file;
    string body = "";
    string path = request.get_path();
    string type = request.get_content_type();
    string query = request.get_query();
    string command = "";
    string response = "";
    http_method_t method = request.get_method();
    http_version_t version = request.get_version();
    pid_t pid;
    int begin = 0;
    int index = 0;
    int querylen = query.length();
    int contentlen;
    int error;
    char c;

    // Check method type
    if (method == GET && status == OK) {
        if (strcmp(type.c_str(), "application/php") == 0) {
            // WARNING: executes bash code with user input
            // Extremely unsafe and will be deprecated
            // I am considering embedding PHP or adding a CGI protocol
            /*
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
            system(command.c_str()); */
            status = NOT_IMPLEMENTED;
            type = "text/html";
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
    response += versions[version];
    response += SPACE;
    response += statuses[status];
    response += CRLF;

    // For GET only
    if (status == OK && method == GET) {
        response += "Accept-Ranges: bytes";
        response += CRLF;
        response += "Content-Type: ";
        response += type;
        response += CRLF;
        response += "Content-Length: ";
        response += std::to_string(contentlen);
        response += CRLF;
    }
    response += CRLF;
    response += body;
    return response;
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
//              HttpRequest                   //
////////////////////////////////////////////////
HttpRequest::HttpRequest(http_method_t method, http_version_t version, string path, string query, string type) {
    Initialize(method, version, path, query, type);
}

HttpRequest::HttpRequest() {
    Initialize(INVALID_METHOD, INVALID_VERSION, "", "", "");
}

void HttpRequest::Initialize(http_method_t method, http_version_t version, string path, string query, string type) {
    // Call parent initialization
    toolong = false;
    this->method = method;
    this->version = version;
    this->path = path;
    this->query = query;
    this->type = type;
}

void HttpRequest::ParseHeaders(const char* buffer, int index) {
    int i = index;
    int length = strlen(buffer);
    string name = "";
    string value = "";
    const Header* header;

    while (i <= length) {
        // Get name of header
        while (i <= length && buffer[i] != ':') {
            name += buffer[i];
            i++;
        }
        // Increment twice for : 
        i += 2;

        // Get value of header
        while (i <= length && buffer[i] != '\r') {
            value += buffer[i];
            i++;
        }
        // Increment twice for \r\n
        i += 2;
        
        // Add new header struct
        if (!isspace(name.c_str()[0])) {
            header = new Header(name, value);
            headers.push_back(header);
        }
        
        // Reset fields
        name = "";
        value = "";
    }
}

void HttpRequest::Reset() {
    path = "";
    type = "";
    method = INVALID_METHOD;
    version = INVALID_VERSION;
    while (!headers.empty()) {
        delete headers.back();
        headers.pop_back();
    }
}

HttpRequest::~HttpRequest() {}

// End of file
