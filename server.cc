#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PORT    5000
#define BACKLOG 10

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
    int version;
    int status;
};

struct http_response {
    int version;
    int status;
};

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
    bind(s->listenfd, (struct sockaddr *)&(s->servaddr), sizeof(s->servaddr));
    listen(s->listenfd, BACKLOG);
    
    /* HTTP Response OK */
    strcpy(s->sendbuf, "HTTP/1.1 200 OK\r\n");
    
    printf("Server starting...\n");
}

int parse_request(char *buffer) {
    return 0;
}

void run() {
    struct server serv;
    initialize_server(&serv);

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
        printf("%s\n", serv.recvbuf);
        
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
