<a id="protocols"></a>
Protocols
---------

There are three protocols. One for client-server communication, second for communication between file servers and the directory server, and third for communication between file servers. 

### Protocol messages
All three protocols are similar in message format, but each of them have messages for their own purposes. All protocol messages contain headers including transaction id, ids of communicating parties and message type field, and then payload data split into sections.

ID fields take 64-bits, *message type* and *section number* are 16-bit unsigned integer

**General message format**

    +----------------------------------------------------------------+
    |                      Transaction ID                            |
    +----------------------------------------------------------------+
    |                                                                |
    |               other ids and header fields                      |
    |                                                                |
    +----------------------------------------------------------------+
    | Message type   | Next section   | Section data ....            |
    +----------------------------------------------------------------+
    | ....                                           | Next section  |

#### Sections
Sections are typed pieces of data and they are placed consecutively after the header. Before each section there is a *next section* field which tells how following bytes should be interpreted.

Valid sections are message specific. Section types are

- *integer* section is always 4-bytes long and it contains only one 4-byte signed integer. Integer is in big-endian byte order and the leftmost bit is sign bit.

- *string* section has two fields: *string length* and *data*. *string length* is a 32-bit unsigned integer and *data* is an arbitrary length ASCII data field. *data* must be *string length* bytes long.

- *binary* section is same as *string* but it may contain non-ASCII characters.

- *file info* section contains following data:
	- file type byte (1=regular file, 2=directory)
	- file path length integer (unsigned, 32-bits, big-endian)
	- file path as ASCII string
	- user name length integer (unisgned, 32-bits, big-endian)
	- user name of last modifier
	- modification time (unsigned, 32-bits, big-endian)
	- file size in bytes (unsigned, 32-bits, big-endian)
	- file availability, number of servers currently storing the file (unsigned, 32-bits, big-endian)

#### Errors
Error codes are also same for all protocols. Error code usage is described in protocol operations section. Error messages should describe the error in more detail. For example "file /home/todo.txt doesn't exist".

Error codes and explanations

| code | explanation           |
| ---- | --------------------- |
| 1    | file not found
| 2    | file already exist
| 3    | incompatible lock
| 4    | command syntax error
| 5    | file server unavailable
| 6    | directory server unavailable
| 7    | insufficient replicas
| 8    | abort
| 9    | malformed message
| 10   | unknown error

#### File access protocol

Clients and file servers use the file access protocol, FAP. FAP uses TCP. TCP port xxxxx is used for control messages and TCP port xxxxx2 for data.

##### Outline

When client process is started, it creates a TCP connection to the file server port xxxxx and sends the **hello** message. Server registers the client and sends a response. Response contains a port number for data transfer.
Client issues commands to the server using the same initial TCP connection. If command requires file data transmission, the data connection is used for the purpose.
When client exits, it closes the connection by sending a **quit** message to the server. Server acknowledges this.

##### Message format

All messages have a common message format. Messages have headers and sections. Message header contains *transaction_id*, *client id*, *server id*, *file system id*, *message type* and *next section* fields.

All identifiers are 64 bits long. *message type* takes 16 bits. *next section* field value is 16 bits.

Every section ends with a the *next section* field. Rest of section format depends on the section type. A message may contain 0 to 127 sections.

    +----------------------------------------------------------------+
    |                      Transaction ID                            |
    +----------------------------------------------------------------+
    |                      Server ID                                 |
    +----------------------------------------------------------------+
    |                      Client ID                                 |
    +----------------------------------------------------------------+
    |                      File system ID                            |
    +----------------------------------------------------------------+
    | Message type   | Next section   | Section data ....            |
    +----------------------------------------------------------------+
    | ....                                           | Next section  |

*transaction_id* identifies a single client server interaction. Transactions are initiated by clients and server's reponse must contain the same *transaction_id*.

##### Message types
Message type determines rest of the message contents. Message types are *hello*, *hello response*, *status*, *quit*, *command*, *ack*, *error* and *response*. Message type numbers are in parentheses in the headers.

###### hello (message type number: 1)
Two string sections: hostname, username. The **client id** and **server id** fields are 0 in this message.

###### quit (3)
Message contains no payload data.

<!--###### status (4)
Message contains no payload data.-->

