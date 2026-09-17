# HTTP Server in C

A small HTTP server built from scratch in C using POSIX sockets.

## Current features

* TCP socket creation
* `bind()`, `listen()`, and `accept()`
* Receive HTTP requests
* Parse the HTTP request line
* Basic HTTP header parsing
* Send a basic HTTP response

## Status

Work in progress.

## Run

```bash
gcc -Wall -Wextra -Wpedantic -std=c17 server.c -o httpd
./httpd
```

Then:

```bash
curl http://127.0.0.1:8080
```
