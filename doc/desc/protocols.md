<a id="protocols"></a>
Protocols
---------

There are three protocols. One for client-server communication, second for communication between file servers and the directory server, and third for communication between file servers. 

### Protocol messages
All three protocols are similar in message format, but each of them have messages for their own purposes. All protocol messages contain headers including transaction id, ids of communicating parties and message type field, and then payload data split into sections.

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

#### File access protocol

Clients and file servers use the file access protocol, FAP. FAP uses TCP. TCP port xxxxx is used for control messages and TCP port xxxxx2 for data.

##### Outline

When client process is started, it creates a TCP connection to the file server port xxxxx and sends the **handshake** message. Server registers the client and sends a response. Response contains a port number for data transfer.
Client issues commands to the server using the same initial TCP connection. If command requires file data transmission, the data connection is used for the purpose.
When client exits, it closes the connection by sending a **quit** message to the server. Server acknowledges this.

##### Message format

All messages have a common message format. Messages have headers and sections. Message header contains *transaction_id*, *client id*, *server id*, *file system id*, *message type* and *next section* fields.

All identifiers are 64 bits long. Command number takes 16 bits. *next section* field value is 16 bits.

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
Message type determines rest of the message contents. Possible message types are *handshake*, *handshake response*, *status*, *quit*, *command*, *ack*, *error* and *response*. Message type numbers are in parentheses in the headers.

###### handshake (message type number: 1)
Two string sections: hostname, username. The **client id** and **server id** fields are 0 in this message.

###### quit (3)
Message contains no payload data.

###### status (4)
Message contains no payload data.

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

	Message sections: path of directory as string

##### Response message types
Response messages are sent from file server to client in response to client messages.

###### handshake response (2)
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

###### Message type
Message types are *handshake*, *handshake response*, *lock*, *get info*, *search*, *advertise*, *disconnect*, *ack* and *file info*.

Message sections and numbers:

- handshake (1)

	Sections: server name (string), server maximum capacity in bytes (integer), server current disk usage in bytes (integer)

	File system ID is 0 in this message

- handshake response (2)

	Message contains no payload data

- lock (3)
	
	Sections: lock type (integer), path (string)

	Lock types can be shared (1), exclusive (2) or free (0)

- get info (4)

	Sections: recursion level (integer), path (string)

	Recursion level values:
	- **0**: only one file's or directory's information
	- **1**: only this directory contents
	- **2** or more: recurse to this many levels to get file information

- search (5)

	Sections: search parameters (string)

- advertise (6)
	
	Sections: action (integer), info (file info)

	Action can be delete (1), create (2), modify (3)

- disconnect (7)
	
	Message contains no payload

- ack (8)

	Message contains no payload

- file info (9)

	Message sections: file count, **n** >= 0 (integer), **n** file info sections (file info)

#### File content transfer protocol
Content transfer protocol FCTP uses TCP port xxxxx4 and is used in File server to File server communication. It's very simple.

##### Message format
FCTP is a binary protocol. Every message contain 64-bit *transaction id*, *from server id*, *to server id* and *file system id* fields, 16-bit *message type* and first *next section* fields.

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

There are 3 message types:

- download (1), one string section: file path
- ok (2), one binary section: file contents
- error (3), sections: error number (integer), error messsage (string)

### Protocol operations
#### FAP
FAP is a request-response protocol. Client sends a request and waits for server's response. Requests are sent when user writes a command to the text interface. If some command requires multiple requests, client software takes care of it.

This list describes communication related to each client action.

##### Client startup
When client process starts it creates the FAP TCP connection to port xxxx1. After connection has been successfully initialized, client sends the **handshake** message and waits for server's **handshake response**. When server receives a **handshake** from client, it adds the client to its client list, assigns a data port for it, and replies with the **handshake response**. User is notified about server name and successful connection

##### Commands
User writes commands to the interface to access files. Each command is executed differently, most with a single message.

###### create directory
1. client sends **command** message with **command number** 1 and file type 2
2. server tries to create the directory (process described in XXX)
3. result is reported to the user
	- if creation fails, an **error** message is sent to client and error message printed
	- if creation succeeds, server responds with **response** message **ok**

###### delete directory
1. client sends **command** message with **command number** 7 and file type 2
2. server tries to delete the directory (process described in XXX)
3. result is reported to the user
	- if deletion fails, an **error** message is sent to client and error message printed
	- if deletion succeeds, server responds with **response** message **ok**

###### list directory contents
1. client sends **command** message with **command number** 10
2. file server aquires an **s** lock to the directory
	- error message is sent to client and printed to user
3. file server gets directory contents from directory server
	- error message is sent to client and printed to user
4. file server sends **response** message with **file info** list
5. client receives and prints file list

###### create file
###### edit file
###### delete file
###### read file
###### copy file
###### search file
