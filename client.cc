#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
    int sockfd = 0;
    int n = 0;
    char recvbuf[1024];
    struct sockaddr_in serv_addr;

    if (argc != 3) {
        printf("\n Usage: %s <ip of server> \n", argv[0]);
        return 1;
    }

    memset(recvbuf, 0, sizeof(recvbuf));
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("\n Error: socket()");
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(5000);

    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0) {
        perror("\n Error: inet_pton()");
        return 1;
    }

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("\n Error: connect()");
        return 1;
    }

    do {
        send(sockfd, argv[2], strlen(argv[2]), 0);
        n = read(sockfd, recvbuf, sizeof(recvbuf) - 1);
        recvbuf[n] = (char)0;
        if (fputs(recvbuf, stdout) == EOF) {
            printf("\n Error: fputs()\n");
        }
    } while (n > 0);

    if (n < 0) {
        printf("\n Error: read()\n");
    }
}
