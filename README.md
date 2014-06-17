HTTP-Server
===========

A toy HTTP server, with support for multi-process and multi-threading.

Usage: 

Compile using `make all`, and use `make clean` to remove all object files and executables.
Runs in multi-process mode by default, and writes request and response text to STDOUT.
HTML files for testing are in folder `test`.

Flags:

--mprocess: run in multi-process mode
--mthreaded: run in mult-threaded mode
--evented: run in evented mode
--silent: silences all output

TODO:
- Configurable hosting folder and port number
- POST, PUT, DELETE
- Implement more status codes (Resource has moved, etc)