###### command (5)
Every command message contains a *command number* section. It is an integer section with command number.
Messages contain additional sections depending on command.

Command names (and command numbers):

- create (command number: 1)

	Message sections: full path string, file type integer (1=regular file, 2=directory)

- open (2)

	Message sections: full path string

- close (3)

	Message sections: full path string

- stat (4)

	Message sections: full path string

- read (5)

	Message sections: file path string, integer offset to start reading from, integer byte count to read. If byte count is **0** whole file is read.

- write (6)

	Message sections: full path string, data byte count (integer)

- delete (7)

	Message sections: full file path as string, file type integer (1=regular file, 2=directory)

- copy (8)

	Message sections: source file path string and destination file path string, file type integer (1=regular file, 2=directory)

- find (9)

	Message sections: search term string, string path of directory to search for

- list (10)

	Message sections: recurse integer (1=yes, 0=no), path of directory as string

##### Response message types
Response messages are sent from file server to client in response to client messages.

###### hello response (2)
Sections: server name (string), TCP port number for data transfer (integer)

###### ack (7)
Message contains no payload

###### error (8)
If an error occurs when server is handling the request, an error message is sent back to client.
Sections: integer error number, string error message

###### response (6)
*response* messages are responses to *command* messages. Response messages contain *response number* section. It is an integer section containing the response number.

Response messages (and their numbers) are

- ok (1)

	contains no payload data

- file information (2)

	sections: file count, **n** >= 0 (integer), **n** file info sections (file info)

- data out (3)

	sections: data length in bytes (integer)

<!--- file list (6)

	sections: arbitrary number of string sections, each containing a single file name-->

#### Directory control protocol
Directory control protocol DCP uses TCP port xxxxx3. File servers make requests to the Directory server using this protocol. It is similar to FAP.

This protocol is used to handle locks and query directory.

##### Message format
DCP is a binary protocol. Every message contain 64-bit *transaction id*, *server id* and *file system id* fields, 16-bit *message type* and first *next section* fields.

    +----------------------------------------------------------------+
    |                      Transaction ID                            |
    +----------------------------------------------------------------+
    |                      Server ID                                 |
    +----------------------------------------------------------------+
    |                      File system ID                            |
    +----------------------------------------------------------------+
    | Message type   | Next section   | Section data ....            |
    +----------------------------------------------------------------+
    | ....                                           | Next section  |

##### Message type
Message types are *hello*, *hello response*, *lock*, *create*, *read*, *update*, *delete*, *search*, *invalidate*, *replica*, *disconnect*, *ack* and *file info*.

Message sections and numbers:

- hello (1)

	Sections: server name (string), server maximum capacity in bytes (integer), server current disk usage in bytes (integer), server file count (integer)

	File system ID and Server ID are 0 in this message

- hello response (2)

	Sections: refresh (integer)

	refresh is 0 or 1

- lock (3)
	
	Sections: lock type (integer), path (string)

	Lock types can be shared (1), exclusive (2) or free (0)

- create (4)

	Sections: info (file info)

- read (5)

	Sections: recursion level (integer), path (string)

	Recursion level values:
	- **0**: only one file's or directory's information
	- **1**: only this directory contents
	- **2** or more: recurse to this many levels to get file information

- update (6)

	sections: info (file info)

- delete (7)

	sections: path (string)

- search (8)

	Sections: search parameters (string)

- invalidate (9)
	
	Sections: path (string)

- replica (10)

	Sections:

	- action (integer), value is -1 (deleted replica) or 1 (new replica)
	- path (string)

- file replicas (11)

	Sections: host count (**n**) (integer), hostnames (**n** strings)

- get file replicas (12)

	Sections: path (string)

- disconnect (13)
	
	Message contains no payload

- ack (14)

	Message contains no payload

- file info (15)

	Message sections: file count, **n** >= 0 (integer), **n** file info sections (file info)

- error (16)

	Message sections: integer error number, string error message

#### File content transfer protocol
Content transfer protocol FCTP uses TCP port xxxxx4 and is used in File server to File server communication. It's very simple.

