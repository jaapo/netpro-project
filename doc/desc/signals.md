<a id="signals"></a>
Signals
-------

### Client process

#### User input through user interface
Client parses entered command. If command is invalid (bad syntax), an error is printed. Otherwise message is sent to client's file server.

#### Unexpected control socket close
Client attempts to reconnect once. If connection fails, client reports error and exits.

#### Unexpected data socket close
Client attempts to reconnect once. If connection fails, client reports error and exits.

#### Server timeout
If server doesn't send response fast enough, but the socket is not dead, client disconnects and tries to reconnect to the server.

#### Process receives SIGINT signal
Same behaviour as with normal UI quit.

### File server process

#### Unexpected client control socket close
Abort client's current commands. Start a client reconnection timer. If client connects before timeout, continue as usual. Else free client's locks and clean data structures.

#### Unexpected directory server control socket close
Attempt to reconnect for 300 seconds. In case of successful reconnection, continue normal operations. If reconnection is unsuccessful, abort all client transactions with **directory server** **error**, but keep them open. Retry directory server reconnection every 60+r seconds where r is random number between 0 and 20.

#### Unexpected file server data socket close
If connection was opened by this process, retry whole download process 5 times (get peer info from server etc.). If this doesn't work, send **file server** **error** to the client.

#### Process receives SIGINT signal
Send **quit** to every client, wait for **ack** response and connection closing. Operations are aborted with **error** **abort**. If client won't close connection, close connection. After clients are done, send **quit** to directory server.

### Directory server
#### Unexpected client control socket close
Abort client's current commands. Start a client reconnection timer. If client connects before timeout, continue as usual. Else free client's locks and clean data structures.

#### Process receives SIGINT signal
Send **disconnect** to all file servers, aborting operations. Write `fs` file, clear `fs.log`. Exit.
