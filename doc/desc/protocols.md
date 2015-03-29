<a id="protocols"></a>
Protocols
---------

There are two protocols. One for client-server communication and other for communication between file servers and the directory server.

### File access protocol

Clients and file servers use the file access protocol, FAP. FAP uses TCP. TCP port xxxxx is used for control messages and TCP port xxxxx2 for data.

#### Outline

When client process is started, it creates a TCP connection to the file server port xxxxx and sends the **handshake** message. Server registers the client and sends a response.

Client issues commands to the server using the same initial TCP connection. If command requires file data transmission, a separate connection is opened for the purpose. Server's control message includes a request to open this new connection. Client should then accept servers connection to port xxxxx2 and receive data described by the control message.

When client exits, it closes the connection by sending a **quit** message to the server. Server acknowledges this.

#### Message format

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

##### Message type
Message type determines rest of the message contents. Possible message types are *handshake*, *handshake response*, *status*, *quit*, *command*, *ack*, *error* and *response*. Message type numbers are in parentheses in the headers.

##### Sections
Valid sections are message specific. Section types are *integer*, *string* and *binary*.

*integer* section is always 4-bytes long and it contains only one 4-byte signed integer. Integer is in big-endian byte order and the leftmost bit is sign bit.

*string* section has two fields: *string length* and *data*. *string length* is a 32-bit unsigned integer and *data* is an arbitrary length ASCII data field. *data* must be *string length* bytes long.

*binary* section is same as *string* but it may contain non-ASCII characters.

##### handshake (message type number: 1)
Two string sections: hostname, username. The **client id** and **server id** fields are 0 in this message.

##### quit (3)
Message contains no payload data.

##### status (4)
Message contains no payload data.

##### command (5)
Every command message contains a *command number* section. It is an integer section with command number.
Messages contain additional sections depending on command.
Following command descriptions contain the command number in parentheses.

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
Message sections: full path string, binary data to write

- delete (7)
Message sections: full file path as string, file type integer (1=regular file, 2=directory)

- copy (8)
Message sections: source file path string and destination file path string, file type integer (1=regular file, 2=directory)

- find (9)
Message sections: search term string, string path of directory to search for

- list (10)
Message sections: path of directory as string

##### Responses
Response messages are sent from file server to client in response to client messages.

###### ack (7)
Message contains no payload

###### error (8)
If an error occurs when server is handling the request, an error message is sent back to client.
Sections: integer error number, string error message

##### response (6)
*response* messages are responses to *command* messages. Response messages contain *response number* section. It is an integer section containing the response number.

Response messages (and their numbers) are

- ok (1)
	contains no payload data

- file status (2)
	sections:

	- file size in bytes (integer)
	- modification time (integer)
	- creator (string)

- data in (3)
	sections:

	- port (integer)

- data out (4)
	sections:

	- data length in bytes (integer)

- file list (5)
	sections:

	- arbitrary number of string sections, each containing a single file name

### Directory control protocol
Directory control protocol DCP uses TCP port xxxxx3. File servers make requests to the Directory server using this protocol. It is similar to FAP, but has it's own messages.

This protocol is used to handle locks and query directory.

### File content transfer protocol
Content transfer protocol FCTP uses TCP port xxxxx4 and is used in File server to File server communication. It is similar to FAP, but the messages are different.
