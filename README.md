# Build Instructions
Run `make` to use the provided Makefile or manually build the server and client with
```bash
gcc server.c defs.h -o server

gcc client.c defs.h -pthread -o client
```

Start the server and connect with a client using
```bash
./server

./client <username> password
```
