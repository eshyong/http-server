#include <iostream>
#include "server.h"
#include "http.h"

int main(void) {
    HttpServer server;
    // Config stuff will go here
    server.Run();
    return 0;
}
