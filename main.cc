#include <iostream>
#include "server.h"
#include "http.h"

using std::cout;
using std::endl;

int main(int argc, char* argv[]) {
    // Config stuff will go here
    server_type type = MPROCESS;
    bool verbose = true;
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], "hi") == 0) {
                cout << "Hello to you too!\n";
            } else if (strcmp(argv[i], "--mprocess") == 0) {
                type = MPROCESS;
            } else if (strcmp(argv[i], "--mthreaded") == 0) {
                type = MTHREADED;
            } else if (strcmp(argv[i], "--evented") == 0) {
                type = EVENTED;
            } else if (strcmp(argv[i], "--silent") == 0 || strcmp(argv[i], "-s") == 0) {
                verbose = false;
            } else if (strcmp(argv[i], "--help") == 0) {
                cout << "Usage: grizzly [flags]\n";
                cout << "By default, grizzly runs in multiprocessed mode.\n";
                cout << "   flags:\n";
                cout << "           --mprocess: server runs in multiprocessed mode\n";
                cout << "           --mthreaded: server runs in multithreaded mode\n";
                cout << "           --evented: server runs in evented mode\n";
                cout << "           --config /path/to/options.conf: specifies the path to the configuration file you want to read.\n";
                cout << "                                           the default path is $PWD/test/http.conf.\n";
                cout << "           --www /path/to/localhost: specifies the path to the localhost folder. the default path is test/home.\n";
                cout << "           -s/--silent: silences any HTTP requests and responses, which are usually written to stdout.\n";
                exit(EXIT_SUCCESS);
            } else {
                cout << "Unknown flag.\n";
            }
        }
    }
    HttpServer Grizzly;
    Grizzly.Run(type, verbose);
    return 0;
}