##### Message format
FCTP is a binary protocol. Every message contain 64-bit *transaction id*, *from server id*, *to server id* and *file system id* fields, 16-bit *message type* and first *next section* fields.

    +----------------------------------------------------------------+
    |                      Transaction ID                            |
    +----------------------------------------------------------------+
    |                      Source Server ID                          |
    +----------------------------------------------------------------+
    |                      Destination server ID                     |
    +----------------------------------------------------------------+
    |                      File system ID                            |
    +----------------------------------------------------------------+
    | Message type   | Next section   | Section data ....            |
    +----------------------------------------------------------------+
    | ....                                           | Next section  |

There are 3 message types:

- download (1), one string section: file path
- ok (2), one binary section: file contents
- error (3), sections: error number (integer), error messsage (string)

### Protocol operations
#### FAP
FAP is a request-response protocol. Client sends a request and waits for server's response. Requests are sent when user writes a command to the text interface. If some command requires multiple requests, client software takes care of it.

This list describes communication related to each client action.

##### Client startup
When client process starts it creates the FAP TCP connection to port xxxx1. After connection has been successfully initialized, client sends the **hello** message and waits for server's **hello response**. When server receives a **hello** from client, it adds the client to its client list, assigns a data port for it, and replies with the **hello response**. Client opens another TCP connection to the server using remote port xxxx2. User is notified about server name and successful connection.

##### Error handling
If file server fails fullfilling a client request, it sends an **error** response to the client. This means that the current transaction is over and the operation failed. **error** message contains error number. Message contains also a string describing the error. File server must log errors. Client must show error's type and description to user.

Possible errors are listed in sub-items in the message sequence lists below.

##### Commands
User writes commands to the interface to access files. Each command is executed differently, most with a single message.

###### create directory
1. client: send **create** **command** message (**command number**=1 and **file type**=2)
3. server: send **create** message to the directory server
	- already exists error is possible
	- directory doesn't exist is possible (for parent directory)
4. server: receive directory server response
5. server: send **ok** **response** to client

###### delete directory
1. client: send **delete** **command** message (file type=2)
2. server: send **delete** message to directory server
	- lock error is possible
	- doesn't exist error is possible
4. server: receive directory server response
3. server: send **ok** **response** to client

###### list directory contents
1. client: send **list** **command**
3. server: send directory server **read** **message** (**recursion level**=1)
	- doesn't exist error is possible
4. server: receive directory server response
6. server: send **file info** **response** to client

###### create file
1. client: send **command** message with **command number** 1 and file type 1
2. server: send **create** message to directory server (implicit **x** locking)
	- already exists error is possible
3. server: get directory server response
3. server: create the file to file server local data directory
4. server: send **response** **ok** to client
5. client: mark file as open

###### open file
1. client: send **open** **command** to the server
2. server: send **lock** message for an **x** lock
	- lock error is possible
	- doesn't exist error is possible
3. server: send **ok** **response**
4. client: mark file as open

###### edit file
1. client: file must be open
2. user: saves the file
3. client: send **write** **command** message with content length information
4. client: data connection to port xxxx2 must be open
5. client: send data to xxxx2 port
6. server: send **ok** **response** when data successfully received
7. client: 
	- if user has closed the file and is not editing it anymore goto step 8
	- otherwise done
8. client: send **close** **command**
9. server: send **update** message to directory server
9. server: free file lock
9. server: send **ok** **response** to client

###### delete file
1. client: send **delete** **command** with file type 1
3. server: send **delete** message to directory server
	- doesn't exist error is possible
4. server: send **response** **ok** to client

###### read file
1. client: file must be open
2. client: send **read** **command** to server
3. server: get file
3. server: reply with **data out** **response**
4. server: send data to data connection
5. client: receive data from data connection
6. client: data is passed to user

###### copy file
1. client: send **copy** **command** to server
2. server: get file
3. server: send **create** message to directory server (implicit **x** lock)
	- already exist error is possible
4. server: copy contents locally
6. server: free **x** lock
7. server: send client **ok** **response**

###### search file
1. client: send **find** **command** to server
2. server: parse client command parameters
	- syntax error possible
3. server: send **search** message to directory server
4. server: receive **file info** response from directory server
5. server: send **file information** message to client

###### quit
1. client: send **quit** **command** to server
2. server: close client data connection
3. server: reply to client **ok** **response**
4. client: process close
5. server: release all locks related to client
6. server: free client data structures

#### DCP
DCP is a request-response protocol. Message sequences of transactions are described in sections below. *transaction id*s stay same in one transaction.

