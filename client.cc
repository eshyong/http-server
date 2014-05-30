#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

class Client {
private:
    struct sockaddr_in servaddr;
    char recvbuf[4096];
    int sockfd;

public:
    Client(int port) {
        memset(recvbuf, 0, sizeof(recvbuf));
        sockfd = socket(AF_INET, SOCK_STREAM, NULL);
        if (sockfd < 0) {
            perror("Error: socket()\n");
            exit(1);
        }

        memset(&servaddr, 0, sizeof(servaddr));
        servaddr.sin_family = AF_INET;
        servaddr.sin_port = htons(port);
    }

    void connect(const char* address) {
        if (inet_pton(AF_INET, address, &servaddr.sin_addr) <= 0) {
            perror("Error: inet_pton()\n");
            exit(1);
        }
    }
};

int main(int argc, char *argv[]) {
    return 0;
}
