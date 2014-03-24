#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PORT    5000
#define BACKLOG 10

enum method {
    UNDEFINED = 0, GET, POST, PUT, DELETE
};

struct server {
    struct sockaddr_in servaddr;
    struct sockaddr_in connaddr;
    socklen_t addrlen;
    int listenfd;
    int connfd;
    char sendbuf[1024];
    char recvbuf[1024];
};

struct http_request {
    uint8_t version;
    enum method mthd;
    char method_name[7];
    char uri[1024];
};

struct http_response {
    int version;
    int status;
};

enum method get_method(const char *const string) {
    if (strcmp(string, "GET") == 0) {
        return GET;
    } else if (strcmp(string, "POST") == 0) {
        return POST;
    } else if (strcmp(string, "PUT") == 0) {
        return PUT;
    } else if (strcmp(string, "DELETE") == 0) {
        return DELETE;
    } else {
        return UNDEFINED;
    }
}

uint8_t get_version(const char *const string) {
    if (strcmp(string, "HTTP/1.0") == 0) {
        return 10;
    } else if (strcmp(string, "HTTP/1.1") == 0) {
        return 11;
    } else if (strcmp(string, "HTTP/2.0") == 0) {
        return 20;
    } else {
        printf("Invalid version number\n");
        return 0;
    }
}

void initialize_server(struct server *s) {
    /* Zero initialize everything */
    memset(s, 0, sizeof(struct server));

    /* Address length must be initialized to size of connection address */
    s->addrlen = sizeof(s->connaddr);
    
    /* Create a socket and add hints */
    s->listenfd = socket(AF_INET, SOCK_STREAM, 0);
    s->servaddr.sin_family = AF_INET;
    s->servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    s->servaddr.sin_port = htons(PORT);

    /* Bind socket */
    bind(s->listenfd, (struct sockaddr *)&s->servaddr, sizeof(s->servaddr));
    listen(s->listenfd, BACKLOG);
    
    /* HTTP Response OK */
    strcpy(s->sendbuf, "HTTP/1.1 200 OK\r\n");
    
    printf("Server starting...\n");
}

struct http_request *parse_request(char const *const buffer) {
    struct http_request *request = (struct http_request *)malloc(sizeof(struct http_request));
    if (request == NULL) {
        fprintf(stderr, "Error: parse_request: bad malloc\n");
        return NULL;
    }
    memset(request, 0, sizeof(struct http_request));

    char word[1024];
    int index = 0;
    int length = strlen(buffer);

    for (int i = 0; i < length; i++) {
        if (isspace(buffer[i])) {
            word[index] = '\0';
            if (request->mthd == UNDEFINED) {
                request->mthd = get_method(word);
                strcpy(request->method_name, word);
            } else if (request->uri[0] == '\0') {
                strcpy(request->uri, word);
            } else if (request->version == 0) {
                request->version = get_version(word);
            } else {
                break;
            }
            index = 0;
        } else {
            word[index] = buffer[i];
            index++;
        }
    }
    return request;
}

void run() {
    struct server serv;
    initialize_server(&serv);

    struct http_request *request = NULL;

    char addr[INET_ADDRSTRLEN];
    int bytes = 0;

    while (1) {
        serv.connfd = accept(serv.listenfd, (struct sockaddr *)&serv.connaddr, &serv.addrlen);
        if (serv.connfd < 0) {
            perror("accept");
            goto cleanup;
        } else {
            inet_ntop(AF_INET, &serv.connaddr.sin_addr, addr, INET_ADDRSTRLEN);
            printf("Accepting connection from %s on port %d\n", addr, ntohs(serv.connaddr.sin_port));
        }
       
        bytes = recv(serv.connfd, serv.recvbuf, sizeof(serv.recvbuf), 0);
        if (bytes < 0) {
            perror("read");
            goto cleanup;
        }
        serv.recvbuf[bytes] = '\0';
        printf("Request: %s\n", serv.recvbuf);

        request = parse_request(serv.recvbuf);
        printf("Method: %s %d\n", request->method_name, request->mthd);
        printf("Uri: %s\n", request->uri);
        printf("Version: %d\n", request->version);

        bytes = write(serv.connfd, serv.sendbuf, strlen(serv.sendbuf));
        if (bytes < 0) {
            perror("write");
            goto cleanup;
        }
    
        cleanup:
            close(serv.connfd);
        
        sleep(1);
    }
}

int main(int argc, char *argv[]) {
    run();
    return 0;
}