##### File server start
1. file server: count stored files and the space they take
1. file server: send **hello** to directory server
2. directory server: reply with **hello response**
	- if file server has files (file count in hello is not 0), set refresh to 1
	- otherwise set refresh to 0 and protocol is done
4. file server: send **file info** message containing all server files
5. directory server: compare timestamps of file server's files and directory server files
6. directory server: send invalidate message for all obsolete files

After this, file server may start serving clients.

##### File server exit
1. file server 1: send **disconnect** message
2. directory server: send **ok** to file server 1
3. directory server: remove file server 1 from replica lists
4. directory server: remove other file server 1 data structures, free locks

##### Error handling
If a DCP request tries to access a non existent file, a *file not found* error is send back. An operation which would create another file to an already taken path yields *file already exists* error. *lock error* occurs when a lock is needed for file which is already incompatibly locked (see **locks** section below).

##### Operations
###### locks
File server reserves locks for files sending **lock** messages. When a lock is requested, directory server checks if it can be given. Any file may have  multiple shared (**s**) locks at a time. An exclusive (**x**) lock forbids all other locks on an object. If a file server already has an **s** lock, an **x** lock request is an upgrade request and may be fullfilled if no other server holds a lock on the object.

Locks are reserved implicitly on certain operations. File creation (**create** message) **x** locks a file. Short term locks are automatically handled on directory server to guarantee operation atomicity (e.g. **delete**).

Locking sequence:
1. file server 1: need to lock a file
2. file server 1: send **lock** request to directory server
3. directory server: create lock for file server 1 if possible
3. directory server: send **ok** to file server 1
	- or **error** if impossible lock
4. file server 1: continue operations
5. file server 1: send **lock** free message
6. directory server: remove lock
7. directory server: send **ok** to file server

###### create
1. file server 1: send **create** message
2. directory server: create file entry to the directory
3. directory server: add file server 1 to the list of file storers
3. directory server: send **file info** back

###### read
1. file server 1: send **read** message to directory server
2. directory server: find requested data, traverse recursively if necessary
3. directory server: send **file info** message

###### update
1. file server 1: send **update** message to directory server
2. directory server: check that file server 1 has an **x** lock on file
3. directory server: assign a timestamp to the update, save it to directory
4. directory server: send updated **file info** to file server 1
5. directory server: send **invalidate** to all other servers storing the file

###### delete
1. file server 1: send **delete** message
2. directory server: check that file server 1 can get an **x** lock on file
3. directory server: send updated **ok** to file server 1
4. directory server: send **invalidate** to all other servers storing the file

###### invalidate
1. directory server: send **invalidate** message to servers with replica
2. directory server: remove servers from file's replica list
3. file servers: remove local file
4. file servers: send **ack** back

###### search
1. file server: send **search** message to directory server
2. directory server: find files matching **search** string (wildcard expression)
3. directory server: send file server **file info** of matched files

###### replica 1
1. file server 1: download a file from file serve 2
2. file server 1: send **replica** (action=1) message to directory server

###### replica -1
1. file server 1: too much files, try to remove some
2. file server 1: send **replica** (action=-1) message to directory server
3. directory server: 
	- if number of file replicas is greater than `min_replicas` configuration parameter, send **ok**
	- else send **insufficient replicas** **error**
4. file server 1: 
	- remove local file, if **ok** received
	- else find another file file to remove

###### get replicas
1. file server 1: client asks for file, no local replica
2. file server 1: send **get replicas** message to directory server
3. directory server: send **file replicas** message to file server 1
4. file server 1: try to get from first server in list (file server 2)
	- try next if server unavailable or other error
	- if no server can serve, send **insufficient replicas** to client, done

#### FCTP
FCTP has only one operation, file transfer.

##### File transfer
1. file server 1: client requests a file, directory server tells that it can be found from file server 2
2. file server 1: send **download** message to file server 2
3. file server 2: send **ok** message to file server 1 with file contents
	- if file can't be found send file not found **error** message
4. file server 1: receive file
	- if capacity is too close to maximum, find least recently used file, send **replica -1** message to directory
	- if directory response is **ok**, delete file from disk
	- otherwise, find next least recently used file that may be removed
6. file server 1: continue


