Protocols
=========

There are two protocols. One for client-server communication and other for communication between file servers and the directory server.

File access protocol
--------------------

Clients and file servers use the file access protocol, FAP. FAP uses TCP. TCP port xxxxx is used for control messages and TCP port xxxxx2 for data.

### Outline

When client process is started, it creates a TCP connection to the file server port xxxxx and sends the **handshake** message. Server registers the client and sends a response.

Client issues commands to the server using the same initial TCP connection. If command requires file data transmission, a separate connection is opened for the purpose. Server's control message includes a request to open this new connection. Client should then accept servers connection to port xxxxx2 and receive data described by the control message.

When client exits, it closes the connection by sending a **quit** message to the server. Server acknowledges this.

### Message format

All messages have a common message format. Messages have headers and sections. Message header contains *message_id*, *client id*, *server id*, *file system id*, *message type* and *next section* fields.

All identifiers are 64 bits long. Command number takes 16 bits. *next section* field value is 16 bits.

Every section ends with a the *next section* field. Rest of section format depends on the section type. A message may contain 0 to 127 sections.

    +----------------------------------------------------------------+
    |                      Message ID                                |
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

#### Message type

Message type determines rest of the message contents. Possible message types are *handshake*, *status*, *quit*, *command* and *response*. Their codes are 1, 2, 3, 4 and 5 respectively.

#### Sections

Valid sections are message specific. Section types are *integer* and *string*.

*integer* section is always 4-bytes long and it contains only one 4-byte signed integer. Integer is in big-endian byte order and the leftmost bit is sign bit.

*string* section has two fields: *string length* and *data*. *string length* is a 32-bit unsigned integer and *data* is an arbitrary length ASCII data field. *data* must be *string length* bytes long.

##### handshake

Two string sections: hostname, username.

##### command

Section types are *command number* and *arguments*.
*command number* section is an 8-bit section with command number.
*argument* section contains string argument. Command message may contain multiple *argument* sections.

##### quit

Message contains no payload data.

##### status

Message contains no payload data.

##### response

Directory control protocol
--------------------------

Directory control protocol DCP uses TCP port xxxxx3. File servers make requests to the Directory server using this protocol. It is similar with FAP, but has it's own messages.

File content transfer protocol
------------------------------

Content transfer protocol FCTP uses TCP port xxxxx4 and is used in File server to File server communication. It is similar with FAP, but the messages are different.
