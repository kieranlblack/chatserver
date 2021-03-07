# Build Instructions
Run `make` to use the provided Makefile or manually build the server and client with
```bash
gcc chatserver.c defs.h -o chatserver

gcc chatclient.c defs.h -pthread -o chatclient
```

Start the server and connect with a client using
```bash
./chatclientserver

./chatclient \
    --username <username> \
    --password cs3251secret \
    --host 127.0.0.1 \
    --port 5001
```
