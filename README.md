# Me
Kieran Black ([kieran@gatech.edu](mailto:kieran@gatech.edu))

# Files Included
| Filename        | Description                                   |
| :-------------- | :-------------------------------------------- |
| `chatserver.c`  | Main event loop for the server.               |
| `chatserver.h`  | Header file for the server.                   |
| `chatclient.c`  | Client used to connect to the server.         |
| `chatclient.h`  | Header file for the client.                   |
| `defs.h`        | Common header for both the server and client. |
| `README.md`     | Various information about the project.        |
| `Makefile`      | Makefile for the project.                     |

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
