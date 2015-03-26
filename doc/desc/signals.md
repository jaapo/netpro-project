<a id="signals"></a>
Signals
-------

### Client

#### User input through user interface
Client parses entered command. If command is invalid (bad syntax), an error is printed. Otherwise message is sent to client's file server.

#### Unexpected control socket close
Client attempts to reconnect. If connection fails, client reports error and exits.

#### Unexpected data socket close
Client sends server a *status* message to query it's status. If a response is received, previous command is determined unsuccessful and reported to user. Otherwise the server is assumed dead and the client exits.

#### Server timeout
If server doesn't send response fast enough, but the socket is not dead, client disconnects and tries to reconnect to the server.

#### Process receives SIGINT signal
Client closes all data sockets immediately, sends *quit* message to server, waits for server's response and exits.

### File server

#### New connection from client

#### New connection from another file server

#### Unexpected client control socket close
Abort client's current commands. Start a client reconnection timer. If client connects before timeout, continue as usual. Else free client's locks and clean data structures.

#### Unexpected directory server control socket close

#### Unexpected file server data socket close

#### Process receives SIGINT signal

### Direcotry server
